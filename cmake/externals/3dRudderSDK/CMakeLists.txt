include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME 3dRudderSDK)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/3DRudder/3dRudderSDK/archive/1.01.zip
  URL_MD5 97c48c9783d984c6ceb8a31d0011eefc
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "hidden/externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/Include CACHE TYPE INTERNAL)

if (WIN32)

  # FIXME need to account for different architectures
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/x64/static/3dRudderSDK.lib CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs(${SOURCE_DIR}/bin/x64)
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win32/static/3dRudderSDK.lib CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)
  endif()

elseif(APPLE)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/MacOsx/lib3DRudderSDK.a CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/MacOsx)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux64/libopenvr_api.so CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux64)
  
endif()

