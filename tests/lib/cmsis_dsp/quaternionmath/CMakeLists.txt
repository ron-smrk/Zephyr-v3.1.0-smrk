# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmsis_dsp_quaternionmath)

target_sources(app PRIVATE
  src/f32.c
  src/main.c
  )
