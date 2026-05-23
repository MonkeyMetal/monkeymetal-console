# MonkeyMetal Console - top-level CMake project
#
# This is the firmware for the MonkeyMetal Console, an open-source 2D
# black-and-white handheld built on the Waveshare ESP32-S3-RLCD-4.2.
#
# See docs/MonkeyMetal-Console-Design.md for the full design spec.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(monkeymetal_console)
