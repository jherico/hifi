# 
#  FindaAsimp.cmake
# 
#  Try to find the assimp library

#  Once done this will define
#
#  ASSIMP_FOUND - system found assimp
#  ASSIMP_INCLUDE_DIRS - the assimp include directory
#  ASSIMP_LIBRARIES - Link this to use assimp
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

mark_as_advanced(ASSIMP_INCLUDE_DIRS ASSIMP_LIBRARIES ASSIMP_SEARCH_DIRS)

include(SelectLibraryConfigurations)
select_library_configurations(ASSIMP)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASSIMP DEFAULT_MSG ASSIMP_INCLUDE_DIRS ASSIMP_LIBRARIES)
