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

#ifndef RT_FRAMEWORK_INC_OS_ALARM_TYPES_H
#define RT_FRAMEWORK_INC_OS_ALARM_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! The data type of alarms which set events. */
struct os_alarm_set_event;
typedef struct os_alarm_set_event os_alarm_set_event_t;

/*! The data type of alarms action. */
union os_alarm_action_attr;
typedef union os_alarm_action_attr os_alarm_action_attr_t;

/*! The data type of alarms autostart. */
struct os_alarm_autostart;
typedef struct os_alarm_autostart os_alarm_autostart_t;

/*! The data type of alarm Instance Description */
struct os_alarm_inst;
typedef struct os_alarm_inst os_alarm_inst_t;

/*! The data type of alarm configuration */
struct os_alarm_cfg;
typedef struct os_alarm_cfg os_alarm_cfg_t;

#ifdef __cplusplus
}
#endif

#endif	 /* RT_FRAMEWORK_INC_OS_ALARM_TYPES_H */
