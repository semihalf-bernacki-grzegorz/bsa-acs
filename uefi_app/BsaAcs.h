/** @file
 * Copyright (c) 2016-2020, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#ifndef __BSA_ACS_LEVEL_H__
#define __BSA_ACS_LEVEL_H__



  #define BSA_ACS_MAJOR_VER  1
  #define BSA_ACS_MINOR_VER  0

  #define G_PRINT_LEVEL ACS_PRINT_TEST

  #define G_SW_OS            0
  #define G_SW_HYP           1
  #define G_SW_PS            2

  #define PE_INFO_TBL_SZ         8192 /*Supports maximum 400 PEs*/
  #define GIC_INFO_TBL_SZ        8192 /*Supports maximum 256 redistributors, 256 ITS blocks & 4 distributors*/
  #define TIMER_INFO_TBL_SZ      1024 /*Supports maximum 2 system timers*/
  #define WD_INFO_TBL_SZ         512  /*Supports maximum 20 Watchdogs*/
  #define MEM_INFO_TBL_SZ        32768/*Supports maximum 800 memory regions*/
  #define IOVIRT_INFO_TBL_SZ     32768/*Supports maximum 240 nodes of a typical iort table*/
  #define PERIPHERAL_INFO_TBL_SZ 1024 /*Supports maximum 20 PCIe EPs (USB and SATA controllers only) */
  #define PCIE_INFO_TBL_SZ       512  /*Supports maximum 20 RC's*/

  #ifdef _AARCH64_BUILD_
  unsigned long __stack_chk_guard = 0xBAAAAAAD;
  unsigned long __stack_chk_fail =  0xBAAFAAAD;
  #endif

#endif
