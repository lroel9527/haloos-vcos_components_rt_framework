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

/* Including File */
#include <nuttx/config.h>
#include "Os.h"
#include "rtfw_types.h"
#include "rtfw_alarm.h"
#include "rtfw_application.h"
#include "rtfw_interrupt.h"
#include "rtfw_task.h"
#include "rtfw_context.h"
#include "rtfw_event.h"
#include "rtfw_error.h"
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
#include "rtfw_counter.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "rtfw_ipicall_internal.h"
#endif

static inline boolean
os_counter_check_value_max_allowed(os_counter_cfg_t const * const counter,
				   const TickType tick_value)
{
	return (tick_value <= counter->max_allowed_value);
}

static inline boolean
os_counter_check_value_min_cycle_or_zero(os_counter_cfg_t const * const counter,
					    const TickType cycle)
{
	return ((cycle >= counter->min_cycle) || (cycle == 0u));
}

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
static inline os_status_t
os_alarm_access_check(const os_ctx_cfg_t *const ctx_ref, const os_alarm_cfg_t *alarm_cfg)
{
	if (UNLIKELY(ctx_ref == NULL || alarm_cfg == NULL)) {
		return OS_STATUS_PARAM_POINTER_1;
	}
	if (UNLIKELY(OS_STATUS_OK !=
		os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(ctx_ref),
				    alarm_cfg->access_app_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
	return OS_STATUS_OK;
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
/* Local Function Implementation */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/* Global Function Implementation */
FUNC(void, OS_CODE) os_alarm_init(const os_alarm_cfg_t * const os_alarm_cfg_table[], OsObjIndexType size)
{
	const os_alarm_cfg_t *alarm_cfg;
	OsObjIndexType index;
	os_alarm_inst_t *alarm_inst;
	const os_app_cfg_t *app_cfg;
	CoreIdType core_id;

	if ((os_alarm_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	core_id = os_get_core_id();

	for (index = 0U; index < size; index++) {
		alarm_cfg = os_alarm_cfg_table[index];
		app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];

		if (app_cfg->core_id != core_id) {
			continue;
		}

		alarm_inst = alarm_cfg->obj_desc;

		/* Initialize basic fields */
		alarm_inst->cycle = 0;
		alarm_inst->alarm_state = OS_ALARMSTATE_DISABLE;

		if (alarm_cfg->autostart.autostart_mode == TRUE) {
			switch (alarm_cfg->autostart.autostart_type) {
			case OS_ALARM_START_ABSOLUTE:
				alarm_inst->cycle = alarm_cfg->autostart.cycle_time;
				os_counter_add_abs_action(&alarm_cfg->action_config, alarm_cfg->autostart.alarm_time);
				alarm_inst->alarm_state = OS_ALARMSTATE_ENABLE;
				break;
			case OS_ALARM_START_RELATIVE:
				alarm_inst->cycle = alarm_cfg->autostart.cycle_time;
				os_counter_add_rel_action(&alarm_cfg->action_config, alarm_cfg->autostart.alarm_time);
				alarm_inst->alarm_state = OS_ALARMSTATE_ENABLE;
				break;
			default:
				/* Do not set ENABLE state for unsupported autostart types */
				break;
			}
		} else {
			continue;
		}
	}
}

FUNC(os_status_t, OS_CODE) os_get_alarm_base(AlarmType alarm_id, AlarmBaseRefType info)
{
	const os_alarm_cfg_t *alarm_cfg;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
#endif
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	const os_app_cfg_t *app_cfg;
#endif

	if (UNLIKELY(alarm_id >= os_alarm_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(info == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETALARMBASE) == FALSE)) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */

	alarm_cfg = os_alarm_cfg_ref_table[alarm_id];

	if (UNLIKELY(alarm_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_alarm_access_check(cur_context, alarm_cfg))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
	app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];
	if (UNLIKELY(os_app_check_is_accessible(app_cfg) != OS_STATUS_OK)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	info->maxallowedvalue = alarm_cfg->action_config.counter->max_allowed_value;
	info->mincycle = alarm_cfg->action_config.counter->min_cycle;
	info->tickperbase = alarm_cfg->action_config.counter->tick_per_base;
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_alarm(AlarmType alarm_id, TickRefType tick)
{
	const os_alarm_cfg_t *alarm_cfg;
	const os_app_cfg_t *app_cfg;
	os_alarm_inst_t const *alarm_inst;
	hrtimer_tick_t now;
	CoreIdType core_id;
	irqlevel_t int_state;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
#endif

	if (UNLIKELY(alarm_id >= os_alarm_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(tick == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETALARM) == FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */

	alarm_cfg = os_alarm_cfg_ref_table[alarm_id];

	if (UNLIKELY(alarm_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_alarm_access_check(cur_context, alarm_cfg))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];
	core_id = app_cfg->core_id;

	if (core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
		if (UNLIKELY(os_app_check_is_accessible(app_cfg) != OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		alarm_inst = alarm_cfg->obj_desc;

		os_int_suspend_kernel(&int_state);

		if (alarm_inst->alarm_state != OS_ALARMSTATE_ENABLE) {
			os_int_resume_kernel(&int_state);
			return OS_STATUS_NOFUNC_1;
		} else {
			now = hrtimer_get_time(&alarm_cfg->action_config.counter->obj_desc->hrtimer_queue);
			*tick = hrtimer_has_expired(alarm_inst->hrtimer.expiration_time, now) ? 0U :
				(TickType)(alarm_inst->hrtimer.expiration_time - now);
		}
		os_int_resume_kernel(&int_state);
		return OS_STATUS_OK;
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_get_alarm((uint32)core_id, alarm_id, tick);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

FUNC(os_status_t, OS_CODE) os_set_rel_alarm(AlarmType alarm_id, TickType increment, TickType cycle)
{
	const os_alarm_cfg_t *alarm_cfg;
	const os_app_cfg_t *app_cfg;
	os_alarm_inst_t *alarm_inst;
	CoreIdType core_id;
	irqlevel_t int_state;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
#endif

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(os_context_check_curcontext_id(cur_context, OS_CONTEXT_SETRELALARM) == FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */
	if (UNLIKELY(alarm_id >= os_alarm_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(increment == (TickType)0)) {
		return OS_STATUS_VALUE;
	}

	alarm_cfg = os_alarm_cfg_ref_table[alarm_id];

	if (UNLIKELY(alarm_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(os_counter_check_value_max_allowed(alarm_cfg->action_config.counter, increment) == FALSE)) {
		return OS_STATUS_VALUE_TOO_HIGH_2;
	}
	if (UNLIKELY(os_counter_check_value_max_allowed(alarm_cfg->action_config.counter, cycle) == FALSE)) {
		return OS_STATUS_VALUE_TOO_HIGH_3;
	}
	if (UNLIKELY(os_counter_check_value_min_cycle_or_zero(alarm_cfg->action_config.counter, cycle) == FALSE)) {
		return OS_STATUS_VALUE_TOO_LOW_3;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_alarm_access_check(cur_context, alarm_cfg))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];
	core_id = app_cfg->core_id;

	if (core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
		if (UNLIKELY(os_app_check_is_accessible(app_cfg) != OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		alarm_inst = alarm_cfg->obj_desc;

		os_int_suspend_kernel(&int_state);

		if (UNLIKELY(alarm_inst->alarm_state == OS_ALARMSTATE_ENABLE)) {
			os_int_resume_kernel(&int_state);
			return OS_STATUS_STATE;
		}
		alarm_inst->cycle = cycle;
		os_counter_add_rel_action(&alarm_cfg->action_config, increment);
		alarm_inst->alarm_state = OS_ALARMSTATE_ENABLE;

		os_int_resume_kernel(&int_state);
		return OS_STATUS_OK;
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_rel_alarm((uint32)core_id, alarm_id, increment, cycle);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

FUNC(os_status_t, OS_CODE) os_set_abs_alarm(AlarmType alarm_id, TickType start, TickType cycle)
{
	const os_alarm_cfg_t *alarm_cfg;
	const os_app_cfg_t *app_cfg;
	os_alarm_inst_t *alarm_inst;
	CoreIdType core_id;
	irqlevel_t int_state;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
#endif

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(os_context_check_curcontext_id(cur_context, OS_CONTEXT_SETABSALARM) == FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */
	if (UNLIKELY(alarm_id >= os_alarm_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	alarm_cfg = os_alarm_cfg_ref_table[alarm_id];

	if (UNLIKELY(alarm_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(os_counter_check_value_max_allowed(alarm_cfg->action_config.counter, start) == FALSE)) {
		return OS_STATUS_VALUE_TOO_HIGH_2;
	}
	if (UNLIKELY(os_counter_check_value_max_allowed(alarm_cfg->action_config.counter, cycle) == FALSE)) {
		return OS_STATUS_VALUE_TOO_HIGH_3;
	}
	if (UNLIKELY(os_counter_check_value_min_cycle_or_zero(alarm_cfg->action_config.counter, cycle) == FALSE)) {
		return OS_STATUS_VALUE_TOO_LOW_3;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_alarm_access_check(cur_context, alarm_cfg))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];
	core_id = app_cfg->core_id;

	if (core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
		if (UNLIKELY(os_app_check_is_accessible(app_cfg) != OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		alarm_inst = alarm_cfg->obj_desc;

		os_int_suspend_kernel(&int_state);

		if (UNLIKELY(alarm_inst->alarm_state == OS_ALARMSTATE_ENABLE)) {
			os_int_resume_kernel(&int_state);
			return OS_STATUS_STATE;
		}
		alarm_inst->cycle = cycle;
		os_counter_add_abs_action(&alarm_cfg->action_config, start);
		alarm_inst->alarm_state = OS_ALARMSTATE_ENABLE;

		os_int_resume_kernel(&int_state);
		return OS_STATUS_OK;
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_abs_alarm((uint32)core_id, alarm_id, start, cycle);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

FUNC(os_status_t, OS_CODE) os_cancel_alarm(AlarmType alarm_id)
{
	const os_alarm_cfg_t *alarm_cfg;
	const os_app_cfg_t *app_cfg;
	os_alarm_inst_t *alarm_inst;
	CoreIdType core_id;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
#endif

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(os_context_check_curcontext_id(cur_context, OS_CONTEXT_CANCELALARM) == FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */

	if (UNLIKELY(alarm_id >= os_alarm_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	alarm_cfg = os_alarm_cfg_ref_table[alarm_id];

	if (UNLIKELY(alarm_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_alarm_access_check(cur_context, alarm_cfg))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

	app_cfg = os_app_cfg_ref_table[alarm_cfg->app_owner_id];
	core_id = app_cfg->core_id;

	if (core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
		if (UNLIKELY(os_app_check_is_accessible(app_cfg) != OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		alarm_inst = alarm_cfg->obj_desc;

		if (UNLIKELY(alarm_inst->alarm_state != OS_ALARMSTATE_ENABLE)) {
			return OS_STATUS_NOFUNC_1;
		}
		os_counter_delete_action(&alarm_cfg->action_config);
		alarm_inst->alarm_state = OS_ALARMSTATE_DISABLE;
		return OS_STATUS_OK;
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_cancel_alarm((uint32)core_id, alarm_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

static inline void alarm_do_cycle(const os_action_cfg_t *action_config, os_alarm_inst_t *alarm_inst)
{
	if (LIKELY(alarm_inst->cycle != 0U)) {
		/* cyclic function */
		os_counter_reload_action(action_config, alarm_inst->cycle);
	} else {
		alarm_inst->alarm_state = OS_ALARMSTATE_DISABLE;
	}
}

FUNC(void, OS_CODE) alarm_set_event_func(struct hrtimer_s *hrtimer)
{
	const os_action_cfg_t *action_config;
	const os_alarm_cfg_t *alarm_cfg;
	os_alarm_inst_t *alarm_inst;
	os_status_t result;

	action_config = (const os_action_cfg_t *)(hrtimer->arg);
	alarm_cfg = OS_GET_ALARM_CFG_BY_ACTION_ADDR(action_config);

	if (UNLIKELY(alarm_cfg == NULL || alarm_cfg->obj_desc == NULL)) {
		return;
	}

	alarm_inst = alarm_cfg->obj_desc;

	if (UNLIKELY(alarm_inst->alarm_state != OS_ALARMSTATE_ENABLE)) {
		return;
	}

	alarm_do_cycle(action_config, alarm_inst);

	result = os_set_event_asyn(alarm_cfg->action_attr.event.task, alarm_cfg->action_attr.event.mask);
	REPORT_SERVICE_ERROR(OSServiceId_SetEvent, result, 2, (uint32)alarm_cfg->action_attr.event.task,
							(uint32)alarm_cfg->action_attr.event.mask);
}

FUNC(void, OS_CODE) alarm_activate_task_func(struct hrtimer_s *hrtimer)
{
	const os_action_cfg_t *action;
	const os_alarm_cfg_t *alarm_cfg;
	os_alarm_inst_t *alarm_inst;
	os_status_t result;

	action = (const os_action_cfg_t *)(hrtimer->arg);
	alarm_cfg = OS_GET_ALARM_CFG_BY_ACTION_ADDR(action);

	if (UNLIKELY(alarm_cfg == NULL || alarm_cfg->obj_desc == NULL)) {
		return;
	}

	alarm_inst = alarm_cfg->obj_desc;

	if (UNLIKELY(alarm_inst->alarm_state != OS_ALARMSTATE_ENABLE)) {
		return;
	}

	alarm_do_cycle(action, alarm_inst);

	result = os_activate_task_asyn(alarm_cfg->action_attr.task);
	REPORT_SERVICE_ERROR(OSServiceId_ActivateTask, result, 1, (uint32)alarm_cfg->action_attr.task);
}

FUNC(void, OS_CODE) alarm_callback_func(struct hrtimer_s *hrtimer)
{
	const os_action_cfg_t *action;
	const os_alarm_cfg_t *alarm_cfg;
	os_alarm_inst_t *alarm_inst;
	os_context_id_t context_id;

	action = (const os_action_cfg_t *)(hrtimer->arg);
	alarm_cfg = OS_GET_ALARM_CFG_BY_ACTION_ADDR(action);

	if (UNLIKELY(alarm_cfg == NULL || alarm_cfg->obj_desc == NULL)) {
		return;
	}

	alarm_inst = alarm_cfg->obj_desc;

	if (UNLIKELY(alarm_inst->alarm_state != OS_ALARMSTATE_ENABLE)) {
		return;
	}

	alarm_do_cycle(action, alarm_inst);

	context_id = os_get_curcontext_id();
	os_set_curcontext_id(OS_ALARMCALLBACK_CONTEXT_ID);
	alarm_cfg->action_attr.callback_function();
	os_set_curcontext_id(context_id);
}

FUNC(void, OS_CODE) os_alarm_kill(const os_alarm_cfg_t * const alarm_cfg)
{
	os_cancel_alarm(alarm_cfg->alarm_id);
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"