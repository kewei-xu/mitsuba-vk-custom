from __future__ import annotations

import argparse
import json

from gonio_gpu_opt.python.gonio_export import render_gonio_bundle_from_obj


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--obj", required=True)
    parser.add_argument("--variant", default="cuda_rgb")
    parser.add_argument("--integrator-type", default="gtracer_gpu_opt")
    parser.add_argument("--sensor-type", default="gonio_gpu_opt")
    parser.add_argument("--emitter-type", default="directionalsimple_gpu_opt")
    parser.add_argument("--samples", type=int, default=1000000)
    parser.add_argument("--samples-per-pass", type=int, default=0)
    parser.add_argument("--coalesce", choices=["true", "false"], default=None)
    parser.add_argument("--precision", type=int, default=4)
    parser.add_argument("--include-projection", action="store_true")
    parser.add_argument("--theta-i", type=float, default=0.0)
    parser.add_argument("--phi-i", type=float, default=0.0)
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
    return parser


def main() -> None:
    args = _build_parser().parse_args()
    bundle = render_gonio_bundle_from_obj(
        args.obj,
        variant=args.variant,
        integrator_type=args.integrator_type,
        sensor_type=args.sensor_type,
        emitter_type=args.emitter_type,
        sample_count=args.samples,
        samples_per_pass=args.samples_per_pass,
        coalesce=None if args.coalesce is None else (args.coalesce == "true"),
        include_projection=args.include_projection,
        precision=args.precision,
        theta_i_deg=args.theta_i,
        phi_i_deg=args.phi_i,
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
        collect_timing=True,
    )

    result = {
        "config": bundle["config"],
        "timings": bundle["timings"],
        "summary": bundle["summary"],
    }
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
