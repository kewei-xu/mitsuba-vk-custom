from __future__ import annotations

import argparse
import json
import math
import struct
import time
from array import array
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import drjit as dr
import mitsuba as mi
try:
    import numpy as np
except ImportError:
    np = None


@dataclass
class ScalarRing:
    theta_min: float
    theta_max: float
    phi_step: float
    base_index: int
    patch_count: int


@dataclass
class ScalarGrid:
    precision: int
    patch_count: int
    theta_cap: float
    cell_solid_angle: float
    rings: list[ScalarRing]

    def index(self, phi: float, theta: float) -> int:
        phi = phi % (2.0 * math.pi)
        theta = max(0.0, min(theta, 0.5 * math.pi))

        ring_id = 0
        while ring_id + 1 < len(self.rings) and theta >= self.rings[ring_id].theta_max:
            ring_id += 1

        ring = self.rings[ring_id]
        patch = min(int(math.floor(phi / ring.phi_step)), ring.patch_count - 1)
        return ring.base_index + patch

    def patch_centers(self) -> tuple[list[float], list[float]]:
        theta_values = [0.0] * self.patch_count
        phi_values = [0.0] * self.patch_count

        for ring in self.rings:
            theta_center = 0.5 * (ring.theta_min + ring.theta_max)
            for patch in range(ring.patch_count):
                idx = ring.base_index + patch
                theta_values[idx] = theta_center
                if ring.patch_count == 1:
                    phi_values[idx] = 0.0
                else:
                    phi_values[idx] = (patch + 0.5) * ring.phi_step

        return theta_values, phi_values


_GRID_CACHE: dict[int, ScalarGrid] = {}
_PATCH_CENTER_CACHE: dict[int, tuple[object, object]] = {}
_PROJECTION_INDEX_CACHE: dict[tuple[int, int], object] = {}
_PROJECTION_SRC_OFFSET_CACHE: dict[tuple[int, int], object] = {}
_CACHE_DIR = Path(__file__).resolve().parent / ".cache"


def expected_patch_count(precision: int) -> int:
    mapping = {1: 1663, 2: 26279, 3: 53224, 4: 523910}
    return mapping.get(precision, 0)


def theta_cap(precision: int) -> float:
    mapping = {
        1: math.radians(1.987071),
        2: math.radians(0.4998442),
        3: math.radians(0.3512243),
        4: math.radians(0.1119462),
    }
    return mapping.get(precision, 0.0)


def build_scalar_grid(precision: int) -> ScalarGrid:
    cached = _GRID_CACHE.get(precision)
    if cached is not None:
        return cached

    patch_count = expected_patch_count(precision)
    cap = theta_cap(precision)
    if patch_count == 0 or cap == 0.0:
        raise ValueError("precision must be in [1, 4]")

    rings = [ScalarRing(0.0, cap, 2.0 * math.pi, 0, 1)]
    theta_p = cap
    radius_p = 2.0 * math.sin(0.5 * theta_p)
    k_p = 1

    while theta_p < 0.5 * math.pi:
        theta = theta_p + 2.0 * math.sin(0.5 * theta_p) * math.sqrt(math.pi / k_p)
        radius = 2.0 * math.sin(0.5 * theta)
        k = round((radius / radius_p) * (radius / radius_p) * k_p)

        theta = math.acos(math.cos(theta_p) - (1.0 - math.cos(cap)) * (k - k_p))
        ring_patch_count = k - k_p
        rings.append(
            ScalarRing(
                theta_min=theta_p,
                theta_max=theta,
                phi_step=(2.0 * math.pi) / ring_patch_count,
                base_index=k_p,
                patch_count=ring_patch_count,
            )
        )

        theta_p = theta
        radius_p = 2.0 * math.sin(0.5 * theta_p)
        k_p = k

    if k_p != patch_count:
        raise RuntimeError(
            f"gonio grid mismatch: expected {patch_count} cells, got {k_p}"
        )

    grid = ScalarGrid(
        precision=precision,
        patch_count=patch_count,
        theta_cap=cap,
        cell_solid_angle=2.0 * math.pi * (1.0 - math.cos(cap)),
        rings=rings,
    )
    _GRID_CACHE[precision] = grid
    return grid


def incident_direction(theta_i_deg: float, phi_i_deg: float) -> tuple[float, float, float]:
    theta = math.radians(theta_i_deg)
    phi = math.radians(phi_i_deg)
    return (
        -math.sin(theta) * math.cos(phi),
        -math.sin(theta) * math.sin(phi),
        -math.cos(theta),
    )


def ensure_plugin_search_path() -> None:
    mi_module = Path(mi.__file__).resolve()
    release_root = mi_module.parent.parent.parent
    plugin_dir = release_root / "plugins"
    if not plugin_dir.exists():
        return

    resolver = mi.file_resolver()
    resolver.append(str(release_root))


def layer_descriptors(merge_l1_l2: bool, record_refraction: bool, analytic_measurement: bool) -> list[tuple[str, str]]:
    if analytic_measurement or merge_l1_l2:
        return [("L1", "H+"), ("L1", "H-")] if record_refraction else [("L1", "H+")]
    if record_refraction:
        return [("L1", "H+"), ("L1", "H-"), ("L2", "H+"), ("L2", "H-")]
    return [("L1", "H+"), ("L2", "H+")]


def make_bsdf_dict(bsdf_type: str,
                   material: str,
                   reflectance: float,
                   alpha: float,
                   int_ior: float,
                   ext_ior: float) -> dict:
    if bsdf_type == "diffuse":
        return {
            "type": "diffuse",
            "reflectance": {"type": "rgb", "value": [reflectance] * 3},
        }
    if bsdf_type == "conductor":
        return {
            "type": "conductor",
            "material": material,
        }
    if bsdf_type == "roughconductor":
        return {
            "type": "roughconductor",
            "material": material,
            "alpha": alpha,
        }
    if bsdf_type == "dielectric":
        return {
            "type": "dielectric",
            "int_ior": int_ior,
            "ext_ior": ext_ior,
        }
    if bsdf_type == "roughdielectric":
        return {
            "type": "roughdielectric",
            "alpha": alpha,
            "int_ior": int_ior,
            "ext_ior": ext_ior,
        }
    raise ValueError(f"Unsupported bsdf_type: {bsdf_type}")


def unpack_tensor(tensor) -> tuple[tuple[int, ...], array]:
    shape = tuple(tensor.shape)
    if np is not None:
        return shape, np.frombuffer(tensor.array.memview(), dtype=np.float32)
    return shape, array("f", tensor.array.memview())


def write_float_dat(path: Path, values: Iterable[float]) -> None:
    if np is not None and isinstance(values, np.ndarray):
        payload = np.asarray(values, dtype=np.float32).tobytes()
    elif isinstance(values, array) and values.typecode == "f":
        payload = values.tobytes()
    else:
        payload = array("f", values).tobytes()
    with path.open("wb") as handle:
        handle.write(payload)


def write_rgb_dat(path: Path, values) -> None:
    if np is not None and isinstance(values, np.ndarray):
        write_float_dat(path, values.reshape(-1))
        return
    if isinstance(values, array) and values.typecode == "f":
        write_float_dat(path, values)
        return

    flat = array("f")
    for rgb in values:
        flat.extend(rgb)
    write_float_dat(path, flat)


def write_energy(path: Path, energy: tuple[float, float, float]) -> None:
    with path.open("w", encoding="utf-8") as handle:
        handle.write(f"R: {energy[0]}\n")
        handle.write(f"G: {energy[1]}\n")
        handle.write(f"B: {energy[2]}\n")


def tonemap_byte(value: float, scale: float) -> int:
    value = max(0.0, value * scale)
    value = min(value, 1.0)
    return int(round((value ** (1.0 / 2.2)) * 255.0))


def write_rgb_bmp(path: Path, width: int, height: int, rgb_flat: list[float]) -> None:
    if np is not None and isinstance(rgb_flat, np.ndarray):
        max_value = float(np.max(rgb_flat)) if rgb_flat.size else 0.0
    else:
        max_value = max(rgb_flat) if rgb_flat else 0.0
    scale = 1.0 / max_value if max_value > 0.0 else 1.0

    row_stride = width * 3
    padding = (4 - (row_stride % 4)) % 4
    if np is not None and isinstance(rgb_flat, np.ndarray):
        image = np.asarray(rgb_flat, dtype=np.float32).reshape(height, width, 3)
        scaled = np.clip(image * np.float32(scale), 0.0, 1.0)
        tonemapped = np.rint(np.power(scaled, np.float32(1.0 / 2.2)) * np.float32(255.0)).astype(np.uint8)
        bgr = tonemapped[::-1, :, ::-1]

        if padding == 0:
            pixel_bytes = bgr.tobytes()
        else:
            rows = np.zeros((height, row_stride + padding), dtype=np.uint8)
            rows[:, :row_stride] = bgr.reshape(height, row_stride)
            pixel_bytes = rows.tobytes()
        pixel_size = len(pixel_bytes)
    else:
        pixel_bytes = bytearray()

        for y in range(height - 1, -1, -1):
            for x in range(width):
                base = (y * width + x) * 3
                r = tonemap_byte(rgb_flat[base], scale)
                g = tonemap_byte(rgb_flat[base + 1], scale)
                b = tonemap_byte(rgb_flat[base + 2], scale)
                pixel_bytes.extend((b, g, r))
            pixel_bytes.extend(b"\x00" * padding)
        pixel_size = len(pixel_bytes)
    file_size = 14 + 40 + pixel_size

    with path.open("wb") as handle:
        handle.write(b"BM")
        handle.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        handle.write(struct.pack("<IIIHHIIIIII",
                                 40, width, height, 1, 24, 0,
                                 pixel_size, 2835, 2835, 0, 0))
        handle.write(pixel_bytes)


def write_sensor_exr(path: Path, bitmap) -> None:
    bitmap.write(str(path))


def write_rgb_exr(path: Path, width: int, height: int, rgb_flat) -> None:
    if np is not None:
        image = np.asarray(rgb_flat, dtype=np.float32).reshape(height, width, 3)
    else:
        image = mi.TensorXf(array("f", rgb_flat), (height, width, 3))
    mi.Bitmap(image).write(str(path))


def patch_center_cache_path(grid: ScalarGrid) -> Path:
    return _CACHE_DIR / f"patch_centers_p{grid.precision}.bin"


def patch_centers_cached(grid: ScalarGrid) -> tuple[array, array]:
    cached = _PATCH_CENTER_CACHE.get(grid.precision)
    if cached is not None:
        return cached

    cache_path = patch_center_cache_path(grid)
    if cache_path.exists():
        if np is not None:
            packed = np.fromfile(str(cache_path), dtype=np.float32)
        else:
            packed = array("f")
            with cache_path.open("rb") as handle:
                packed.frombytes(handle.read())
        if len(packed) == grid.patch_count * 2:
            cached = (packed[:grid.patch_count], packed[grid.patch_count:])
            _PATCH_CENTER_CACHE[grid.precision] = cached
            return cached

    theta_values, phi_values = grid.patch_centers()
    if np is not None:
        cached = (
            np.asarray(theta_values, dtype=np.float32),
            np.asarray(phi_values, dtype=np.float32),
        )
    else:
        cached = (array("f", theta_values), array("f", phi_values))
    _CACHE_DIR.mkdir(parents=True, exist_ok=True)
    with cache_path.open("wb") as handle:
        handle.write(cached[0].tobytes())
        handle.write(cached[1].tobytes())
    _PATCH_CENTER_CACHE[grid.precision] = cached
    return cached


def projection_cache_path(grid: ScalarGrid, image_size: int) -> Path:
    return _CACHE_DIR / f"projection_p{grid.precision}_s{image_size}.bin"


def projection_src_offset_cache_path(grid: ScalarGrid, image_size: int) -> Path:
    return _CACHE_DIR / f"projection_src_offsets_p{grid.precision}_s{image_size}.bin"


def projection_index_map(grid: ScalarGrid, image_size: int):
    key = (grid.precision, image_size)
    cached = _PROJECTION_INDEX_CACHE.get(key)
    if cached is not None:
        return cached

    cache_path = projection_cache_path(grid, image_size)
    if cache_path.exists():
        if np is not None:
            cached = np.fromfile(str(cache_path), dtype=np.int32)
        else:
            cached = array("i")
            with cache_path.open("rb") as handle:
                cached.frombytes(handle.read())
        if len(cached) == image_size * image_size:
            _PROJECTION_INDEX_CACHE[key] = cached
            return cached

    center = image_size * 0.5
    if np is not None:
        indices = np.full(image_size * image_size, -1, dtype=np.int32)
    else:
        indices = array("i", [-1]) * (image_size * image_size)

    for y in range(image_size):
        for x in range(image_size):
            dx = x - center
            dy = y - center
            dist = math.sqrt(dx * dx + dy * dy)
            if dist >= center:
                continue

            theta = math.asin(dist / center)
            phi = math.atan2(dy, dx) + math.pi
            indices[y * image_size + x] = grid.index(phi, theta)

    _CACHE_DIR.mkdir(parents=True, exist_ok=True)
    with cache_path.open("wb") as handle:
        handle.write(indices.tobytes())
    _PROJECTION_INDEX_CACHE[key] = indices
    return indices


def projection_src_offset_map(grid: ScalarGrid, image_size: int) -> array:
    key = (grid.precision, image_size)
    cached = _PROJECTION_SRC_OFFSET_CACHE.get(key)
    if cached is not None:
        return cached

    cache_path = projection_src_offset_cache_path(grid, image_size)
    if cache_path.exists():
        if np is not None:
            cached = np.fromfile(str(cache_path), dtype=np.int32)
        else:
            cached = array("i")
            with cache_path.open("rb") as handle:
                cached.frombytes(handle.read())
        if len(cached) == image_size * image_size:
            _PROJECTION_SRC_OFFSET_CACHE[key] = cached
            return cached

    indices = projection_index_map(grid, image_size)
    if np is not None:
        offsets = np.empty(len(indices), dtype=np.int32)
    else:
        offsets = array("i", [0]) * len(indices)
    for pixel_idx, cell_idx in enumerate(indices):
        offsets[pixel_idx] = -1 if cell_idx < 0 else cell_idx * 3

    _CACHE_DIR.mkdir(parents=True, exist_ok=True)
    with cache_path.open("wb") as handle:
        handle.write(offsets.tobytes())
    _PROJECTION_SRC_OFFSET_CACHE[key] = offsets
    return offsets


def projection_pixels_flat(src_offsets: array, values_flat: array) -> array:
    if np is not None and isinstance(src_offsets, np.ndarray) and isinstance(values_flat, np.ndarray):
        values = values_flat.reshape(-1, 3)
        pixels = np.zeros((len(src_offsets), 3), dtype=np.float32)
        valid = src_offsets >= 0
        if np.any(valid):
            pixels[valid] = values[src_offsets[valid] // 3]
        return pixels.reshape(-1)

    pixels = array("f", [0.0]) * (len(src_offsets) * 3)
    for pixel_idx, src in enumerate(src_offsets):
        if src < 0:
            continue
        dst = pixel_idx * 3
        pixels[dst] = values_flat[src]
        pixels[dst + 1] = values_flat[src + 1]
        pixels[dst + 2] = values_flat[src + 2]
    return pixels


def extract_layer_channels(raw_flat: array,
                           layer_idx: int,
                           width: int,
                           channels: int,
                           header_cells: int,
                           patch_count: int) -> tuple[array, array, array, array]:
    layer_start = layer_idx * width * channels + header_cells * channels
    layer_end = layer_start + patch_count * channels
    if np is not None and isinstance(raw_flat, np.ndarray):
        layer = raw_flat[layer_start:layer_end].reshape(patch_count, channels)
        return layer[:, 0], layer[:, 1], layer[:, 2], layer[:, 3]
    layer = raw_flat[layer_start:layer_end]
    return (
        layer[0::channels],
        layer[1::channels],
        layer[2::channels],
        layer[3::channels],
    )


def normalize_layer_channels(raw_r: array,
                             raw_g: array,
                             raw_b: array,
                             raw_counts: array,
                             patch_area: float,
                             surface_hits: float,
                             normalize: str) -> tuple[array, tuple[float, float, float], float]:
    if np is not None and isinstance(raw_counts, np.ndarray):
        sensor_hits = float(raw_counts.sum(dtype=np.float64))
    else:
        sensor_hits = float(sum(raw_counts))
    if normalize == "sensor":
        denom = sensor_hits * patch_area if sensor_hits > 0.0 else 1.0
    elif normalize == "surface":
        denom = surface_hits * patch_area if surface_hits > 0.0 else 1.0
    elif normalize == "none":
        denom = 1.0
    else:
        raise ValueError("normalize must be one of: sensor, surface, none")

    if np is not None and isinstance(raw_r, np.ndarray):
        inv_denom = np.float32(1.0 / denom)
        normalized = np.empty((len(raw_counts), 3), dtype=np.float32)
        normalized[:, 0] = raw_r * inv_denom
        normalized[:, 1] = raw_g * inv_denom
        normalized[:, 2] = raw_b * inv_denom
        energy_scale = float(inv_denom) * patch_area
        energy = (
            float(raw_r.sum(dtype=np.float64)) * energy_scale,
            float(raw_g.sum(dtype=np.float64)) * energy_scale,
            float(raw_b.sum(dtype=np.float64)) * energy_scale,
        )
        return normalized.reshape(-1), energy, sensor_hits

    inv_denom = 1.0 / denom
    patch_count = len(raw_counts)
    normalized = array("f", [0.0]) * (patch_count * 3)
    for patch_idx in range(patch_count):
        r = raw_r[patch_idx] * inv_denom
        g = raw_g[patch_idx] * inv_denom
        b = raw_b[patch_idx] * inv_denom
        dst = patch_idx * 3
        normalized[dst] = r
        normalized[dst + 1] = g
        normalized[dst + 2] = b

    energy_scale = inv_denom * patch_area
    energy = (
        float(sum(raw_r)) * energy_scale,
        float(sum(raw_g)) * energy_scale,
        float(sum(raw_b)) * energy_scale,
    )
    return normalized, energy, sensor_hits


def render_gonio_bundle_from_obj(obj_path: str,
                                 *,
                                 variant: str = "scalar_rgb",
                                 integrator_type: str = "gtracer_gpu_opt",
                                 sensor_type: str = "gonio_gpu_opt",
                                 emitter_type: str = "directionalsimple_gpu_opt",
                                 sample_count: int = 4096,
                                 samples_per_pass: int = 0,
                                 coalesce: bool | None = None,
                                 include_preview: bool = False,
                                 include_projection: bool = False,
                                 precision: int = 1,
                                 theta_i_deg: float = 0.0,
                                 phi_i_deg: float = 0.0,
                                 merge_l1_l2: bool = False,
                                 record_refraction: bool = False,
                                 normalize: str = "surface",
                                 image_size: int = 512,
                                 seed: int = 0,
                                 bsdf_type: str = "diffuse",
                                 material: str = "Cu",
                                 reflectance: float = 1.0,
                                 alpha: float = 0.1,
                                 int_ior: float = 1.5,
                                 ext_ior: float = 1.0,
                                 max_depth: int = 8,
                                 rr_depth: int = 8,
                                 collect_timing: bool = False) -> dict:
    obj_file = Path(obj_path).resolve()
    if not obj_file.exists():
        raise FileNotFoundError(obj_file)

    if theta_i_deg < 0.0 or theta_i_deg >= 90.0:
        raise ValueError("theta_i_deg must be in [0, 90).")
    if phi_i_deg < 0.0 or phi_i_deg >= 360.0:
        raise ValueError("phi_i_deg must be in [0, 360).")

    mi.set_variant(variant)
    ensure_plugin_search_path()
    grid = build_scalar_grid(precision)

    scene_dict = {
        "type": "scene",
        "integrator": {
            "type": integrator_type,
            "max_depth": max_depth,
            "rr_depth": rr_depth,
        },
        "sensor": {
            "type": sensor_type,
            "precision": precision,
            "merge_l1_l2": merge_l1_l2,
            "record_refraction": record_refraction,
            "sampler": {
                "type": "independent",
                "sample_count": sample_count,
            },
            "film": {
                "type": "hdrfilm",
                "width": 1,
                "height": 1,
                "rfilter": {"type": "box"},
            },
        },
        "emitter": {
            "type": emitter_type,
            "direction": incident_direction(theta_i_deg, phi_i_deg),
            "irradiance": {
                "type": "rgb",
                "value": [1.0, 1.0, 1.0],
            },
        },
        "shape": {
            "type": "obj",
            "filename": str(obj_file),
            "bsdf": make_bsdf_dict(bsdf_type, material, reflectance, alpha, int_ior, ext_ior),
        },
    }
    if samples_per_pass > 0:
        scene_dict["integrator"]["samples_per_pass"] = samples_per_pass
    if coalesce is not None:
        scene_dict["integrator"]["coalesce"] = coalesce

    timings: dict[str, float] = {}
    t0 = time.perf_counter()
    scene = mi.load_dict(scene_dict)
    timings["load_dict_s"] = time.perf_counter() - t0
    sensor = scene.sensors()[0]
    integrator = scene.integrator()

    t0 = time.perf_counter()
    integrator.render(scene, sensor=sensor, seed=seed, develop=False, evaluate=True)
    timings["render_s"] = time.perf_counter() - t0

    t0 = time.perf_counter()
    raw = sensor.film().develop(raw=True)
    timings["develop_raw_s"] = time.perf_counter() - t0

    preview_shape = None
    preview_flat = None
    timings["develop_preview_s"] = 0.0
    if include_preview:
        t0 = time.perf_counter()
        preview = sensor.film().develop()
        timings["develop_preview_s"] = time.perf_counter() - t0
        preview_shape, preview_flat = unpack_tensor(preview)

    t0 = time.perf_counter()
    raw_shape, raw_flat = unpack_tensor(raw)
    height, width, channels = raw_shape
    header_cells = 1
    if width != grid.patch_count + header_cells:
        raise RuntimeError(
            f"Unexpected film width {width}, expected {grid.patch_count + header_cells}"
        )
    if channels < 4:
        raise RuntimeError("Expected raw gonio film to expose at least RGBW channels.")

    t0 = time.perf_counter()
    if channels == 4:
        raw_channel_names = ["R", "G", "B", "W"]
    elif channels == 5:
        raw_channel_names = ["R", "G", "B", "A", "W"]
    else:
        raw_channel_names = [f"C{i}" for i in range(channels)]
    raw_bitmap = mi.Bitmap(raw, mi.Bitmap.PixelFormat.MultiChannel, raw_channel_names)
    timings["bitmap_raw_s"] = time.perf_counter() - t0

    theta_values, phi_values = patch_centers_cached(grid)
    projection_offsets = (
        projection_src_offset_map(grid, image_size) if include_projection else None
    )
    layer_info = layer_descriptors(merge_l1_l2, record_refraction, False)
    surface_hits = float(raw_flat[3]) if header_cells > 0 else float(sample_count)
    timings["decode_setup_s"] = time.perf_counter() - t0

    per_layer_summary = []
    layer_outputs = []
    t_extract = 0.0
    t_normalize = 0.0
    t_project = 0.0
    for layer_idx, (bounce_label, hemi_label) in enumerate(layer_info):
        t1 = time.perf_counter()
        layer_r, layer_g, layer_b, layer_counts = extract_layer_channels(
            raw_flat, layer_idx, width, channels, header_cells, grid.patch_count
        )
        t_extract += time.perf_counter() - t1

        t1 = time.perf_counter()
        normalized, energy, sensor_hits = normalize_layer_channels(
            layer_r, layer_g, layer_b, layer_counts, grid.cell_solid_angle,
            surface_hits, normalize
        )
        t_normalize += time.perf_counter() - t1

        base_name = f"sensor_{bounce_label}_{hemi_label}"
        projection = None
        if include_projection:
            t1 = time.perf_counter()
            projection = projection_pixels_flat(projection_offsets, normalized)
            t_project += time.perf_counter() - t1

        per_layer_summary.append({
            "layer_index": layer_idx,
            "name": base_name,
            "sensor_hits": float(sensor_hits),
            "energy_rgb": list(energy),
        })
        layer_outputs.append({
            "name": base_name,
            "hemi": hemi_label,
            "bounce": bounce_label,
            "data": normalized,
            "energy_rgb": energy,
            "projection_pixels": projection,
        })

    config = {
        "obj_path": str(obj_file),
        "variant": variant,
        "integrator_type": integrator_type,
        "sensor_type": sensor_type,
        "emitter_type": emitter_type,
        "sample_count": sample_count,
        "samples_per_pass": samples_per_pass,
        "precision": precision,
        "theta_i_deg": theta_i_deg,
        "phi_i_deg": phi_i_deg,
        "merge_l1_l2": merge_l1_l2,
        "record_refraction": record_refraction,
        "normalize": normalize,
        "include_projection": include_projection,
        "image_size": image_size,
        "seed": seed,
        "bsdf_type": bsdf_type,
        "material": material,
        "reflectance": reflectance,
        "alpha": alpha,
        "int_ior": int_ior,
        "ext_ior": ext_ior,
        "max_depth": max_depth,
        "rr_depth": rr_depth,
    }
    summary = {
        "patch_count": grid.patch_count,
        "patch_area": grid.cell_solid_angle,
        "surface_hits": float(surface_hits),
        "surface_hit_source": "film_header",
        "layers": per_layer_summary,
    }
    timings["decode_extract_layers_s"] = t_extract
    timings["decode_normalize_s"] = t_normalize
    timings["decode_project_s"] = t_project
    timings["decode_s"] = time.perf_counter() - t0

    total_result = {
        "preview_shape": preview_shape,
        "preview_pixels": preview_flat,
        "raw_bitmap": raw_bitmap,
        "theta_values": theta_values,
        "phi_values": phi_values,
        "layers": layer_outputs,
        "summary": summary,
        "config": config,
        "incident": {
            "phi_i_deg": phi_i_deg,
            "theta_i_deg": theta_i_deg,
            "phi_i_rad": math.radians(phi_i_deg),
            "theta_i_rad": math.radians(theta_i_deg),
        },
    }
    if collect_timing:
        total_result["timings"] = timings
    return total_result


def export_gonio_from_obj(obj_path: str,
                          output_dir: str,
                          **kwargs) -> dict:
    kwargs.pop("write_raw_preview", None)
    kwargs["include_preview"] = False
    kwargs.setdefault("include_projection", False)
    bundle = render_gonio_bundle_from_obj(obj_path, **kwargs)

    output_path = Path(output_dir).resolve()
    imgs_path = output_path / "imgs"
    imgs_path.mkdir(parents=True, exist_ok=True)

    write_sensor_exr(output_path / "raw_sensor.exr", bundle["raw_bitmap"])

    for layer in bundle["layers"]:
        write_rgb_dat(imgs_path / f"{layer['name']}_sensorData.dat", layer["data"])
        write_energy(imgs_path / f"{layer['name']}", layer["energy_rgb"])
        if bundle["config"]["include_projection"] and layer["projection_pixels"] is not None:
            image_size = kwargs.get("image_size", 512)
            write_rgb_exr(imgs_path / f"{layer['name']}.exr",
                          image_size, image_size, layer["projection_pixels"])

    for hemi in sorted({layer["hemi"] for layer in bundle["layers"]}):
        write_float_dat(imgs_path / f"sensor_{hemi}_sensorTheta.dat", bundle["theta_values"])
        write_float_dat(imgs_path / f"sensor_{hemi}_sensorPhi.dat", bundle["phi_values"])

    incident = bundle["incident"]
    with (imgs_path / "sensor_incident_dir.txt").open("w", encoding="utf-8") as handle:
        handle.write(f"Phi-i (deg): {incident['phi_i_deg']}\n")
        handle.write(f"Theta-i (deg): {incident['theta_i_deg']}\n")
        handle.write(f"Phi-i (rad): {incident['phi_i_rad']}\n")
        handle.write(f"Theta-i (rad): {incident['theta_i_rad']}\n")

    with (output_path / "render_config.json").open("w", encoding="utf-8") as handle:
        json.dump(bundle["config"], handle, indent=2)

    with (output_path / "sensor_summary.json").open("w", encoding="utf-8") as handle:
        json.dump(bundle["summary"], handle, indent=2)

    with (output_path / "cooked_sensor_valid_sample_count.txt").open("w", encoding="utf-8") as handle:
        handle.write(f"{bundle['summary']['surface_hits']}\n")

    return bundle["summary"]


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--obj", required=True, help="Input OBJ mesh file.")
    parser.add_argument("--output", required=True, help="Output directory.")
    parser.add_argument("--variant", default="scalar_rgb")
    parser.add_argument("--integrator-type", default="gtracer_gpu_opt")
    parser.add_argument("--sensor-type", default="gonio_gpu_opt")
    parser.add_argument("--emitter-type", default="directionalsimple_gpu_opt")
    parser.add_argument("--samples", type=int, default=4096)
    parser.add_argument("--samples-per-pass", type=int, default=0)
    parser.add_argument("--coalesce", choices=["true", "false"], default=None)
    parser.add_argument("--write-raw-preview", action="store_true")
    parser.add_argument("--include-projection", action="store_true")
    parser.add_argument("--precision", type=int, default=1)
    parser.add_argument("--theta-i", type=float, default=0.0)
    parser.add_argument("--phi-i", type=float, default=0.0)
    parser.add_argument("--merge-l1-l2", action="store_true")
    parser.add_argument("--record-refraction", action="store_true")
    parser.add_argument("--normalize", choices=["sensor", "surface", "none"], default="surface")
    parser.add_argument("--image-size", type=int, default=512)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--bsdf", default="diffuse",
                        choices=["diffuse", "conductor", "roughconductor", "dielectric", "roughdielectric"])
    parser.add_argument("--material", default="Cu")
    parser.add_argument("--reflectance", type=float, default=1.0)
    parser.add_argument("--alpha", type=float, default=0.1)
    parser.add_argument("--int-ior", type=float, default=1.5)
    parser.add_argument("--ext-ior", type=float, default=1.0)
    parser.add_argument("--max-depth", type=int, default=8)
    parser.add_argument("--rr-depth", type=int, default=8)
    return parser


def main() -> None:
    args = _build_parser().parse_args()
    export_gonio_from_obj(
        args.obj,
        args.output,
        variant=args.variant,
        integrator_type=args.integrator_type,
        sensor_type=args.sensor_type,
        emitter_type=args.emitter_type,
        sample_count=args.samples,
        samples_per_pass=args.samples_per_pass,
        coalesce=None if args.coalesce is None else (args.coalesce == "true"),
        write_raw_preview=args.write_raw_preview,
        include_projection=args.include_projection,
        precision=args.precision,
        theta_i_deg=args.theta_i,
        phi_i_deg=args.phi_i,
        merge_l1_l2=args.merge_l1_l2,
        record_refraction=args.record_refraction,
        normalize=args.normalize,
        image_size=args.image_size,
        seed=args.seed,
        bsdf_type=args.bsdf,
        material=args.material,
        reflectance=args.reflectance,
        alpha=args.alpha,
        int_ior=args.int_ior,
        ext_ior=args.ext_ior,
        max_depth=args.max_depth,
        rr_depth=args.rr_depth,
    )


if __name__ == "__main__":
    main()
