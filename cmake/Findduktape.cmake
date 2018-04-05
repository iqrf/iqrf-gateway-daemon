# - Find duktape
# Find the duktape includes and library from vcpkg
# It's temporary solution on Win, written issue:
# https://github.com/Microsoft/vcpkg/issues/3175
#  duktape_INCLUDE_DIR - Where to find duktape includes
#  duktape_LIBRARIES   - List of libraries when using duktape
#  duktape_FOUND       - True if duktape was found

IF(duktape_INCLUDE_DIR)
  SET(duktape_FIND_QUIETLY TRUE)
ENDIF(duktape_INCLUDE_DIR)

FIND_PATH(duktape_INCLUDE_DIR "duktape.h"
  PATHS
  c:/devel/vcpkg/installed/x64-windows/include
  DOC "duktape - Headers"
)

SET(duktape_NAMES duktape.lib)

FIND_LIBRARY(duktape_LIBRARY NAMES ${duktape_NAMES}
  PATHS
  c:/devel/vcpkg/installed/x64-windows/lib
  PATH_SUFFIXES lib lib64
  DOC "duktape - Library"
)

INCLUDE(FindPackageHandleStandardArgs)

SET(duktape_LIBRARIES ${duktape_LIBRARY})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(duktape DEFAULT_MSG duktape_LIBRARY duktape_INCLUDE_DIR)
  
MARK_AS_ADVANCED(duktape_LIBRARY duktape_INCLUDE_DIR)
  
IF(duktape_FOUND)
  SET(duktape_INCLUDE_DIRS ${duktape_INCLUDE_DIR})
ENDIF(duktape_FOUND)