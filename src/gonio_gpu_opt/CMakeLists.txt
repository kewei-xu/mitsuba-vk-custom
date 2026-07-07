set(MI_PLUGIN_PREFIX "gonio_gpu_opt")

add_plugin(gonio_gpu_opt sensors/gonio.cpp)
add_plugin(directionalsimple_gpu_opt emitters/directionalsimple.cpp)
add_plugin(gtracer_gpu_opt integrators/gtracer.cpp)

set(MI_PLUGIN_TARGETS "${MI_PLUGIN_TARGETS}" PARENT_SCOPE)
