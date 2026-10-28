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

#include <string.h>
#include "rtfw_mem_internal.h"

static inline_function void rtfw_memory_section_copy(const struct memcpy_section_s *const memcpy_sec)
{
	size_t length = (size_t)(memcpy_sec->run_addr_end - memcpy_sec->run_addr_begin);

	(void)memcpy((void *)memcpy_sec->run_addr_begin,
		     (const void *)memcpy_sec->load_addr_begin, length);
}

static inline_function void rtfw_memory_section_set(const struct memset_section_s *const memset_sec)
{
	size_t length = (size_t)(memset_sec->run_addr_end - memset_sec->run_addr_begin);

	(void)memset((void *)memset_sec->run_addr_begin, memset_sec->init_value, length);
}

void RTFW_CODE rtfw_memory_init(const struct mem_init_table_s *const mem_table)
{
	uint32 i;

	if (mem_table == NULL_PTR) {
		return;
	}

	for (i = 0U; i < mem_table->memset_section_count; i++) {
		rtfw_memory_section_set(&(mem_table->memset_sections[i]));
	}

	for (i = 0U; i < mem_table->memcpy_section_count; i++) {
		rtfw_memory_section_copy(&(mem_table->memcpy_sections[i]));
	}
}
