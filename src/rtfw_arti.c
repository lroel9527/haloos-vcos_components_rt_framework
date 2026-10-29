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
#include <nuttx/config.h>

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1)

#include <nuttx/event.h>
#include <sched/sched.h>

#include "rtfw_arti.h"
#include "rtfw_core.h"
#include "rtfw_isr_internal.h"
#include "rtfw_task.h"

#define OS_START_SEC_VAR
#include "rtfw_memmap.h"
volatile uint32 os_current_task_id[CONFIG_BMP_NCPUS] = {0};
#define OS_STOP_SEC_VAR
#include "rtfw_memmap.h"

static uint8_t g_arti_idle_resume_reason[CONFIG_BMP_NCPUS];

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static FUNC(os_task_inst_t *, OS_CODE)
rtfw_arti_get_task_instance_by_id(uint32_t core_id, uint32_t task_id)
{
	const os_task_cfg_t *task_cfg;

	if ((task_id >= (uint32_t)os_task_cfg_ref_table_size) ||
	    (os_task_cfg_ref_table[task_id] == NULL_PTR)) {
		return NULL_PTR;
	}

	task_cfg = os_task_cfg_ref_table[task_id];
	if ((uint32_t)OS_TASK_GET_OWNER_COREID(task_cfg) != core_id) {
		return NULL_PTR;
	}

	return OS_TASK_GET_INSTANCE_REF(task_cfg);
}

FUNC(int, OS_CODE) rtfw_arti_get_task_info(const void *context, uint32_t *core_id,
				      uint32_t *task_id)
{
	const os_ctx_cfg_t *ctx = (const os_ctx_cfg_t *)context;

	if ((ctx == NULL_PTR) ||
	    ((OS_CONTEXT_GET_INITCONTEXTID(ctx) & OS_CONTEXT_ANYTASK) == 0)) {
		return 0;
	}

	*core_id = (uint32_t)OS_CONTEXT_GET_OWNER_COREID(ctx);
	*task_id = (uint32_t)OS_CONTEXT_GET_CURRENT_TASKID(ctx);
	return 1;
}

FUNC(int, OS_CODE) rtfw_arti_get_current_task_info(uint32_t core_id,
					      uint32_t *task_id)
{
	uint32_t context_core_id;

	if (task_id == NULL) {
		return 0;
	}

	if ((rtfw_arti_get_task_info(os_context_get_contexthdl(),
				       &context_core_id, task_id) != 0) &&
	    (context_core_id == core_id)) {
		return 1;
	}

#if defined(CONFIG_BMP_NCPUS)
	if (core_id < (uint32_t)CONFIG_BMP_NCPUS) {
		*task_id = (uint32_t)os_task_cfg_ref_table_size + core_id;
		return 1;
	}
#endif

	return 0;
}

FUNC(uint32_t, OS_CODE) rtfw_arti_get_configured_task_count(void)
{
	return (uint32_t)os_task_cfg_ref_table_size;
}

FUNC(void, OS_CODE) rtfw_arti_set_task_resume_reason(const void *context,
	rtfw_arti_task_resume_reason_t reason)
{
	const os_ctx_cfg_t *ctx = (const os_ctx_cfg_t *)context;

	if ((ctx != NULL_PTR) &&
	    ((OS_CONTEXT_GET_INITCONTEXTID(ctx) & OS_CONTEXT_ANYTASK) != 0)) {
		OS_CONTEXT_GET_TASK_INSTANCE_REF(ctx)->arti_resume_reason =
			(uint8_t)reason;
	}
}

FUNC(rtfw_arti_task_resume_reason_t, OS_CODE)
rtfw_arti_take_task_resume_reason(const void *context)
{
	const os_ctx_cfg_t *ctx = (const os_ctx_cfg_t *)context;
	rtfw_arti_task_resume_reason_t reason = RTFW_ARTI_TASK_RESUME_NONE;

	if ((ctx != NULL_PTR) &&
	    ((OS_CONTEXT_GET_INITCONTEXTID(ctx) & OS_CONTEXT_ANYTASK) != 0)) {
		os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(ctx);

		reason = (rtfw_arti_task_resume_reason_t)task_inst->arti_resume_reason;
		task_inst->arti_resume_reason = (uint8_t)RTFW_ARTI_TASK_RESUME_NONE;
	}

	return reason;
}

FUNC(void, OS_CODE) rtfw_arti_set_task_resume_reason_by_id(uint32_t core_id,
	uint32_t task_id, rtfw_arti_task_resume_reason_t reason)
{
	os_task_inst_t *task_inst =
		rtfw_arti_get_task_instance_by_id(core_id, task_id);

	if (task_inst != NULL_PTR) {
		task_inst->arti_resume_reason = (uint8_t)reason;
	} else if ((core_id < (uint32_t)CONFIG_BMP_NCPUS) &&
		   (task_id == (uint32_t)os_task_cfg_ref_table_size + core_id)) {
		g_arti_idle_resume_reason[core_id] = (uint8_t)reason;
	}
}

FUNC(rtfw_arti_task_resume_reason_t, OS_CODE)
rtfw_arti_take_task_resume_reason_by_id(uint32_t core_id, uint32_t task_id)
{
	os_task_inst_t *task_inst =
		rtfw_arti_get_task_instance_by_id(core_id, task_id);
	rtfw_arti_task_resume_reason_t reason = RTFW_ARTI_TASK_RESUME_NONE;

	if (task_inst != NULL_PTR) {
		reason = (rtfw_arti_task_resume_reason_t)task_inst->arti_resume_reason;
		task_inst->arti_resume_reason =
			(uint8_t)RTFW_ARTI_TASK_RESUME_NONE;
	} else if ((core_id < (uint32_t)CONFIG_BMP_NCPUS) &&
		   (task_id == (uint32_t)os_task_cfg_ref_table_size + core_id)) {
		reason = (rtfw_arti_task_resume_reason_t)
			g_arti_idle_resume_reason[core_id];
		g_arti_idle_resume_reason[core_id] =
			(uint8_t)RTFW_ARTI_TASK_RESUME_NONE;
	}

	return reason;
}

FUNC(void, OS_CODE) rtfw_arti_set_task_job_active_by_id(uint32_t core_id,
						    uint32_t task_id,
						    int active)
{
	os_task_inst_t *task_inst =
		rtfw_arti_get_task_instance_by_id(core_id, task_id);

	if (task_inst != NULL_PTR) {
		task_inst->arti_job_active = active ? 1U : 0U;
	}
}

FUNC(int, OS_CODE) rtfw_arti_is_task_job_active_by_id(uint32_t core_id,
						  uint32_t task_id)
{
	os_task_inst_t *task_inst =
		rtfw_arti_get_task_instance_by_id(core_id, task_id);

	if (task_inst != NULL_PTR) {
		return task_inst->arti_job_active != 0U;
	}

	/* Idle tasks are permanent scheduler jobs and may always be preempted. */
	return (core_id < (uint32_t)CONFIG_BMP_NCPUS) &&
	       (task_id == (uint32_t)os_task_cfg_ref_table_size + core_id);
}

FUNC(int, OS_CODE) rtfw_arti_is_task_waiting_event_by_id(uint32_t core_id,
						     uint32_t task_id)
{
#ifdef CONFIG_SCHED_EVENTS
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;

	if ((task_id >= (uint32_t)os_task_cfg_ref_table_size) ||
	    (os_task_cfg_ref_table[task_id] == NULL_PTR)) {
		return 0;
	}

	task_cfg = os_task_cfg_ref_table[task_id];
	if (((uint32_t)OS_TASK_GET_OWNER_COREID(task_cfg) != core_id) ||
	    (task_cfg->event_hdl == NULL_PTR)) {
		return 0;
	}

	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

	/* sem_post clears waitobj before the wakeup note. Keep the ARTI wait
	 * state until the matching wakeup has recorded its resume reason.
	 */
	return (task_inst->thread.tcb.waitobj ==
		(void *)&task_cfg->event_hdl->wait.sem) ||
	       (task_inst->arti_resume_reason ==
		(uint8_t)RTFW_ARTI_TASK_RESUME_WAITING_EVENT);
#else
	(void)core_id;
	(void)task_id;
	return 0;
#endif
}

FUNC(void, OS_CODE) rtfw_arti_set_current_task_id(uint32_t core_id,
					      uint32_t task_id)
{
	if ((core_id < (uint32_t)CONFIG_BMP_NCPUS) &&
	    ((task_id < (uint32_t)os_task_cfg_ref_table_size) ||
	     (task_id == (uint32_t)os_task_cfg_ref_table_size + core_id))) {
		os_current_task_id[core_id] = task_id;
	}
}

#if defined(CONFIG_ARTI_TASK_TRACE) && (CONFIG_ARTI_TASK_TRACE == 1)
static FUNC(int, OS_CODE) rtfw_arti_is_idle_tcb(const struct tcb_s *tcb)
{
	/* BMP gives every per-CPU idle TCB the reserved PID 0.  The idle
	 * priority and the missing RT Framework context distinguish it from
	 * configured tasks; PID alone cannot identify its CPU.
	 */
	return (tcb != NULL_PTR) &&
	       (tcb->contexthdl == NULL_PTR) &&
	       (tcb->sched_priority == 0);
}

static FUNC(int, OS_CODE) rtfw_arti_get_tcb_task_info(const struct tcb_s *tcb,
							  uint32_t *core_id,
							  uint32_t *task_id)
{
	if (tcb == NULL_PTR) {
		return 0;
	}

	if (rtfw_arti_get_task_info(tcb->contexthdl, core_id, task_id) != 0) {
		return 1;
	}

	if (rtfw_arti_is_idle_tcb(tcb)) {
		*core_id = (uint32_t)os_get_core_id();
		if (*core_id >= (uint32_t)CONFIG_BMP_NCPUS) {
			return 0;
		}

		*task_id = rtfw_arti_get_configured_task_count() + *core_id;
		return 1;
	}

	return 0;
}

FUNC(void, OS_CODE) rtfw_arti_task_activate(const void *task_cfg_ref)
{
	const os_task_cfg_t *task_cfg = (const os_task_cfg_t *)task_cfg_ref;

	ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os,
		   OS_TASK_GET_OWNER_COREID(task_cfg), OsTask_Activate,
		   OS_TASK_GET_TASKID(task_cfg));
}

FUNC(void, OS_CODE) rtfw_arti_task_start(const void *task_cfg_ref)
{
	const os_task_cfg_t *task_cfg = (const os_task_cfg_t *)task_cfg_ref;
	uint32_t core_id = (uint32_t)OS_TASK_GET_OWNER_COREID(task_cfg);
	uint32_t task_id = (uint32_t)OS_TASK_GET_TASKID(task_cfg);

	rtfw_arti_set_task_resume_reason(OS_TASK_GET_CONTEXT_REF(task_cfg),
					  RTFW_ARTI_TASK_RESUME_NONE);
	ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id, OsTask_Start,
		   task_id);
	rtfw_arti_set_task_job_active_by_id(core_id, task_id, 1);
}

FUNC(void, OS_CODE) rtfw_arti_task_terminate(const void *task_cfg_ref)
{
	const os_task_cfg_t *task_cfg = (const os_task_cfg_t *)task_cfg_ref;
	uint32_t core_id = (uint32_t)OS_TASK_GET_OWNER_COREID(task_cfg);
	uint32_t task_id = (uint32_t)OS_TASK_GET_TASKID(task_cfg);

	rtfw_arti_set_task_job_active_by_id(core_id, task_id, 0);
	rtfw_arti_set_task_resume_reason(OS_TASK_GET_CONTEXT_REF(task_cfg),
					  RTFW_ARTI_TASK_RESUME_NONE);
	ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id, OsTask_Terminate,
		   task_id);
}

FUNC(void, OS_CODE) rtfw_arti_task_reactivate(const void *task_cfg_ref)
{
	rtfw_arti_task_activate(task_cfg_ref);
}

FUNC(void, OS_CODE) rtfw_arti_task_suspend(const void *tcb_ref)
{
	const struct tcb_s *tcb = (const struct tcb_s *)tcb_ref;
	uint32_t core_id;
	uint32_t task_id;

#if defined(CONFIG_ARTI_CAT2ISR_TRACE) && (CONFIG_ARTI_CAT2ISR_TRACE == 1)
	if (up_interrupt_context()) {
		return;
	}
#endif

	if ((rtfw_arti_get_tcb_task_info(tcb, &core_id, &task_id) == 0) ||
	    (rtfw_arti_is_task_job_active_by_id(core_id, task_id) == 0)) {
		return;
	}

#ifdef CONFIG_SCHED_EVENTS
	if (rtfw_arti_is_task_waiting_event_by_id(core_id, task_id) != 0) {
		rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
			RTFW_ARTI_TASK_RESUME_WAITING_EVENT);
		rtfw_arti_set_task_job_active_by_id(core_id, task_id, 0);
		ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
			   OsTask_Wait, task_id);
		return;
	}
#endif

	if ((tcb->task_state == TSTATE_TASK_READYTORUN) ||
	    rtfw_arti_is_idle_tcb(tcb)) {
		rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
			RTFW_ARTI_TASK_RESUME_PREEMPTED);
		ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
			   OsTask_Preempt, task_id);
	}
}

FUNC(void, OS_CODE) rtfw_arti_task_resume(const void *tcb_ref)
{
	const struct tcb_s *tcb = (const struct tcb_s *)tcb_ref;
	rtfw_arti_task_resume_reason_t reason;
	uint32_t core_id;
	uint32_t task_id;

#if defined(CONFIG_ARTI_CAT2ISR_TRACE) && (CONFIG_ARTI_CAT2ISR_TRACE == 1)
	if (up_interrupt_context()) {
		return;
	}
#endif

	if (rtfw_arti_get_tcb_task_info(tcb, &core_id, &task_id) == 0) {
		return;
	}

	rtfw_arti_set_current_task_id(core_id, task_id);
	reason = rtfw_arti_take_task_resume_reason_by_id(core_id, task_id);
	if (reason == RTFW_ARTI_TASK_RESUME_PREEMPTED) {
		ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
			   OsTask_Resume, task_id);
	} else if ((reason == RTFW_ARTI_TASK_RESUME_RELEASED) ||
		   (reason == RTFW_ARTI_TASK_RESUME_UNBLOCKED)) {
		ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
			   OsTask_Continue, task_id);
		rtfw_arti_set_task_job_active_by_id(core_id, task_id, 1);
	}
}

FUNC(void, OS_CODE) rtfw_arti_task_wakeup(const void *tcb_ref)
{
#ifdef CONFIG_SCHED_EVENTS
	const struct tcb_s *tcb = (const struct tcb_s *)tcb_ref;
	uint32_t core_id;
	uint32_t task_id;

	if ((tcb != NULL_PTR) &&
	    (rtfw_arti_get_tcb_task_info(tcb, &core_id, &task_id) != 0) &&
	    (rtfw_arti_is_task_waiting_event_by_id(core_id, task_id) != 0)) {
		if (tcb->errcode == 0) {
			rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
				RTFW_ARTI_TASK_RESUME_RELEASED);
			ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
				   OsTask_Release, task_id);
		} else {
			/* Timeout/signal wakeups resume execution without SetEvent. */
			rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
				RTFW_ARTI_TASK_RESUME_UNBLOCKED);
		}
	}
#else
	(void)tcb_ref;
#endif
}
#else
FUNC(void, OS_CODE) rtfw_arti_task_activate(const void *task_cfg_ref)
{
	(void)task_cfg_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_start(const void *task_cfg_ref)
{
	(void)task_cfg_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_terminate(const void *task_cfg_ref)
{
	(void)task_cfg_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_reactivate(const void *task_cfg_ref)
{
	(void)task_cfg_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_suspend(const void *tcb_ref)
{
	(void)tcb_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_resume(const void *tcb_ref)
{
	(void)tcb_ref;
}

FUNC(void, OS_CODE) rtfw_arti_task_wakeup(const void *tcb_ref)
{
	(void)tcb_ref;
}
#endif

#if defined(CONFIG_ARTI_CAT2ISR_TRACE) && \
	(CONFIG_ARTI_CAT2ISR_TRACE == 1) && \
	defined(CONFIG_IRQ_USE_ARCH_DISPATCH)
static FUNC(uint32_t, OS_CODE)
rtfw_arti_get_cat2isr_trace_id(const os_isr_cfg_t *isr_cfg)
{
	uint32_t cat2_index = 0U;
	uint32_t isr_id;

	/*
	 * isr_id is the logical ID of all configured ISRs and therefore contains
	 * CAT1 entries.  TRACE32 /AddTraceID assigns IDs only to the ISR2 objects
	 * emitted by haloos.orti, so derive the matching dense CAT2 ordinal here.
	 */
	for (isr_id = 0U; isr_id < (uint32_t)os_isr_cfg_ref_table_size; isr_id++) {
		const os_isr_cfg_t *candidate = os_isr_cfg_ref_table[isr_id];

		if (candidate == NULL_PTR) {
			continue;
		}

		if (candidate == isr_cfg) {
			return rtfw_arti_get_configured_task_count() +
			       (uint32_t)CONFIG_BMP_NCPUS + cat2_index;
		}

		if (OS_ISR_GET_ISR_TYPE(candidate) == ISR_CAT2) {
			cat2_index++;
		}
	}

	/* Keep a usable ID if a non-table ISR reaches the trace hook. */
	return rtfw_arti_get_configured_task_count() +
	       (uint32_t)CONFIG_BMP_NCPUS +
	       (uint32_t)OS_ISR_GET_ISRID(isr_cfg);
}

FUNC(uint8_t, OS_CODE) rtfw_arti_cat2isr_enter(const void *isr_cfg_ref)
{
	const os_isr_cfg_t *isr_cfg = (const os_isr_cfg_t *)isr_cfg_ref;
	uint32_t core_id = (uint32_t)OS_ISR_GET_OWNER_COREID(isr_cfg);
	uint8_t nesting = up_get_irq_nesting();
	uint32_t task_id;
	int task_job_active = 0;
	int task_waiting_event = 0;

	if ((nesting == 0U) &&
	    (rtfw_arti_get_current_task_info(core_id, &task_id) != 0)) {
		task_job_active = rtfw_arti_is_task_job_active_by_id(core_id,
							      task_id);
		if (task_job_active != 0) {
			task_waiting_event =
				rtfw_arti_is_task_waiting_event_by_id(core_id,
								     task_id);
			if (task_waiting_event != 0) {
				rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
					RTFW_ARTI_TASK_RESUME_WAITING_EVENT);
				rtfw_arti_set_task_job_active_by_id(core_id,
								 task_id, 0);
				ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
					   OsTask_Wait, task_id);
			}
		}

		if ((task_job_active != 0) && (task_waiting_event == 0)) {
			rtfw_arti_set_task_resume_reason_by_id(core_id, task_id,
				RTFW_ARTI_TASK_RESUME_PREEMPTED);
			ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
				   OsTask_Preempt, task_id);
		}
	}

	ARTI_TRACE(NOSUSP, AR_CP_OS_CAT2ISR, Os, core_id, OsCat2Isr_Start,
		   rtfw_arti_get_cat2isr_trace_id(isr_cfg));

	return nesting;
}

FUNC(void, OS_CODE) rtfw_arti_cat2isr_exit(const void *isr_cfg_ref)
{
	const os_isr_cfg_t *isr_cfg = (const os_isr_cfg_t *)isr_cfg_ref;
	uint32_t core_id = (uint32_t)OS_ISR_GET_OWNER_COREID(isr_cfg);

	ARTI_TRACE(NOSUSP, AR_CP_OS_CAT2ISR, Os, core_id, OsCat2Isr_Stop,
		   rtfw_arti_get_cat2isr_trace_id(isr_cfg));
}

FUNC(void, OS_CODE) rtfw_arti_cat2isr_complete(const void *isr_cfg_ref,
					   uint8_t entry_nesting)
{
	const os_isr_cfg_t *isr_cfg = (const os_isr_cfg_t *)isr_cfg_ref;
	uint32_t core_id = (uint32_t)OS_ISR_GET_OWNER_COREID(isr_cfg);
	uint32_t task_id;
	rtfw_arti_task_resume_reason_t resume_reason;

	if ((entry_nesting == 0U) &&
	    (rtfw_arti_get_current_task_info(core_id, &task_id) != 0)) {
		rtfw_arti_set_current_task_id(core_id, task_id);
		resume_reason = rtfw_arti_take_task_resume_reason_by_id(core_id,
								      task_id);
		if (resume_reason == RTFW_ARTI_TASK_RESUME_PREEMPTED) {
			ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
				   OsTask_Resume, task_id);
		} else if ((resume_reason == RTFW_ARTI_TASK_RESUME_RELEASED) ||
			   (resume_reason == RTFW_ARTI_TASK_RESUME_UNBLOCKED)) {
			ARTI_TRACE(NOSUSP, AR_CP_OSARTI_TASK, Os, core_id,
				   OsTask_Continue, task_id);
			rtfw_arti_set_task_job_active_by_id(core_id, task_id, 1);
		}
	}
}
#endif

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#endif
