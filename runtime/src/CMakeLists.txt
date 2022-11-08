# Gather our source files in this directory.
set(
  gazprea_rt_files
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeTypes.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeTypes.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeVariables.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeVariables.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeErrors.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeErrors.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/NDArray.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/NDArray.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Enums.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Enums.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/FreeList.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/FreeList.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Literal.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Literal.h"
)

# Build our executable from the source files.
add_library(gazrt SHARED ${gazprea_rt_files})
target_include_directories(gazc PUBLIC ${RUNTIME_INCLUDE})

# Symbolic link our library to the base directory so we don't have to go searching for it.
symlink_to_bin("gazrt")
