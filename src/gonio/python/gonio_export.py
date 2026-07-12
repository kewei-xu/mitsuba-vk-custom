"""Run a gonio scene and convert Mitsuba's film into research-file outputs.

The C++ plugins produce a one-dimensional angular measurement layout. This
module reconstructs the same grid in Python, decodes the raw film, applies an
explicit normalization policy, and writes the legacy ``.dat``/BMP bundle.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import drjit as dr
import mitsuba as mi


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
        """Map a spherical direction to the equal-area patch index."""
        phi = phi % (2.0 * math.pi)
        theta = max(0.0, min(theta, 0.5 * math.pi))

        ring_id = 0
        while ring_id + 1 < len(self.rings) and theta >= self.rings[ring_id].theta_max:
            ring_id += 1

        ring = self.rings[ring_id]
        patch = min(int(math.floor(phi / ring.phi_step)), ring.patch_count - 1)
        return ring.base_index + patch

    def patch_centers(self) -> tuple[list[float], list[float]]:
        """Return one representative (theta, phi) pair for every patch."""
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
    """Rebuild the C++ ring construction exactly for Python-side decoding."""
    patch_count = expected_patch_count(precision)
    cap = theta_cap(precision)
    if patch_count == 0 or cap == 0.0:
        raise ValueError("precision must be in [1, 4]")

    rings = [ScalarRing(0.0, cap, 2.0 * math.pi, 0, 1)]
    theta_p = cap
    radius_p = 2.0 * math.sin(0.5 * theta_p)
    k_p = 1

    # The stereographic radius estimates the next ring population. The
    # spherical-area equation below corrects that estimate to equal area.
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

    return ScalarGrid(
        precision=precision,
        patch_count=patch_count,
        theta_cap=cap,
        cell_solid_angle=2.0 * math.pi * (1.0 - math.cos(cap)),
        rings=rings,
    )


def incident_direction(theta_i_deg: float, phi_i_deg: float) -> tuple[float, float, float]:
    """Return the travel direction of the collimated incident light."""
    theta = math.radians(theta_i_deg)
    phi = math.radians(phi_i_deg)
    return (
        -math.sin(theta) * math.cos(phi),
        -math.sin(theta) * math.sin(phi),
        -math.cos(theta),
    )


def layer_descriptors(merge_l1_l2: bool, record_refraction: bool, analytic_measurement: bool) -> list[tuple[str, str]]:
    """Describe the film rows in the same order used by GonioSensor."""
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


def unpack_tensor(tensor) -> tuple[tuple[int, ...], list[float]]:
    shape = tuple(tensor.shape)
    return shape, list(dr.ravel(tensor))


def write_float_dat(path: Path, values: Iterable[float]) -> None:
    values = list(values)
    with path.open("wb") as handle:
        handle.write(struct.pack(f"<{len(values)}f", *values))


def write_rgb_dat(path: Path, values: list[tuple[float, float, float]]) -> None:
    flat: list[float] = []
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
    max_value = max(rgb_flat) if rgb_flat else 0.0
    scale = 1.0 / max_value if max_value > 0.0 else 1.0

    row_stride = width * 3
    padding = (4 - (row_stride % 4)) % 4
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


def projection_pixels(grid: ScalarGrid,
                      values: list[tuple[float, float, float]],
                      image_size: int) -> list[float]:
    center = image_size * 0.5
    pixels = [0.0] * (image_size * image_size * 3)

    for y in range(image_size):
        for x in range(image_size):
            dx = x - center
            dy = y - center
            dist = math.sqrt(dx * dx + dy * dy)
            if dist >= center:
                continue

            theta = math.asin(dist / center)
            phi = math.atan2(dy, dx) + math.pi
            idx = grid.index(phi, theta)
            rgb = values[idx]
            base = (y * image_size + x) * 3
            pixels[base:base + 3] = [rgb[0], rgb[1], rgb[2]]

    return pixels


def normalize_layer(raw_values: list[tuple[float, float, float]],
                    raw_counts: list[float],
                    patch_area: float,
                    surface_hits: float,
                    normalize: str) -> tuple[list[tuple[float, float, float]], tuple[float, float, float], float]:
    # The raw film stores accumulated radiance and a per-cell sample count.
    # Dividing by patch solid angle converts the cell sum to a density; the
    # selected hit count controls whether the result is sensor- or surface-
    # normalized.
    sensor_hits = float(sum(raw_counts))
    if normalize == "sensor":
        denom = sensor_hits * patch_area if sensor_hits > 0.0 else 1.0
    elif normalize == "surface":
        denom = surface_hits * patch_area if surface_hits > 0.0 else 1.0
    elif normalize == "none":
        denom = 1.0
    else:
        raise ValueError("normalize must be one of: sensor, surface, none")

    normalized: list[tuple[float, float, float]] = []
    sum_r = sum_g = sum_b = 0.0
    for rgb in raw_values:
        value = (rgb[0] / denom, rgb[1] / denom, rgb[2] / denom)
        normalized.append(value)
        sum_r += value[0]
        sum_g += value[1]
        sum_b += value[2]

    energy = (
        sum_r * patch_area,
        sum_g * patch_area,
        sum_b * patch_area,
    )
    return normalized, energy, sensor_hits


def render_gonio_bundle_from_obj(obj_path: str,
                                 *,
                                 variant: str = "scalar_rgb",
                                 sample_count: int = 4096,
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
                                 rr_depth: int = 8) -> dict:
    obj_file = Path(obj_path).resolve()
    if not obj_file.exists():
        raise FileNotFoundError(obj_file)

    if theta_i_deg < 0.0 or theta_i_deg >= 90.0:
        raise ValueError("theta_i_deg must be in [0, 90).")
    if phi_i_deg < 0.0 or phi_i_deg >= 360.0:
        raise ValueError("phi_i_deg must be in [0, 360).")

    # Keep scene construction here so the command-line and Python APIs use
    # precisely the same plugin names and measurement options.
    mi.set_variant(variant)
    grid = build_scalar_grid(precision)

    scene_dict = {
        "type": "scene",
        "integrator": {
            "type": "gtracer",
            "max_depth": max_depth,
            "rr_depth": rr_depth,
        },
        "sensor": {
            "type": "gonio",
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
            "type": "directionalsimple",
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

    scene = mi.load_dict(scene_dict)
    sensor = scene.sensors()[0]
    integrator = scene.integrator()

    # develop=False avoids an unnecessary preview conversion during the
    # measurement pass; the raw film is the authoritative data source.
    integrator.render(scene, sensor=sensor, seed=seed, develop=False, evaluate=True)

    preview = sensor.film().develop()
    raw = sensor.film().develop(raw=True)

    preview_shape, preview_flat = unpack_tensor(preview)

    raw_shape, raw_flat = unpack_tensor(raw)
    height, width, channels = raw_shape
    header_cells = 1
    if width != grid.patch_count + header_cells:
        raise RuntimeError(
            f"Unexpected film width {width}, expected {grid.patch_count + header_cells}"
        )
    if channels < 4:
        raise RuntimeError("Expected raw gonio film to expose at least RGBW channels.")

    theta_values, phi_values = grid.patch_centers()
    layer_info = layer_descriptors(merge_l1_l2, record_refraction, False)
    surface_hits = raw_flat[3] if header_cells > 0 else float(sample_count)

    per_layer_summary = []
    layer_outputs = []
    # The first film column is a header counter in non-analytic mode, so the
    # exported angular arrays start at x=1.
    for layer_idx, (bounce_label, hemi_label) in enumerate(layer_info):
        layer_rgb: list[tuple[float, float, float]] = []
        layer_counts: list[float] = []
        for x in range(header_cells, width):
            base = (layer_idx * width + x) * channels
            layer_rgb.append(
                (raw_flat[base], raw_flat[base + 1], raw_flat[base + 2])
            )
            layer_counts.append(raw_flat[base + 3])

        normalized, energy, sensor_hits = normalize_layer(
            layer_rgb, layer_counts, grid.cell_solid_angle, surface_hits, normalize
        )

        base_name = f"sensor_{bounce_label}_{hemi_label}"
        projection = projection_pixels(grid, normalized, image_size)

        per_layer_summary.append({
            "layer_index": layer_idx,
            "name": base_name,
            "sensor_hits": sensor_hits,
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
        "sample_count": sample_count,
        "precision": precision,
        "theta_i_deg": theta_i_deg,
        "phi_i_deg": phi_i_deg,
        "merge_l1_l2": merge_l1_l2,
        "record_refraction": record_refraction,
        "normalize": normalize,
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
        "surface_hits": surface_hits,
        "layers": per_layer_summary,
    }
    return {
        "preview_shape": preview_shape,
        "preview_pixels": preview_flat,
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


def export_gonio_from_obj(obj_path: str,
                          output_dir: str,
                          **kwargs) -> dict:
    # Rendering and serialization are intentionally separate: callers that
    # need in-memory arrays can use render_gonio_bundle_from_obj directly.
    bundle = render_gonio_bundle_from_obj(obj_path, **kwargs)

    output_path = Path(output_dir).resolve()
    imgs_path = output_path / "imgs"
    imgs_path.mkdir(parents=True, exist_ok=True)

    write_rgb_bmp(output_path / "raw_sensor.bmp",
                  bundle["preview_shape"][1], bundle["preview_shape"][0],
                  bundle["preview_pixels"])

    image_size = kwargs.get("image_size", 512)
    for layer in bundle["layers"]:
        write_rgb_dat(imgs_path / f"{layer['name']}_sensorData.dat", layer["data"])
        write_energy(imgs_path / f"{layer['name']}", layer["energy_rgb"])
        write_rgb_bmp(imgs_path / f"{layer['name']}.bmp",
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
    parser.add_argument("--samples", type=int, default=4096)
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
        sample_count=args.samples,
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
