set(EXTERNAL_NAME spirv_binaries)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)

if (WIN32)
  set(DOWNLOAD_URL https://public.highfidelity.com/dependencies/vulkan/vulkansdk-win32-1.1.82.1.tar.gz)
  set(DOWNLOAD_MD5 3a83ef490bce248b1a4d6726a3e5893e)
elseif (APPLE)
  set(DOWNLOAD_URL https://public.highfidelity.com/dependencies/vulkan/vulkansdk-macos-1.1.82.1.tar.gz)
  set(DOWNLOAD_MD5 a57d37275b2c5db023ba8e84a63461ff)
else ()
  set(DOWNLOAD_URL https://public.highfidelity.com/dependencies/vulkan/vulkansdk-linux-x86_64-1.1.82.1.tar.gz)
  set(DOWNLOAD_MD5 5a7c9eeda8cee6b36724da7f7cbe5ec6)
endif ()

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL ${DOWNLOAD_URL}
  URL_MD5 ${DOWNLOAD_MD5}
  BUILD_COMMAND ""
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")
ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

if(APPLE)
  set(BIN_DIR "${SOURCE_DIR}/macOS/bin")
elseif(WIN32)
  set(BIN_DIR "${SOURCE_DIR}/Bin")
else()
  set(BIN_DIR "${SOURCE_DIR}/x86_64/bin")
endif()

set(${EXTERNAL_NAME_UPPER}_DIR ${BIN_DIR} CACHE FILEPATH "SPIRV binary tools location")

