## Scope of this release

This repository contains the implementation of the **polydisperse micrograin BSDF plugin** used in our work.

Please note that the plugin depends on a small but non-standard modification of Mitsuba's rendering core. More precisely, both the BSDF `sample_ex()` and `eval_ex()` interfaces were extended with an additional `Sampler` parameter. This change is required because the diffuse component of the model performs extra sampling of microfacet normal directions, which cannot be expressed through the standard plugin interface alone.

Therefore, the released code should be understood as:

- the plugin implementation itself, and
- a description of the minimal Mitsuba-side interface adaptation required to run it.

The current implementation is **not drop-in compatible** with the unmodified upstream Mitsuba source.

The required Mitsuba-side changes are limited to:
1. extending the BSDF interface declaration in `bsdf.h`, the following methods were extended with an extra `Sampler* sampler` argument:

- `BSDF::sample_ex(...)`
- `BSDF::eval_ex(...)`

2. propagating the sampler argument in integrators such as path tracer `path.cpp` when calling `eval_ex()` and `sample_ex()` must forward the current sampler:

- `bsdf->sample_ex(..., sampler)`
- `bsdf->eval_ex(..., sampler)`

To keep this release focused and lightweight, we only publish the plugin code here, not a full fork of Mitsuba.


<!-- ## Required Mitsuba-side adaptations

The plugin assumes the following interface changes in Mitsuba:

### 1. BSDF interface extension
The following methods were extended with an extra `Sampler* sampler` argument:
- `BSDF::sample_ex(...)`
- `BSDF::eval_ex(...)`

### 2. Integrator-side propagation
In the path tracer (e.g. `path.cpp`), all calls to the above methods must forward the current sampler:
- `bsdf->sample_ex(..., sampler)`
- `bsdf->eval_ex(..., sampler)`

These changes are necessary because the diffuse part of the polydisperse micrograin model performs additional stochastic sampling of microfacet normals inside BSDF evaluation/sampling. -->