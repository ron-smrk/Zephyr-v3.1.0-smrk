# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(getchar)

target_sources(app PRIVATE src/commands.c src/console.c src/shell.c src/lib.c src/i2c.c src/gpio.c src/boardctl.c src/pmbus.c src/pmbus_cmds.c src/vrails.c src/irps5401.c src/led.c src/history.c) 
