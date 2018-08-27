# Declare dependencies
macro (setup_testcase_dependencies)
  link_hifi_libraries(shared test-utils gpu shaders gl vk ${PLATFORM_GL_BACKEND} gpu-vk)
  package_libraries_for_deployment()
  target_vulkan()
endmacro ()

setup_hifi_testcase(Gui)

