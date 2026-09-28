# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/*.S
)

idf_component_register(
    SRCS ${app_sources}
    REQUIRES bluepad32 btstack
)

# BTstack is a static archive and btstack_init() is its ESP32 entry point.
# Force the linker to pull the port object even with ESP-IDF 4.4 link ordering.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u btstack_init")
