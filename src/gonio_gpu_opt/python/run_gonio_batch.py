from __future__ import annotations

import argparse
import json
import time
from pathlib import Path

from gonio_gpu_opt.python.gonio_export import export_gonio_from_obj


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--obj", required=True, help="Input OBJ mesh file.")
    parser.add_argument("--output-root", required=True, help="Root directory for batch outputs.")
    parser.add_argument("--variants", nargs="+", required=True)
    parser.add_argument("--integrator-type", default="gtracer_gpu_opt")
    parser.add_argument("--sensor-type", default="gonio_gpu_opt")
    parser.add_argument("--emitter-type", default="directionalsimple_gpu_opt")
    parser.add_argument("--thetas", nargs="+", type=float, required=True)
    parser.add_argument("--phi", type=float, default=0.0)
    parser.add_argument("--samples", type=int, required=True)
    parser.add_argument("--samples-per-pass", type=int, default=0)
    parser.add_argument("--coalesce", choices=["true", "false"], default=None)
    parser.add_argument("--write-raw-preview", action="store_true")
    parser.add_argument("--include-projection", action="store_true")
    parser.add_argument("--precision", type=int, required=True)
    parser.add_argument("--image-size", type=int, default=512)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--bsdf", default="conductor",
                        choices=["diffuse", "conductor", "roughconductor", "dielectric", "roughdielectric"])
    parser.add_argument("--material", default="Au")
    parser.add_argument("--reflectance", type=float, default=1.0)
    parser.add_argument("--alpha", type=float, default=0.1)
    parser.add_argument("--int-ior", type=float, default=1.5)
    parser.add_argument("--ext-ior", type=float, default=1.0)
    parser.add_argument("--max-depth", type=int, default=8)
    parser.add_argument("--rr-depth", type=int, default=8)
    parser.add_argument("--normalize", choices=["sensor", "surface", "none"], default="surface")
    parser.add_argument("--merge-l1-l2", action="store_true")
    parser.add_argument("--record-refraction", action="store_true")
    return parser


def main() -> None:
    args = _build_parser().parse_args()

    output_root = Path(args.output_root).resolve()
    output_root.mkdir(parents=True, exist_ok=True)

    batch_summary: list[dict] = []

    for variant in args.variants:
        for theta in args.thetas:
            run_name = f"{variant}_theta_{str(theta).replace('.', '_')}"
            run_dir = output_root / run_name

            start = time.perf_counter()
            summary = export_gonio_from_obj(
                args.obj,
                str(run_dir),
                variant=variant,
                integrator_type=args.integrator_type,
                sensor_type=args.sensor_type,
                emitter_type=args.emitter_type,
                sample_count=args.samples,
                samples_per_pass=args.samples_per_pass,
                coalesce=None if args.coalesce is None else (args.coalesce == "true"),
                write_raw_preview=args.write_raw_preview,
                include_projection=args.include_projection,
                precision=args.precision,
                theta_i_deg=theta,
                phi_i_deg=args.phi,
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
            elapsed_seconds = time.perf_counter() - start

            timing_record = {
                "variant": variant,
                "theta_i_deg": theta,
                "phi_i_deg": args.phi,
                "elapsed_seconds": elapsed_seconds,
                "output_dir": str(run_dir),
                "surface_hits": summary["surface_hits"],
            }
            batch_summary.append(timing_record)

            with (run_dir / "timing.json").open("w", encoding="utf-8") as handle:
                json.dump(timing_record, handle, indent=2)

    with (output_root / "batch_timing_summary.json").open("w", encoding="utf-8") as handle:
        json.dump(batch_summary, handle, indent=2)


if __name__ == "__main__":
    main()
