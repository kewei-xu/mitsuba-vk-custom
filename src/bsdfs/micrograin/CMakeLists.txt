set(MI_PLUGIN_PREFIX "bsdfs")

add_plugin(micrograin_conductor micrograin_conductor.cpp)
add_plugin(micrograin_diffuse   micrograin_diffuse.cpp)
add_plugin(micrograin_plastic   micrograin_plastic.cpp)
add_plugin(poly_micrograin poly_micrograin.cpp)
add_plugin(blend_poly_surf_bulk blend_poly_surf_bulk.cpp)
add_plugin(blend_surf_bulk blend_surf_bulk.cpp)
add_plugin(coated_micrograin coated_micrograin.cpp)

set(MI_PLUGIN_TARGETS "${MI_PLUGIN_TARGETS}" PARENT_SCOPE)
