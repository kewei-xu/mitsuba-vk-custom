# Gonio GPU-optimized plugin package

This directory contains the migrated gonioreflectometer implementation.
The code is intentionally kept out of Mitsuba's core `render/` headers so the
feature can evolve as a self-contained plugin package.

Implemented layout:

- `common/`: shared grid, layout, and data conventions used by render and export code.
- `sensors/`: Mitsuba sensor plugins.
- `emitters/`: gonio-specific emitters.
- `integrators/`: gonio measurement integrators.
- `io/`: tensor/dat/image export helpers.
- `python/`: Python-side orchestration and post-processing helpers.

Static measurement geometry is built once on the CPU during plugin
initialization, while per-sample classification and accumulation remain
Dr.Jit-compatible. The package is registered under separate `_gpu_opt` plugin
names so it can be compared with `src/gonio`.

See the repository-level `README.md` for usage and `EXPERIMENTAL.md` for the
current validation policy.
