This directory is a working copy of `src/gonio` for GPU optimization work.

Current policy:

- `src/gonio` remains the stable, validated implementation.
- Changes for GPU performance experiments should be made in `src/gonio_gpu_opt` first.
- The directory is currently wired into `src/CMakeLists.txt` under separate
  plugin names: `gonio_gpu_opt`, `directionalsimple_gpu_opt`, and
  `gtracer_gpu_opt`. This keeps the stable plugins available for A/B tests.
- The optimized exporter writes the raw film as EXR and uses optional NumPy
  arrays plus deterministic `.cache` lookup tables to reduce CPU decode cost.

When the optimized implementation is validated, we can either:

1. replace `src/gonio`, or
2. register this directory under separate experimental plugin names.
