/*
 * Copyright (c) 2021 GOODIX.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 ****************************************************************************************
 *
 * @file scatter_config.h
 *
 * @brief Common scatter file definition file.
 *
 *
 ****************************************************************************************
 */

#ifndef SCATTER_CONFIG_H
#define SCATTER_CONFIG_H

#include "custom_config.h"

/*****************************************************************
 * if CSTACK_HEAP_SIZE is not defined in custom_config.h,
 * keep default setting to 32KB
 */
#ifndef CSTACK_HEAP_SIZE
#define CSTACK_HEAP_SIZE     0x8000
#endif

#define FLASH_START_ADDR         0x01000000
#define FLASH_SIZE               0x00800000

/* size of ROM reserved RAM in retention cell */
#ifndef ROM_RTN_RAM_SIZE
#define ROM_RTN_RAM_SIZE        0x4020
#endif

#define RAM_ALIAS

/*****************************************************************
 * Warning: User App developer never change the six macros below
 */
#ifdef RAM_ALIAS
#define RAM_START_ADDR          0x00800000
#else
#define RAM_START_ADDR          0x30000000
#endif

#if (CHIP_TYPE == 0)
#define RAM_SIZE            0x00040000
#else
#define RAM_SIZE            0x00020000
#endif

#define RAM_END_ADDR            (RAM_START_ADDR + RAM_SIZE)


#define FERP_SIZE               0x8000     // 32K
#define CRITICAL_CODE_MAX_SIZE  0x10000    // maximum size of critical code reserved

#if (APP_CODE_RUN_ADDR == APP_CODE_LOAD_ADDR && \
        APP_CODE_RUN_ADDR >= FLASH_START_ADDR && \
        APP_CODE_RUN_ADDR < FLASH_START_ADDR + FLASH_SIZE)
#define XIP_MODE
#endif
/****************************************************************/

/* ************************************************************************
 * developer must define CFG_MAX_CONNECTIONS in custom_config.h .
 * Max value for GR551X: 10 which must be same with CFG_CON
 * in ROM's configs.opt
 */
#ifndef CFG_MAX_CONNECTIONS
#error "CFG_MAX_CONNECTIONS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_CONNECTIONS <= 10)
#define USER_MAX_CONNECTIONS CFG_MAX_CONNECTIONS
#else
#define USER_MAX_CONNECTIONS (1)
#endif

#ifndef CFG_MAX_ADVS
#error "CFG_MAX_ADVS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_ADVS <= 5)
#define USER_MAX_ADVS CFG_MAX_ADVS
#else
#define USER_MAX_ADVS (1)
#endif

#ifndef CFG_MAX_PER_ADVS
#error "CFG_MAX_PER_ADVS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_PER_ADVS <= 5)
#define USER_MAX_PER_ADVS CFG_MAX_PER_ADVS
#else
#define USER_MAX_PER_ADVS (0)
#endif

#if ((USER_MAX_ADVS+USER_MAX_PER_ADVS) > 5)
#error "The number of BLE Legacy/Extended/Periodic Advertising exceeds the limit."
#endif

#ifndef CFG_MAX_SCAN
#error "CFG_MAX_SCAN is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_SCAN <= 1)
#define USER_MAX_SCAN CFG_MAX_SCAN
#else
#define USER_MAX_SCAN (1)
#endif

#ifndef CFG_MAX_SYNCS
#error "CFG_MAX_SYNCS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_SYNCS <= 5)
#define USER_MAX_SYNCS CFG_MAX_SYNCS
#else
#define USER_MAX_SYNCS (0)
#endif

#if ((USER_MAX_CONNECTIONS+USER_MAX_ADVS+2*USER_MAX_PER_ADVS+USER_MAX_SCAN+USER_MAX_SYNCS) > 12)
#error "The number of BLE Activities exceeds the limit."
#endif

#ifndef CFG_MAX_BOND_DEVS
#error "CFG_MAX_BOND_DEVS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_BOND_DEVS <= 10)
#define USER_MAX_BOND_DEVS CFG_MAX_BOND_DEVS
#else
#define USER_MAX_BOND_DEVS (1)
#endif

#ifndef CFG_MAX_PRFS
#error "CFG_MAX_PRFS is not defined in app's custom_config.h ."
#endif

#if (CFG_MAX_PRFS <= 64)
#define USER_MAX_PRFS CFG_MAX_PRFS
#else
#define USER_MAX_PRFS (1)
#endif

/* The macro is used to compute size of the heap block in bytes. */
#define MEM_HEAP_HEADER                     (12 / sizeof(uint32_t))
#define MEM_CALC_HEAP_LEN(len)              ((((len) + (sizeof(uint32_t) - 1)) / sizeof(uint32_t)) + MEM_HEAP_HEADER)
#define MEM_CALC_HEAP_LEN_IN_BYTES(len)     (MEM_CALC_HEAP_LEN(len) * sizeof(uint32_t))

#define ENV_HEAP_SIZE       MEM_CALC_HEAP_LEN_IN_BYTES(292 * USER_MAX_CONNECTIONS + \
                                                       426 * (USER_MAX_CONNECTIONS + USER_MAX_ADVS + \
                                                       2 * USER_MAX_PER_ADVS+USER_MAX_SCAN+USER_MAX_SYNCS) + \
                                                       600)
/* The size of heap for ATT database depends on the number of attributes in
 * profiles. The value can be tuned based on supported profiles. */
#if (CFG_MESH_SUPPORT == 1)
#include "mesh_stack_config.h"
#define ATT_DB_HEAP_SIZE    MEM_CALC_HEAP_LEN_IN_BYTES(1000 + MESH_HEAP_SIZE_ADD)
#else
#define ATT_DB_HEAP_SIZE    MEM_CALC_HEAP_LEN_IN_BYTES(1024)
#endif

#define KE_MSG_HEAP_SIZE    MEM_CALC_HEAP_LEN_IN_BYTES(1650 * (USER_MAX_SCAN+USER_MAX_SYNCS) + \
                                                       112 *(USER_MAX_CONNECTIONS + USER_MAX_ADVS + \
                                                       2*USER_MAX_PER_ADVS) + 408 *(USER_MAX_CONNECTIONS + \
                                                       USER_MAX_ADVS + 2 * USER_MAX_PER_ADVS + \
                                                       USER_MAX_SCAN + USER_MAX_SYNCS)+ 3072)
/* The size of non-retention heap is customized. This heap will used by BLE
 * stack only when other three heaps are full. */
#define NON_RET_HEAP_SIZE   MEM_CALC_HEAP_LEN_IN_BYTES(328 * 2)

#define PRF_BUF_SIZE    (92*USER_MAX_PRFS + 4)
#define BOND_BUF_SIZE   (8*USER_MAX_BOND_DEVS + 4)
#define CONN_BUF_SIZE   (372*USER_MAX_CONNECTIONS + 4)


/**************************************************************************/
/* sections on retention RAM cells */
#ifdef CFG_FERP
#define STACK_END_ADDR         (RAM_END_ADDR-FERP_SIZE)
#else
#define STACK_END_ADDR         (RAM_END_ADDR)
#endif

#ifndef GR5515_E
#define USE_TINY_RAM_SPACE
#endif

#define TINY_RAM_SPACE_START    (RAM_START_ADDR + 0x35CC) /* DONT MODIFY ME !!! */
#define TINY_RAM_SPACE_SIZE     (0x750)                   /* DONT MODIFY ME !!! */

#define FPB_SECTION_START       0x30004000
#define FPB_SECTION_SIZE        0x20

#define UNUSED_SECTION_SIZE     0x64

// Code size of Application
#ifndef APP_MAX_CODE_SIZE
#define APP_MAX_CODE_SIZE       0x00800000
#endif

// RAM size of Application
#ifndef APP_RAM_SIZE
#define APP_RAM_SIZE            0x00030000
#endif

#endif // SCATTER_CONFIG_H
