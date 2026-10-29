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

#ifndef RT_FRAMEWORK_INC_ARTI_H
#define RT_FRAMEWORK_INC_ARTI_H

#include <nuttx/config.h>
#include <stdint.h>

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1)

#if defined(CONFIG_ARTI_TASK_TRACE) && (CONFIG_ARTI_TASK_TRACE == 1)
/* trace task state changes */
#define ARTI_ENABLE_OS_TASK
#endif

#if defined(CONFIG_ARTI_RESOURCE_TRACE) && (CONFIG_ARTI_RESOURCE_TRACE == 1)
/* trace OS resource state changes */
#define ARTI_ENABLE_OS_RESOURCE
#endif

#if defined(CONFIG_ARTI_SPINLOCK_TRACE) && (CONFIG_ARTI_SPINLOCK_TRACE == 1)
/* trace spinlocks */
#define ARTI_ENABLE_OS_SPINLOCK
#endif

#if defined(CONFIG_ARTI_CAT2ISR_TRACE) && (CONFIG_ARTI_CAT2ISR_TRACE == 1)
/* trace category 2 ISRs */
#define ARTI_ENABLE_OS_CAT2ISR
#endif

#if defined(CONFIG_ARTI_INT_TRACE) && (CONFIG_ARTI_INT_TRACE == 1)
/* trace OS interrupt service calls */
#define ARTI_ENABLE_OS_SERVICECALLS
#endif

#if defined(CONFIG_ARTI_RUNNABLE_TRACE) && (CONFIG_ARTI_RUNNABLE_TRACE == 1)
/* trace RTE runnables */
#define ARTI_ENABLE_RTE_RUNNABLE
#endif

#include "arti.h"

typedef enum {
	RTFW_ARTI_TASK_RESUME_NONE = 0,
	RTFW_ARTI_TASK_RESUME_PREEMPTED,
	RTFW_ARTI_TASK_RESUME_RELEASED,
	RTFW_ARTI_TASK_RESUME_WAITING_EVENT,
	RTFW_ARTI_TASK_RESUME_UNBLOCKED
} rtfw_arti_task_resume_reason_t;

int rtfw_arti_get_task_info(const void *context, uint32_t *core_id,
				    uint32_t *task_id);
int rtfw_arti_get_current_task_info(uint32_t core_id, uint32_t *task_id);
uint32_t rtfw_arti_get_configured_task_count(void);
void rtfw_arti_set_task_resume_reason(const void *context,
				      rtfw_arti_task_resume_reason_t reason);
rtfw_arti_task_resume_reason_t
rtfw_arti_take_task_resume_reason(const void *context);
void rtfw_arti_set_task_resume_reason_by_id(uint32_t core_id,
					    uint32_t task_id,
					    rtfw_arti_task_resume_reason_t reason);
rtfw_arti_task_resume_reason_t
rtfw_arti_take_task_resume_reason_by_id(uint32_t core_id, uint32_t task_id);
void rtfw_arti_set_task_job_active_by_id(uint32_t core_id, uint32_t task_id,
					 int active);
int rtfw_arti_is_task_job_active_by_id(uint32_t core_id, uint32_t task_id);
int rtfw_arti_is_task_waiting_event_by_id(uint32_t core_id,
					   uint32_t task_id);
void rtfw_arti_set_current_task_id(uint32_t core_id, uint32_t task_id);
void rtfw_arti_task_activate(const void *task_cfg);
void rtfw_arti_task_start(const void *task_cfg);
void rtfw_arti_task_terminate(const void *task_cfg);
void rtfw_arti_task_reactivate(const void *task_cfg);
void rtfw_arti_task_suspend(const void *tcb);
void rtfw_arti_task_resume(const void *tcb);
void rtfw_arti_task_wakeup(const void *tcb);
uint8_t rtfw_arti_cat2isr_enter(const void *isr_cfg);
void rtfw_arti_cat2isr_exit(const void *isr_cfg);
void rtfw_arti_cat2isr_complete(const void *isr_cfg,
				 uint8_t entry_nesting);

#else

/* Keep instrumentation sites harmless when ARTI is not selected. */
#define ARTI_TRACE(...) do { } while (0)

#endif

#endif /* RT_FRAMEWORK_INC_ARTI_H */
