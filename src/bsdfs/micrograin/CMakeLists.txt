add_plugin(micrograin_conductor micrograin_conductor.cpp)
add_plugin(micrograin_diffuse   micrograin_diffuse.cpp)
add_plugin(micrograin_plastic   micrograin_plastic.cpp)

add_plugin(poly_micrograin poly_micrograin.cpp)
add_plugin(blend_surf_bulk blend_surf_bulk.cpp)

target_include_directories(micrograin_conductor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(micrograin_diffuse   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(micrograin_plastic   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(MI_PLUGIN_TARGETS "${MI_PLUGIN_TARGETS}" PARENT_SCOPE)