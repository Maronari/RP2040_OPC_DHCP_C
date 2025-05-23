#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <FreeRTOS.h>

/* Use Pico SDK ISR handlers */
#define vPortSVCHandler isr_svcall
#define xPortPendSVHandler isr_pendsv
#define xPortSysTickHandler isr_systick

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      133000000
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    10
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 256
#define configTOTAL_HEAP_SIZE                   (100 * 1024)
#define configMAX_TASK_NAME_LEN                 32
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* System */
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* SMP port only */
#define configNUM_CORES                         1
#define configTICK_CORE                         0
#define configRUN_MULTIPLE_PRIORITIES           0
#define configUSE_CORE_AFFINITY                 0

/* RP2040 specific */
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           1
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

#include <assert.h>
/* Define to trap errors during development. */
#define configASSERT(x)                         assert(x)

/* Interrupt nesting behaviour configuration. */
#define configPRIO_BITS 4
#define configKERNEL_INTERRUPT_PRIORITY         0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    1
#define configMAX_API_CALL_INTERRUPT_PRIORITY   1

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1                
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1

#define configENABLE_MPU                        1
#define configSYSTEM_CALL_STACK_SIZE            256

#define configRECORD_STACK_HIGH_ADDRESS         1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 
extern uint64_t                                 time_us_64(void);				                // "hardware/timer.h"
#define RUN_TIME_STAT_time_us_64Divider         1000			                                // stat granularity is mS
#define portGET_RUN_TIME_COUNTER_VALUE()        (time_us_64()/RUN_TIME_STAT_time_us_64Divider)	// runtime counter in mS     /* Define this to sample the timer/counter */

#define portGET_CORE_ID()                       get_core_num()
#define portCHECK_IF_IN_ISR()                   ({ \
                                                uint32_t ulIPSR; \
                                                __asm volatile ("mrs %0, IPSR" : "=r" (ulIPSR)::); \
                                                ((uint8_t)ulIPSR)>0;})

#endif /* FREERTOS_CONFIG_H */
