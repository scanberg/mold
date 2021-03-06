cmake_minimum_required(VERSION 3.5)

project(mold VERSION 0.1.1 LANGUAGES C)

option(MD_UNITTEST "Enable unittest" ON)
option(MD_USE_RELATIVE_PATHS "Use relative paths" OFF)
option(MD_PROFILE_COMPILE_TIMES "Enable compile time profiling" OFF)

include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_EXTENSIONS ON)
endif()

add_subdirectory(src)

if(MD_UNITTEST)
  enable_testing()
  add_subdirectory(unittest)
endif()
