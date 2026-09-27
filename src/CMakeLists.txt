# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/*.S
)
list(FILTER app_sources EXCLUDE REGEX "/miniz/")

idf_component_register(
    SRCS ${app_sources}
    REQUIRES bluepad32 btstack
)
