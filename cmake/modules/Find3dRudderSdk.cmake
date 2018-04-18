#  Try to find the 3dRudder library
#
#  You must provide a 3DRUDDERSDK_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  3DRUDDERSDK_FOUND - system found 3DRUDDERSDK
#  3DRUDDERSDK_INCLUDE_DIRS - the 3DRUDDERSDK include directory
#  3DRUDDERSDK_LIBRARIES - Link this to use 3DRUDDERSDK SDK
#
#  Created on 17/04/2018 by Nicolas PERRET
#

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("3dRudderSdk")
  

#find_path(3DRUDDERSDK_INCLUDE_DIRS 3dRudderSDK.h PATH_SUFFIXES include HINTS ${3DRUDDERSDK_SEARCH_DIRS})

#if (WIN32)
#
#  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
#      set(ARCH_DIR "x64/Static")
#  else()
#      set(ARCH_DIR "Win32/Static")
#  endif()
#
#  find_library(3DRUDDERSDK_LIBRARY_DEBUG 3dRudderSDK PATH_SUFFIXES "lib/${ARCH_DIR}" HINTS ${3DRUDDERSDK_SEARCH_DIRS})
#  find_library(3DRUDDERSDK_LIBRARY_RELEASE 3dRudderSDK PATH_SUFFIXES "lib/${ARCH_DIR}" HINTS ${3DRUDDERSDK_SEARCH_DIRS})
#  find_path(3DRUDDERSDK_DLL_PATH 3dRudderSDK.dll PATH_SUFFIXES "lib/${ARCH_DIR}" HINTS ${3DRUDDERSDK_SEARCH_DIRS})
#elseif (APPLE)
#  find_library(3DRUDDERSDK_LIBRARY_RELEASE 3dRudderSDK PATH_SUFFIXES "lib/MacOsx" HINTS ${3DRUDDERSDK_SEARCH_DIRS})
#endif ()

include(SelectLibraryConfigurations)
select_library_configurations(3DRUDDERSDK)

#set(3DRUDDERSDK_LIBRARY_DEBUG ${3DRUDDERSDK_LIBRARY})
#set(3DRUDDERSDK_LIBRARY_RELEASE ${3DRUDDERSDK_LIBRARY})
#set(3DRUDDERSDK_LIBRARIES ${3DRUDDERSDK_LIBRARY})

set(3DRUDDERSDK_REQUIREMENTS 3DRUDDERSDK_INCLUDE_DIRS 3DRUDDERSDK_LIBRARIES)
if (WIN32)
#  list(APPEND 3DRUDDERSDK_REQUIREMENTS 3DRUDDERSDK_DLL_PATH)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(3dRudderSdk DEFAULT_MSG ${3DRUDDERSDK_REQUIREMENTS})

if (WIN32)
  #add_paths_to_fixup_libs(${3DRUDDERSDK_DLL_PATH})
endif ()

mark_as_advanced(3DRUDDERSDK_INCLUDE_DIRS 3DRUDDERSDK_LIBRARIES 3DRUDDERSDK_SEARCH_DIRS)
