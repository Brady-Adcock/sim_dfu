# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
message(WARNING "Zephyr base: $ENV{ZEPHYR_BASE}")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
if (NOT SYSBUILD)
	message(WARNING
		" This is a multi-image application that should be built using sysbuild.\n"
		" Add --sysbuild argument to west build command to prepare all the images.")
endif()


project(sim_dfu)

target_sources(app PRIVATE src/main.c)
