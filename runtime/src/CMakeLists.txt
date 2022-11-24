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
  "${CMAKE_CURRENT_SOURCE_DIR}/VariableStdio.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/VariableStdio.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/Bool.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/NDArrayVariable.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/NDArrayVariable.h"
)

# Build our executable from the source files.
add_library(gazrt SHARED ${gazprea_rt_files})
target_include_directories(gazc PUBLIC ${RUNTIME_INCLUDE})

# author usr1234567 edited by Sled
# Stackoverflow url:https://stackoverflow.com/questions/34625627/how-to-link-to-the-c-math-library-with-cmake
target_link_libraries(gazrt m)

# Symbolic link our library to the base directory so we don't have to go searching for it.
symlink_to_bin("gazrt")
