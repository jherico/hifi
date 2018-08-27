set(EXTERNAL_NAME vma)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://public.highfidelity.com/dependencies/vulkan/VulkanMemoryAllocator-2.1.0.zip
  URL_MD5 3744f2127bf654a155681b7c4a698281
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${SOURCE_DIR}/src CACHE FILEPATH "Vulkan Memory Allocator")

