# SPDX-License-Identifier: Apache-2.0

#board_runner_args(jlink "--device=STM32F411VE" "--speed=4000")
board_runner_args(jlink "--device=SMRK100G" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
