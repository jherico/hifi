
set(TARGET_NAME gpu-frame-player)
 
setup_memory_debugger()
setup_hifi_project(Gui Widgets)


# link in the shared libraries
link_hifi_libraries(shared ktx shaders gpu)


if (USE_GL)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_GL)
    link_hifi_libraries(gl ${PLATFORM_GL_BACKEND})
    target_opengl()
else()
    link_hifi_libraries(vk gpu-vk)
    target_vulkan()
endif()

package_libraries_for_deployment()
