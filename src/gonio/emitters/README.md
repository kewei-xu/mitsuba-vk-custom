# Gonio stable emitters

`directionalsimple.cpp` implements the collimated, delta-direction emitter
used by the gonioreflectometer. It samples the scene AABB and launches
parallel rays along the configured `direction`.
