# cmake-format: off
#******************************************************************************
#   Copyright 2020 Embedded Office GmbH & Co. KG
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#******************************************************************************

file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage(
  NAME lwrb
  URL  https://github.com/MaJerle/lwrb/archive/refs/tags/v3.2.0.tar.gz
)

#---
# define the canopen stack library
#
add_library(canopen-stack)

#---
# define interface
#
target_include_directories(canopen-stack
  PUBLIC
    config
    core
    hal
    object/basic
    object/cia301
    service/cia301
    service/cia305
    driver/socketcan
    driver/nvm_mmap
    driver/swcycle
)

#---
# specify the implementation files
#
target_sources(canopen-stack
  PRIVATE
    # config interface
    config/callbacks.c

    # core API
    core/co_core.c
    core/co_dict.c
    core/co_nmt.c
    core/co_obj.c
    core/co_tmr.c
    core/co_ver.c
    # hardware abstraction
    hal/co_if.c
    hal/co_if_can.c
    hal/co_if_nvm.c
    hal/co_if_timer.c
    # driver
    driver/socketcan/drv_can_socketcan.c
    driver/nvm_mmap/drv_nvm_mmap.c
    driver/swcycle/drv_timer_swcycle.c

    # object type functions
    # - basic types
    object/basic/co_domain.c
    object/basic/co_string.c
    object/basic/co_integer8.c
    object/basic/co_integer16.c
    object/basic/co_integer32.c
    # - CiA301 types
    object/cia301/co_emcy_hist.c
    object/cia301/co_emcy_id.c
    object/cia301/co_hb_cons.c
    object/cia301/co_hb_prod.c
    object/cia301/co_para_store.c
    object/cia301/co_para_restore.c
    object/cia301/co_pdo_event.c
    object/cia301/co_pdo_id.c
    object/cia301/co_pdo_map.c
    object/cia301/co_pdo_num.c
    object/cia301/co_pdo_type.c
    object/cia301/co_sdo_id.c
    object/cia301/co_sync_cycle.c
    object/cia301/co_sync_id.c

    # network services
    # - CiA301
    service/cia301/co_csdo.c
    service/cia301/co_emcy.c
    service/cia301/co_pdo.c
    service/cia301/co_ssdo.c
    service/cia301/co_sync.c
    # - CiA305
    service/cia305/co_lss.c
)

target_link_libraries(canopen-stack PUBLIC lwrb)
target_compile_options(canopen-stack PRIVATE "-std=c2x")

# cmake-format: on
add_library(
  canopen-stack-master
  master/co_nmt_master.c master/co_master.cpp master/od_util.cpp
  master/sche_util.cpp master/co_size_only.c)
target_compile_options(canopen-stack-master PRIVATE "-std=c++17")
target_link_libraries(canopen-stack-master PUBLIC canopen-stack)
target_include_directories(canopen-stack-master PUBLIC master)

if(NOT BUILD_TESTING STREQUAL OFF)
  add_subdirectory(master/tests)
endif()
