# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(swadge2024)

idf_build_set_property(COMPILE_OPTIONS "-finline-functions" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCFG_TUD_HID_EP_BUFSIZE=255" APPEND)

