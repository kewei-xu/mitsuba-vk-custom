# Gonio plugin workspace

This directory contains the migrated gonioreflectometer implementation.
The code is intentionally kept out of Mitsuba's core `render/` headers so the
feature can evolve as a self-contained plugin package.

Planned layout:

- `common/`: shared grid, layout, and data conventions used by render and export code.
- `sensors/`: Mitsuba sensor plugins.
- `emitters/`: gonio-specific emitters.
- `integrators/`: gonio measurement integrators.
- `io/`: tensor/dat/image export helpers.
- `python/`: Python-side orchestration and post-processing helpers.

The design is GPU-first: static measurement geometry is built once on the CPU
during plugin initialization, while per-sample classification and accumulation
must remain Dr.Jit-compatible.
