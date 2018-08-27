

set(TARGET_NAME vk-test)
# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Quick Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")
link_hifi_libraries(shared vk)
package_libraries_for_deployment()


list(APPEND SHADERS "${PROJECT_SOURCE_DIR}/src/cube.frag")
compile_spirv_shader("${PROJECT_SOURCE_DIR}/src/cube.frag")
list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
list(APPEND SHADERS "${PROJECT_SOURCE_DIR}/src/cube.vert")
compile_spirv_shader("${PROJECT_SOURCE_DIR}/src/cube.vert")
list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
add_custom_target(vkshaders SOURCES ${SHADERS} ${COMPILED_SHADERS})
add_dependencies(vk-test vkshaders)
target_opengl()
target_vulkan()