# SPDX-License-Identifier: Apache-2.0

if(CONFIG_XTENSA_RESET_VECTOR)
  zephyr_library()

  zephyr_library_cc_option(
    -c
    -mtext-section-literals
    -mlongcalls
    )

  zephyr_library_sources(
    memerror-vector.S
    memctl_default.S
    reset-vector.S
    )
endif()
