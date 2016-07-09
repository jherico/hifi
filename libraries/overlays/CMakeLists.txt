set(TARGET_NAME overlays)
AUTOSCRIBE_SHADER_LIB(gpu model procedural render render-utils)
setup_hifi_library(Widgets Network Script)
link_hifi_libraries(shared gpu ui model model-networking script-engine render render-utils networking fbx animation)

