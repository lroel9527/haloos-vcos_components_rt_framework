/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 * Licensed under the Apache License, Version 2.0(the "License");
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

#ifndef RT_FRAMEWORK_INC_RTFW_MEM_TYPES_H
#define RT_FRAMEWORK_INC_RTFW_MEM_TYPES_H

#include <nuttx/config.h>
#include "Std_Types.h"
#include "rtfw_compiler_cfg.h"
#include "core_cfg.h"
#include "rtfw_types.h"

struct memcpy_section_s{
    uintptr_t    run_addr_begin;
    uintptr_t    run_addr_end;
    uintptr_t    load_addr_begin;
};

struct memset_section_s{
    uintptr_t    run_addr_begin;
    uintptr_t    run_addr_end;
    unsigned char    init_value;
};

struct mem_init_table_s {
    const struct memset_section_s *const memset_sections;
    const struct memcpy_section_s *const memcpy_sections;

    uint32 memset_section_count;
    uint32 memcpy_section_count;
};

extern const struct mem_init_table_s *const *const mem_init_table_ref_list[OS_CORE_CNT] RTFW_CONST;

void RTFW_CODE rtfw_memory_init(const struct mem_init_table_s *const mem_table);

#endif
