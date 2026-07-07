set(MI_PLUGIN_PREFIX "gonio")

add_plugin(gonio sensors/gonio.cpp)
add_plugin(directionalsimple emitters/directionalsimple.cpp)
add_plugin(gtracer integrators/gtracer.cpp)

set(MI_PLUGIN_TARGETS "${MI_PLUGIN_TARGETS}" PARENT_SCOPE)
