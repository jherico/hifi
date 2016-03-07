set(TARGET_NAME model-io)
setup_hifi_library()
link_hifi_libraries(shared gpu model networking octree)

target_assimp()