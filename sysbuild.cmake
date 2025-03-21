#
# Copyright (c) 2024 Impulse Wellness LLC.
# All rights reserved.
#
# This file is part of IW-TKOS and is provided under a proprietary license.
# 
# You may not copy, modify, or distribute this software without written permission from Impulse Wellness LLC.
# Portions of this file are based on software licensed by Nordic Semiconductor ASA under the Apache-2.0 license.
#
# SPDX-License-Identifier: Proprietary

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "REMOTE_BOARD must be set to a valid board name")
endif()

# Add remote project
ExternalZephyrProject_Add(
    APPLICATION tkos_net
    SOURCE_DIR ${APP_DIR}/tkos_net
    BOARD ${SB_CONFIG_REMOTE_BOARD}
  )
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES tkos_net)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET tkos_net)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION tkos_net CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(sim_dfu tkos_net)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH sim_dfu tkos_net)

# Add custom config & overlay to the child image for debugging; TODO: make a sysbuild option to enable/disable this for easy on-off
message(STATUS "Adding overlay config from ${APP_DIR}/tkos_net/debug.conf to net image")
add_overlay_config(tkos_net ${APP_DIR}/tkos_net/debug_overlay.conf)
message(STATUS "Adding devicetree overlay from ${APP_DIR}/tkos_net/nrf5340dk_nrf5340_cpunet.overlay")
add_overlay_dts(tkos_net ${APP_DIR}/tkos_net/nrf5340dk_nrf5340_cpunet.overlay)
