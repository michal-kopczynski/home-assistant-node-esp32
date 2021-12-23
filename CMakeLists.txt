# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CONF_FILE prj.conf )

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(home-assistant-node-esp32)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
