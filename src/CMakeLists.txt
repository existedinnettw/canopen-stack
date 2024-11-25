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
  URL  https://github.com/existedinnettw/lwrb/archive/refs/heads/hotfix/cmake.tar.gz
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
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hal>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/object/basic>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/object/cia301>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/service/cia301>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/service/cia305>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/driver/socketcan>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/driver/nvm_mmap>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/driver/swcycle>
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
set_target_properties(
  canopen-stack
  PROPERTIES
    PUBLIC_HEADER
    "${CMAKE_CURRENT_LIST_DIR}/config/co_cfg.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia305/co_lss.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_time.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_csdo.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_sync.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_pdo.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_sdo.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_ssdo.h;\
${CMAKE_CURRENT_LIST_DIR}/service/cia301/co_emcy.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_tmr.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_ver.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_nmt.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_err.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_dict.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_obj.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_types.h;\
${CMAKE_CURRENT_LIST_DIR}/core/co_core.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/_template/drv_can_dummy.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/_template/drv_timer_dummy.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/_template/drv_nvm_dummy.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/nvm_mmap/drv_nvm_mmap.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/swcycle/drv_timer_swcycle.h;\
${CMAKE_CURRENT_LIST_DIR}/driver/socketcan/drv_can_socketcan.h;\
${CMAKE_CURRENT_LIST_DIR}/hal/co_if_timer.h;\
${CMAKE_CURRENT_LIST_DIR}/hal/co_if_can.h;\
${CMAKE_CURRENT_LIST_DIR}/hal/co_if.h;\
${CMAKE_CURRENT_LIST_DIR}/hal/co_if_nvm.h;\
${CMAKE_CURRENT_LIST_DIR}/object/basic/co_integer8.h;\
${CMAKE_CURRENT_LIST_DIR}/object/basic/co_integer32.h;\
${CMAKE_CURRENT_LIST_DIR}/object/basic/co_string.h;\
${CMAKE_CURRENT_LIST_DIR}/object/basic/co_integer16.h;\
${CMAKE_CURRENT_LIST_DIR}/object/basic/co_domain.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_emcy_hist.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_para_store.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_para_restore.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_pdo_num.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_hb_cons.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_sync_cycle.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_pdo_event.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_emcy_id.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_pdo_map.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_sync_id.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_pdo_type.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_sdo_id.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_para.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_pdo_id.h;\
${CMAKE_CURRENT_LIST_DIR}/object/cia301/co_hb_prod.h;")

install(TARGETS canopen-stack EXPORT canopen-stack-Config)

install(
  EXPORT canopen-stack-Config
  NAMESPACE canopen-stack::
  FILE canopen-stack-Config.cmake
  DESTINATION lib/cmake/canopen-stack)

add_library(
  canopen-stack-master
  master/co_nmt_master.c master/co_master.cpp master/od_util.cpp
  master/sche_util.cpp master/co_size_only.c)
target_compile_options(canopen-stack-master PRIVATE "-std=c++17")
target_link_libraries(canopen-stack-master PUBLIC canopen-stack)
target_include_directories(
  canopen-stack-master
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/master>)
set_target_properties(
  canopen-stack-master
  PROPERTIES
    PUBLIC_HEADER
    "${CMAKE_CURRENT_LIST_DIR}/master/co_nmt_master.h;\
${CMAKE_CURRENT_LIST_DIR}/master/co_size_only.h;\
${CMAKE_CURRENT_LIST_DIR}/master/od_util.hpp;\
${CMAKE_CURRENT_LIST_DIR}/master/co_master.hpp;\
${CMAKE_CURRENT_LIST_DIR}/master/sche_util.hpp;")
install(TARGETS canopen-stack-master EXPORT canopen-stack-master-Config)
install(
  EXPORT canopen-stack-master-Config
  NAMESPACE canopen-stack::
  FILE canopen-stack-master-Config.cmake
  DESTINATION lib/cmake/canopen-stack)

if(NOT BUILD_TESTING STREQUAL OFF)
  add_subdirectory(master/tests)
endif()
