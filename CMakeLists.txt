cmake_minimum_required(VERSION 3.16.0)

# Bluepad32 is fetched into the project tree by tools/prepare_bluepad32.py.
list(APPEND EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/components/fabgl"
    "${CMAKE_CURRENT_LIST_DIR}/components/pwm_audio"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/bluepad32/src/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESPectrum2)
