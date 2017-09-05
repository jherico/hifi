#
#  FindBinaryVR.cmake
#
#  Try to find the BINARYVR controller library
#
#  This module defines the following variables
#
#  BINARYVR_FOUND - Was BINARYVR found
#  BINARYVR_INCLUDE_DIRS - the BINARYVR include directory
#  BINARYVR_LIBRARIES - Link this to use BINARYVR
#
#  This module accepts the following variables
#
#  BINARYVR_ROOT - Can be set to BINARYVR install path or Windows build path
#
#  Created on 6/8/2016 by Clement Brisset
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include(SelectLibraryConfigurations)
select_library_configurations(BINARYVR)

set(BINARYVR_REQUIREMENTS BINARYVR_INCLUDE_DIRS BINARYVR_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BINARYVR DEFAULT_MSG BINARYVR_INCLUDE_DIRS BINARYVR_LIBRARIES)
mark_as_advanced(BINARYVR_LIBRARIES BINARYVR_INCLUDE_DIRS BINARYVR_SEARCH_DIRS)
