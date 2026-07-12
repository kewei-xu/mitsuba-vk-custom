# Gonio stable plugin package

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

The stable package favors readability and scalar/reference validation. See the
repository-level `README.md` for command-line usage and `DEVELOPMENT_LOG.md`
for the migration history.
