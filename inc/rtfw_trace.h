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

#ifndef RT_FRAMEWORK_INC_TRACE_H
#define RT_FRAMEWORK_INC_TRACE_H

#include <nuttx/config.h>

#include "rtfw_core.h"

#if defined(CONFIG_FTRACE) && (CONFIG_FTRACE == 1)
#include "ftrace_os.h"

#if (VCOS_MODULE_CONFIG_FTRACE_RESOURCE == 1)
#define RTFW_FTRACE_GET_RESOURCE(resource_id) \
	os_trace_get_resource(resource_id)
#define RTFW_FTRACE_RELEASE_RESOURCE(resource_id) \
	os_trace_release_resource(resource_id)
#else
#define RTFW_FTRACE_GET_RESOURCE(resource_id) ((void)0)
#define RTFW_FTRACE_RELEASE_RESOURCE(resource_id) ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_SPINLOCK == 1)
#define RTFW_FTRACE_GET_SPINLOCK(spinlock_id) \
	os_trace_get_spinlock(spinlock_id)
#define RTFW_FTRACE_RELEASE_SPINLOCK(spinlock_id) \
	os_trace_release_spinlock(spinlock_id)
#else
#define RTFW_FTRACE_GET_SPINLOCK(spinlock_id) ((void)0)
#define RTFW_FTRACE_RELEASE_SPINLOCK(spinlock_id) ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_DISABLE_ALL_INTERRUPT == 1)
#define RTFW_FTRACE_DISABLE_ALL_INTERRUPT() os_trace_disable_all_interrupt()
#define RTFW_FTRACE_ENABLE_ALL_INTERRUPT() os_trace_enable_all_interrupt()
#else
#define RTFW_FTRACE_DISABLE_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_ENABLE_ALL_INTERRUPT() ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_SUSPEND_ALL_INTERRUPT == 1)
#define RTFW_FTRACE_SUSPEND_ALL_INTERRUPT() os_trace_suspend_all_interrupt()
#define RTFW_FTRACE_RESUME_ALL_INTERRUPT() os_trace_resume_all_interrupt()
#else
#define RTFW_FTRACE_SUSPEND_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_RESUME_ALL_INTERRUPT() ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_SUSPEND_OS_INTERRUPT == 1)
#define RTFW_FTRACE_SUSPEND_OS_INTERRUPT() os_trace_suspend_os_interrupt()
#define RTFW_FTRACE_RESUME_OS_INTERRUPT() os_trace_resume_os_interrupt()
#else
#define RTFW_FTRACE_SUSPEND_OS_INTERRUPT() ((void)0)
#define RTFW_FTRACE_RESUME_OS_INTERRUPT() ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_SUSPEND_KERNEL == 1)
#define RTFW_FTRACE_SUSPEND_KERNEL() os_trace_suspend_kernel()
#define RTFW_FTRACE_RESUME_KERNEL() os_trace_resume_kernel()
#else
#define RTFW_FTRACE_SUSPEND_KERNEL() ((void)0)
#define RTFW_FTRACE_RESUME_KERNEL() ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_DISABLE_GLOBAL == 1)
#define RTFW_FTRACE_DISABLE_GLOBAL() os_trace_disable_global()
#define RTFW_FTRACE_ENABLE_GLOBAL() os_trace_enable_global()
#else
#define RTFW_FTRACE_DISABLE_GLOBAL() ((void)0)
#define RTFW_FTRACE_ENABLE_GLOBAL() ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_IRQ == 1)
#define RTFW_FTRACE_IRQ_HANDLER_ENTRY(isr_cfg) \
	os_trace_irq_handler_entry(isr_cfg)
#define RTFW_FTRACE_IRQ_HANDLER_EXIT(isr_cfg) \
	os_trace_irq_handler_exit(isr_cfg)
#else
#define RTFW_FTRACE_IRQ_HANDLER_ENTRY(isr_cfg) ((void)0)
#define RTFW_FTRACE_IRQ_HANDLER_EXIT(isr_cfg) ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_WAKEUP == 1)
#define RTFW_FTRACE_TASK_ACTIVATE(task_cfg) os_trace_sched_wakeup(task_cfg)
#define RTFW_FTRACE_TASK_RELEASE(task_cfg) os_trace_sched_wakeup(task_cfg)
#else
#define RTFW_FTRACE_TASK_ACTIVATE(task_cfg) ((void)0)
#define RTFW_FTRACE_TASK_RELEASE(task_cfg) ((void)0)
#endif

#if (VCOS_MODULE_CONFIG_FTRACE_REACTIVATE == 1)
#define RTFW_FTRACE_TASK_REACTIVATE(task_cfg) os_trace_reactivate()
#else
#define RTFW_FTRACE_TASK_REACTIVATE(task_cfg) ((void)0)
#endif

#else
#define RTFW_FTRACE_GET_RESOURCE(resource_id) ((void)0)
#define RTFW_FTRACE_RELEASE_RESOURCE(resource_id) ((void)0)
#define RTFW_FTRACE_GET_SPINLOCK(spinlock_id) ((void)0)
#define RTFW_FTRACE_RELEASE_SPINLOCK(spinlock_id) ((void)0)
#define RTFW_FTRACE_DISABLE_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_ENABLE_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_SUSPEND_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_RESUME_ALL_INTERRUPT() ((void)0)
#define RTFW_FTRACE_SUSPEND_OS_INTERRUPT() ((void)0)
#define RTFW_FTRACE_RESUME_OS_INTERRUPT() ((void)0)
#define RTFW_FTRACE_SUSPEND_KERNEL() ((void)0)
#define RTFW_FTRACE_RESUME_KERNEL() ((void)0)
#define RTFW_FTRACE_DISABLE_GLOBAL() ((void)0)
#define RTFW_FTRACE_ENABLE_GLOBAL() ((void)0)
#define RTFW_FTRACE_IRQ_HANDLER_ENTRY(isr_cfg) ((void)0)
#define RTFW_FTRACE_IRQ_HANDLER_EXIT(isr_cfg) ((void)0)
#define RTFW_FTRACE_TASK_ACTIVATE(task_cfg) ((void)0)
#define RTFW_FTRACE_TASK_RELEASE(task_cfg) ((void)0)
#define RTFW_FTRACE_TASK_REACTIVATE(task_cfg) ((void)0)
#endif

#ifndef OS_TRACE_EVENT
#define OS_TRACE_EVENT(task_cfg, mask) ((void)0)
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1)
#include "rtfw_arti.h"
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1) && \
	defined(CONFIG_ARTI_RESOURCE_TRACE) && \
	(CONFIG_ARTI_RESOURCE_TRACE == 1)
#define RTFW_ARTI_GET_RESOURCE(resource_id) \
	ARTI_TRACE(NOSUSP, AR_CP_OS_RESOURCE, Os, os_get_core_id(), \
		   OsResource_Got, resource_id)
#define RTFW_ARTI_RELEASE_RESOURCE(resource_id) \
	ARTI_TRACE(NOSUSP, AR_CP_OS_RESOURCE, Os, os_get_core_id(), \
		   OsResource_Released, resource_id)
#else
#define RTFW_ARTI_GET_RESOURCE(resource_id) ((void)0)
#define RTFW_ARTI_RELEASE_RESOURCE(resource_id) ((void)0)
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1) && \
	defined(CONFIG_ARTI_SPINLOCK_TRACE) && \
	(CONFIG_ARTI_SPINLOCK_TRACE == 1)
#define RTFW_ARTI_GET_SPINLOCK(spinlock_id) \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SPINLOCK, Os, os_get_core_id(), \
		   OsSpinlock_Locked, spinlock_id)
#define RTFW_ARTI_RELEASE_SPINLOCK(spinlock_id) \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SPINLOCK, Os, os_get_core_id(), \
		   OsSpinlock_Released, spinlock_id)
#else
#define RTFW_ARTI_GET_SPINLOCK(spinlock_id) ((void)0)
#define RTFW_ARTI_RELEASE_SPINLOCK(spinlock_id) ((void)0)
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1) && \
	defined(CONFIG_ARTI_INT_TRACE) && (CONFIG_ARTI_INT_TRACE == 1)
#define RTFW_ARTI_DISABLE_ALL_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_DisableAllInterrupts_Start, 0u)
#define RTFW_ARTI_ENABLE_ALL_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_DisableAllInterrupts_Return, 0u)
#define RTFW_ARTI_SUSPEND_ALL_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_SuspendAllInterrupts_Start, 0u)
#define RTFW_ARTI_RESUME_ALL_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_SuspendAllInterrupts_Return, 0u)
#define RTFW_ARTI_SUSPEND_OS_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_SuspendOSInterrupts_Start, 0u)
#define RTFW_ARTI_RESUME_OS_INTERRUPT() \
	ARTI_TRACE(NOSUSP, AR_CP_OS_SERVICECALLS, Os, os_get_core_id(), \
		   OsServiceCall_SuspendOSInterrupts_Return, 0u)
#else
#define RTFW_ARTI_DISABLE_ALL_INTERRUPT() ((void)0)
#define RTFW_ARTI_ENABLE_ALL_INTERRUPT() ((void)0)
#define RTFW_ARTI_SUSPEND_ALL_INTERRUPT() ((void)0)
#define RTFW_ARTI_RESUME_ALL_INTERRUPT() ((void)0)
#define RTFW_ARTI_SUSPEND_OS_INTERRUPT() ((void)0)
#define RTFW_ARTI_RESUME_OS_INTERRUPT() ((void)0)
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1) && \
	defined(CONFIG_ARTI_CAT2ISR_TRACE) && \
	(CONFIG_ARTI_CAT2ISR_TRACE == 1) && \
	defined(CONFIG_IRQ_USE_ARCH_DISPATCH)
#define RTFW_ARTI_IRQ_HANDLER_ENTRY(isr_cfg, entry_nesting) do { \
	*(entry_nesting) = rtfw_arti_cat2isr_enter(isr_cfg); \
} while (0)
#define RTFW_ARTI_IRQ_HANDLER_EXIT(isr_cfg) \
	rtfw_arti_cat2isr_exit(isr_cfg)
#define RTFW_ARTI_IRQ_HANDLER_COMPLETE(isr_cfg, entry_nesting) \
	rtfw_arti_cat2isr_complete(isr_cfg, entry_nesting)
#else
#define RTFW_ARTI_IRQ_HANDLER_ENTRY(isr_cfg, entry_nesting) \
	((void)(entry_nesting))
#define RTFW_ARTI_IRQ_HANDLER_EXIT(isr_cfg) ((void)0)
#define RTFW_ARTI_IRQ_HANDLER_COMPLETE(isr_cfg, entry_nesting) \
	((void)(entry_nesting))
#endif

#if defined(CONFIG_ARTI_ENABLED) && (CONFIG_ARTI_ENABLED == 1) && \
	defined(CONFIG_ARTI_TASK_TRACE) && (CONFIG_ARTI_TASK_TRACE == 1)
#define RTFW_ARTI_TASK_ACTIVATE(task_cfg) rtfw_arti_task_activate(task_cfg)
#define RTFW_ARTI_TASK_START(task_cfg) rtfw_arti_task_start(task_cfg)
#define RTFW_ARTI_TASK_TERMINATE(task_cfg) rtfw_arti_task_terminate(task_cfg)
#define RTFW_ARTI_TASK_REACTIVATE(task_cfg) rtfw_arti_task_reactivate(task_cfg)
#else
#define RTFW_ARTI_TASK_ACTIVATE(task_cfg) ((void)0)
#define RTFW_ARTI_TASK_START(task_cfg) ((void)0)
#define RTFW_ARTI_TASK_TERMINATE(task_cfg) ((void)0)
#define RTFW_ARTI_TASK_REACTIVATE(task_cfg) ((void)0)
#endif

#define OS_TRACE_GET_RESOURCE(resource_id) do { \
	RTFW_FTRACE_GET_RESOURCE(resource_id); \
	RTFW_ARTI_GET_RESOURCE(resource_id); \
} while (0)

#define OS_TRACE_RELEASE_RESOURCE(resource_id) do { \
	RTFW_FTRACE_RELEASE_RESOURCE(resource_id); \
	RTFW_ARTI_RELEASE_RESOURCE(resource_id); \
} while (0)

#define OS_TRACE_GET_SPINLOCK(spinlock_id) do { \
	RTFW_FTRACE_GET_SPINLOCK(spinlock_id); \
	RTFW_ARTI_GET_SPINLOCK(spinlock_id); \
} while (0)

#define OS_TRACE_RELEASE_SPINLOCK(spinlock_id) do { \
	RTFW_FTRACE_RELEASE_SPINLOCK(spinlock_id); \
	RTFW_ARTI_RELEASE_SPINLOCK(spinlock_id); \
} while (0)

#define OS_TRACE_TASK_ACTIVATE(task_cfg) do { \
	RTFW_FTRACE_TASK_ACTIVATE(task_cfg); \
	RTFW_ARTI_TASK_ACTIVATE(task_cfg); \
} while (0)

#define OS_TRACE_TASK_START(task_cfg) RTFW_ARTI_TASK_START(task_cfg)

#define OS_TRACE_TASK_RELEASE(task_cfg) RTFW_FTRACE_TASK_RELEASE(task_cfg)

#define OS_TRACE_TASK_TERMINATE(task_cfg) \
	RTFW_ARTI_TASK_TERMINATE(task_cfg)

#define OS_TRACE_TASK_REACTIVATE(task_cfg) do { \
	RTFW_FTRACE_TASK_REACTIVATE(task_cfg); \
	RTFW_ARTI_TASK_REACTIVATE(task_cfg); \
} while (0)

#define OS_TRACE_DISABLE_ALL_INTERRUPT() do { \
	RTFW_FTRACE_DISABLE_ALL_INTERRUPT(); \
	RTFW_ARTI_DISABLE_ALL_INTERRUPT(); \
} while (0)

#define OS_TRACE_ENABLE_ALL_INTERRUPT() do { \
	RTFW_FTRACE_ENABLE_ALL_INTERRUPT(); \
	RTFW_ARTI_ENABLE_ALL_INTERRUPT(); \
} while (0)

#define OS_TRACE_SUSPEND_ALL_INTERRUPT() do { \
	RTFW_FTRACE_SUSPEND_ALL_INTERRUPT(); \
	RTFW_ARTI_SUSPEND_ALL_INTERRUPT(); \
} while (0)

#define OS_TRACE_RESUME_ALL_INTERRUPT() do { \
	RTFW_FTRACE_RESUME_ALL_INTERRUPT(); \
	RTFW_ARTI_RESUME_ALL_INTERRUPT(); \
} while (0)

#define OS_TRACE_SUSPEND_OS_INTERRUPT() do { \
	RTFW_FTRACE_SUSPEND_OS_INTERRUPT(); \
	RTFW_ARTI_SUSPEND_OS_INTERRUPT(); \
} while (0)

#define OS_TRACE_RESUME_OS_INTERRUPT() do { \
	RTFW_FTRACE_RESUME_OS_INTERRUPT(); \
	RTFW_ARTI_RESUME_OS_INTERRUPT(); \
} while (0)

#define OS_TRACE_IRQ_HANDLER_ENTRY(isr_cfg, entry_nesting) do { \
	RTFW_FTRACE_IRQ_HANDLER_ENTRY(isr_cfg); \
	RTFW_ARTI_IRQ_HANDLER_ENTRY(isr_cfg, entry_nesting); \
} while (0)

#define OS_TRACE_IRQ_HANDLER_EXIT(isr_cfg) do { \
	RTFW_FTRACE_IRQ_HANDLER_EXIT(isr_cfg); \
	RTFW_ARTI_IRQ_HANDLER_EXIT(isr_cfg); \
} while (0)

#define OS_TRACE_IRQ_HANDLER_COMPLETE(isr_cfg, entry_nesting) \
	RTFW_ARTI_IRQ_HANDLER_COMPLETE(isr_cfg, entry_nesting)

#define OS_TRACE_SUSPEND_KERNEL() RTFW_FTRACE_SUSPEND_KERNEL()
#define OS_TRACE_RESUME_KERNEL() RTFW_FTRACE_RESUME_KERNEL()
#define OS_TRACE_DISABLE_GLOBAL() RTFW_FTRACE_DISABLE_GLOBAL()
#define OS_TRACE_ENABLE_GLOBAL() RTFW_FTRACE_ENABLE_GLOBAL()

#endif /* RT_FRAMEWORK_INC_TRACE_H */