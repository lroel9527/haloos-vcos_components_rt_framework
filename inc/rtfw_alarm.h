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

#ifndef RT_FRAMEWORK_INC_OS_ALARM_H
#define RT_FRAMEWORK_INC_OS_ALARM_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include "rtfw_alarm_internal.h"

/* Alarm declare */
#define DeclareAlarm(section_const, section_var, alarm_id) \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_alarm_inst_t, alarm_id); \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_alarm, const os_alarm_cfg_t, alarm_id)

#define ALARMCFGVAR(alarm_id) RTFW_LINK_ENTRY_NAME(os_alarm, alarm_id)
#define ALARMOBJVAR(alarm_id) RTFW_LINK_ENTRY_NAME(os_obj, alarm_id)

#define GET_APP_CFG_BY_ALARM_ID(alarm_id) \
os_app_cfg_ref_table[os_alarm_cfg_ref_table[alarm_id]->app_owner_id]
#define GET_COUNTER_ID_BY_ALARM_ID(alarm_id) \
	((os_alarm_cfg_ref_table[alarm_id]->action_config.counter)->counter_id)
#define OS_GET_ALARM_CFG_BY_ACTION_ADDR(_ActionAddr) \
	((os_alarm_cfg_t *)((char *)(_ActionAddr) - offsetof(os_alarm_cfg_t, action_config)))

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/*!  \brief         Alarm SetEventAlarm callback function
 *  \details        None
 *
 *  \param[in]      hrtimer High resolution timer
 *
 *  \return         void
 *
 *
 *  \context        ISR2|TASK
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) alarm_set_event_func(struct hrtimer_s *hrtimer);
/*!  \brief         Alarm ActivateTask callback function
 *  \details        None
 *
 *  \param[in]      hrtimer High resolution timer
 *
 *  \return         void
 *
 *
 *  \context        ISR2|TASK
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) alarm_activate_task_func(struct hrtimer_s *hrtimer);
/*!  \brief         Alarm CallBack type callback function
 *  \details        None
 *
 *  \param[in]      hrtimer High resolution timer
 *
 *  \return         void
 *
 *
 *  \context        ISR2|TASK
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) alarm_callback_func(struct hrtimer_s *hrtimer);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif	 /* RT_FRAMEWORK_INC_OS_ALARM_H */
