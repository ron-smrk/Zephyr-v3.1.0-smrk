# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib_os_cbprintf)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
