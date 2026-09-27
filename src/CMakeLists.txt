# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
list(FILTER app_sources EXCLUDE REGEX "\\.(h|hpp)$")

idf_component_register(SRCS ${app_sources}\n                    REQUIRES bluepad32 btstack)
