# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(shell_module)

target_sources(app PRIVATE src/main.c src/my2c.c src/lib.c src/led.c src/boardctl.c src/gpio.c)
