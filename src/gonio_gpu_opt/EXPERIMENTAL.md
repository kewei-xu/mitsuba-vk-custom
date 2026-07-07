This directory is a working copy of `src/gonio` for GPU optimization work.

Current policy:

- `src/gonio` remains the stable, validated implementation.
- Changes for GPU performance experiments should be made in `src/gonio_gpu_opt` first.
- This directory is not wired into the build yet, so the existing `gonio`,
  `directionalsimple`, and `gtracer` plugins remain unchanged.

When the optimized implementation is validated, we can either:

1. replace `src/gonio`, or
2. register this directory under separate experimental plugin names.
