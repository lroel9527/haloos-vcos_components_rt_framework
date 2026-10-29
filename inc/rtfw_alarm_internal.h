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

#ifndef RT_FRAMEWORK_INC_OS_ALARM_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_ALARM_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_alarm_types.h"
#include "rtfw_counter_internal.h"
#include "rtfw_compiler_cfg.h"
#include "rtfw_core_internal.h"

typedef void (* alarm_callback_function_t)(void);
typedef alarm_callback_function_t alarm_callback_type;

/* Global Enumeration Definition */
typedef enum {
	ALARM_ACTION_ACTIVATETASK = 0,
	ALARM_ACTION_SETEVENT,
	ALARM_ACTION_ALARMCALLBACK,                      /*! Only class 1 */
	ALARM_ACTION_CNT,
} os_alarm_action_id_type;

/*!
 * Configuration start mode information of alarms.
 */
typedef enum {
	OS_ALARM_START_ABSOLUTE,                         /*! Start with SetAbsAlarm() */
	OS_ALARM_START_RELATIVE,                         /*! Start with SetRelAlarm() */
} os_alarm_start_mode_type;

/* Global Type & Structure Definition */

/*!
 * Alarms state.
 */
typedef enum {
	OS_ALARMSTATE_DISABLE,                           /*! Alarm disabled state */
	OS_ALARMSTATE_ENABLE                             /*! Alarm enabled state */
} os_alarm_state_type;

/*!
 * Configuration information of alarms, which set events.
 */
struct os_alarm_set_event {
	/*! The task, who's event has to be set. */
	TaskType task;
	/*! The events, to be set. */
	EventMaskType mask;
};

/*!
 * Configuration action information of alarms.
 */
union os_alarm_action_attr {
	/*! The task, who's event has to be set. */
	os_alarm_set_event_t event;
	/*! The task, to be activated. */
	TaskType task;
	/*! The callback function, to be called. */
	alarm_callback_type callback_function;
};

/* STD container : OsAlarmAutostart
 * OsAlarmAlarmTime:            1    Int
 * OsAlarmAutoStartType:        1    Int, ABSOLUTE, RELATIVE
 * OsAlarmCycleTime:            1    Int
 * OsAlarmAppModeRef:           1..*
 */
struct os_alarm_autostart {
	TickType alarm_time;                             /*! Alarm first expiration time */
	TickType cycle_time;                             /*! Alarm cycle execution time */
	os_alarm_start_mode_type autostart_type;         /*! Alarm autostart type */
	AppModeType autostart_mode;                      /*! Autostart mode */
};

/* STD container : OsAlarm
 * OsAlarmAccessingApplication: 0..* Ref to OS application
 * OsAlamCounterRef:            1    Ref to counter
 * OsAlarmAction[C]             1    Action when alarm expires
 * OsAlarmAutostart[C]          0..1 Autostart
 */

/*! Alarm dynamic data */
struct os_alarm_inst {
	struct hrtimer_s hrtimer;                        /*! High resolution timer for alarm expiration */
	TickType cycle;                                  /*! Cycle period */
	os_alarm_state_type alarm_state;                 /*! Alarm state */
};

/*! Alarm configuration by user */
struct os_alarm_cfg {
	os_alarm_inst_t *obj_desc;                      /*! Object instance description */
	AlarmType alarm_id;                              /*! Alarm identifier */
	const char *alarm_name;                          /*! Alarm name */
	os_alarm_action_id_type action_type;             /*! Alarm action type */
	os_alarm_action_attr_t action_attr;              /*! Action attributes when alarm expires */
	os_action_cfg_t action_config;                   /*! Action Config, Use for Action Priority queue */
	os_alarm_autostart_t autostart;                  /*! Autostart configuration */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	ApplicationIDType app_owner_id;                  /*! Application ID that the alarm belongs to */
	uint32 access_app_mask;                          /*! Application access permissions */
#endif
};

/* Global Macro Definition */
DeclareRtfwRefTable_H(alarm)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/*!
 * \brief           Alarm initialization
 * \details         The operating system initializes all Alarms during initialization. If the configured mode is
 *                  autostart mode, this function activates the Alarm and sets all dynamic parameters of the
 *                  given Alarm to its configured startup settings.
 *
 * \param[in,out]   os_alarm_cfg_table Alarm configuration table to be initialized
 * \param[in]       size Size of the Alarm table, i.e., the number of Alarms
 *
 * \return          None
 *
 * \context         OS_INTERNAL
 *
 * \reentrant       TRUE
 *
 * \synchronous     TRUE
 */
extern FUNC(void, OS_CODE) os_alarm_init(const os_alarm_cfg_t * const os_alarm_cfg_table[], OsObjIndexType size);

extern FUNC(os_status_t, OS_CODE) os_get_alarm(AlarmType alarm_id, TickRefType Tick);
extern FUNC(os_status_t, OS_CODE) os_get_alarm_base( AlarmType alarm_id, AlarmBaseRefType Info );
extern FUNC(os_status_t, OS_CODE) os_set_rel_alarm(AlarmType alarm_id, TickType Increment, TickType cycle);
extern FUNC(os_status_t, OS_CODE) os_set_abs_alarm(AlarmType alarm_id, TickType Start, TickType cycle);
extern FUNC(os_status_t, OS_CODE) os_cancel_alarm(AlarmType alarm_id);
extern FUNC(void, OS_CODE) os_alarm_kill(const os_alarm_cfg_t * const alarm_cfg);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
#ifdef __cplusplus
}
#endif

#endif	 /* RT_FRAMEWORK_INC_OS_ALARM_INTERNAL_H */
