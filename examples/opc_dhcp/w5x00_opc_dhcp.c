/**
 * Copyright (c) 2022 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdio.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "socket.h"
#include "w5x00_spi.h"
#include "w5x00_lwip.h"

#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

#include "lwip/apps/lwiperf.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "timer.h"

#include "open62541.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Socket */
#define SOCKET_MACRAW 0

/* Port */
#define PORT_LWIPERF 5001

/* Task */
#define DHCP_TASK_STACK_SIZE 256
#define DHCP_TASK_PRIORITY 2

#define OPC_TASK_STACK_SIZE 4096
#define OPC_TASK_PRIORITY 1

/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_DHCP 0
#define SOCKET_DNS 3

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define DNS_RETRY_COUNT 5

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
extern uint8_t mac[6];
static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};

/* LWIP */
struct netif g_netif;

/* DNS */
static uint8_t g_dns_target_domain[] = "www.wiznet.io";

static uint8_t g_dns_get_ip_flag = 0;
static uint32_t g_ip;
static ip_addr_t g_resolved;

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* Semaphore */
static xSemaphoreHandle dns_sem = NULL;

/* Timer */
static volatile uint32_t g_msec_cnt = 0;

/* FreeRTOS */
TaskHandle_t dhcp_handle_t = NULL;
TaskHandle_t opc_handle_t = NULL;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* Task */
void dhcp_task(void *argument);
void dns_task(void *argument);
static void opc_task(void *argument);

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    set_clock_khz();

    // Initialize stdio after the clock change
    // TODO: uncomment when release
    stdio_init_all();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    printf("FreeRTOS Init\n");
    if (pdPASS == xTaskCreate(dhcp_task, "DHCP_Task", DHCP_TASK_STACK_SIZE, NULL, DHCP_TASK_PRIORITY, &dhcp_handle_t))
    {
        printf("[DHCP] Task create\n");
    }
    else
    {
        printf("[DHCP] Error creating task - couldn't allocate required memory\n");
    }
    if (pdPASS == xTaskCreate(opc_task, "OPC_Task", OPC_TASK_STACK_SIZE, NULL, OPC_TASK_PRIORITY, &opc_handle_t))
    {
        printf("[OPC UA] Task create\n");
    }
    else
    {
        printf("[OPC UA] Error creating task - couldn't allocate required memory\n");
    }

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
void dhcp_task(void *argument)
{
    int8_t retval = 0;
    uint8_t *pack = malloc(ETHERNET_MTU);
    uint16_t pack_len = 0;
    struct pbuf *p = NULL;

    // Set ethernet chip MAC address
    setSHAR(mac);
    ctlwizchip(CW_RESET_PHY, 0);

    // Initialize LWIP in NO_SYS mode
    lwip_init();

    netif_add(&g_netif, &IP_ADDR_ANY->u_addr.ip4, &IP_ADDR_ANY->u_addr.ip4, &IP_ADDR_ANY->u_addr.ip4, NULL, netif_initialize, netif_input);
    g_netif.name[0] = 'e';
    g_netif.name[1] = '0';

    // Assign callbacks for link and status
    netif_set_link_callback(&g_netif, netif_link_callback);
    netif_set_status_callback(&g_netif, netif_status_callback);

    // MACRAW socket open
    retval = socket(SOCKET_MACRAW, Sn_MR_MACRAW, PORT_LWIPERF, 0x00);

    if (retval < 0)
    {
        printf("[DHCP] MACRAW socket open failed\n");
    }

    if (retval < 0)
    {
        printf("[DHCP] MACRAW socket open failed\n");
    }

    // Set the default interface and bring it up
    netif_set_default(&g_netif);
    netif_set_link_up(&g_netif);
    netif_set_up(&g_netif);

    // Start DHCP configuration for an interface
    dhcp_start(&g_netif);

    dns_init();

    while (1)
    {
        getsockopt(SOCKET_MACRAW, SO_RECVBUF, &pack_len);

        if (pack_len > 0)
        {
            pack_len = recv_lwip(SOCKET_MACRAW, (uint8_t *)pack, pack_len);

            if (pack_len)
            {
                p = pbuf_alloc(PBUF_RAW, pack_len, PBUF_POOL);
                pbuf_take(p, pack, pack_len);
                free(pack);

                pack = malloc(ETHERNET_MTU);
            }
            else
            {
                printf("[DHCP] No packet received\n");
            }

            if (pack_len && p != NULL)
            {
                LINK_STATS_INC(link.recv);

                if (g_netif.input(p, &g_netif) != ERR_OK)
                {
                    pbuf_free(p);
                }
            }
        }

        if ((g_netif.ip_addr.u_addr.ip4.addr > 0) && (g_netif.netmask.u_addr.ip4.addr > 0) && (g_dns_get_ip_flag != 1))
        {
            g_dns_get_ip_flag = 1;
            printf("[DHCP] task end\n");
            vTaskSuspend(NULL);
        }

        /* Cyclic lwIP timers check */
        sys_check_timeouts();
    }
}

void opc_task(void *argument)
{
    UA_Boolean running = true;
    UA_StatusCode retval;
    // Allows to set smaller buffer for the connections, which can cause problems
    UA_UInt32 sendBufferSize = 16000; // 64 KB was too much for my platform
    UA_UInt32 recvBufferSize = 16000; // 64 KB was too much for my platform
    UA_UInt16 portNumber = 4840;

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode configStatus = UA_ServerConfig_setMinimalCustomBuffer(config, portNumber, 0, sendBufferSize, recvBufferSize);
    if(configStatus != UA_STATUSCODE_GOOD)
    {
        printf("[OPC UA] Error to create new server config");
        while (1)
        {
        }
    }
    // VERY IMPORTANT: Set the hostname with your IP before allocating the server
    while (g_dns_get_ip_flag != 1)
    {
        vTaskDelay(50);
    }
    printf("[OPC UA] server IP: %s\n", ip4addr_ntoa(netif_ip4_addr(&g_netif)));
    UA_String UA_hostname = UA_STRING(ip4addr_ntoa(netif_ip4_addr(&g_netif)));

    // UA_String_clear(&config->customHostname);
    UA_String_copy(&UA_hostname, &config->customHostname);

    // The rest is the same as the example

    // add a variable node to the adresspace
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode status = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                     parentReferenceNodeId, myIntegerName,
                                                     UA_NODEID_NULL, attr, NULL, NULL);

    if (status != UA_STATUSCODE_GOOD)
    {
        printf("UA_Server_addVariableNode() Status: 0x%x\n", status);
    }

    /* allocations on the heap need to be freed */
    UA_VariableAttributes_clear(&attr);
    UA_NodeId_clear(&myIntegerNodeId);
    UA_QualifiedName_clear(&myIntegerName);
    retval = UA_Server_run(server, &running);
    if (retval == UA_STATUSCODE_GOOD)
    {
        printf("[OPC UA] Server successfully running on %s:%d", config->customHostname.data, portNumber);
    }

    UA_Server_delete(server);
    // UA_ServerConfig_delete(config);
}

static void s_command_handler(const char *arg)
{
    // Показать размер свободной кучи.
    // Частый вызов этой функции позволит показать есть ли утечка памяти.
    // У меня, поскольку все вызывается один раз, то куча принимает один и тот же размер
    // И не меняется динамически. А вот если уменьшается, то спустя какое то время получим переполнение
    // и все повиснет.
    printf("Heap Size = %d\n", xPortGetFreeHeapSize());

    // Показать аптайм задачи в секундах. FreeRTOS считает тики, а у меня они на 1мс записаны.
    printf("Uptime = %ds\n",xTaskGetTickCount()/1000);
 
    TaskStatus_t xTaskStatus;  // Создаем струкуру под статус. 			
 
// Читаем получаем заголовок нужной задачи. В качестве параметра идет текстовая метка задачи. Помните как создается задача? 
// xTaskCreate(vBlinker2,"B", configMINIMAL_STACK_SIZE+512, NULL, tskIDLE_PRIORITY + 10, NULL);
// Вот второй параметр "B" это та самая метка и есть. По этой метке функция xTaskGetHandle найдет заголовок нужной задачи. 
    TaskHandle_t xTask = xTaskGetHandle(arg); 
 
// Читаем данные задачи по ее заголовку
    vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eInvalid);
 
// Выводим информацию о задаче
 
// Имя задачи. Очень важно выводить Т.к. чуть ошибешься в написании и оно выведет не то. 
// А тут можно проконтроллировать туда ли мы смотрим. 
    printf("Task name: %s\n", xTaskStatus.pcTaskName);  
 
// Показать текущее состояние задачи. 
// 0 - запущена, 1 - ожидает запуска, готова, 2 - заблокирована (ждет чего то по таймеру), 
// 3 - усыплена (заблокирована с бесконечным временем ожидания условия),
// 4 - удалена, 5 - ошибка. 
    printf("Task status: %d\n", xTaskStatus.eCurrentState);
 
// Показать текущий приоритет задачи. 
    printf("Task priority: %d\n", xTaskStatus.uxCurrentPriority);
 
// Показать наибольшее заполнение стека за всю историю выполнения. Т.е. верхняя отметка.
// Сколько осталось свободного места в худшем случае. 
    printf("Task stack high water mark (freespace): %d\n", xTaskStatus.usStackHighWaterMark);
}

/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

void vApplicationMallocFailedHook()
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // ( void ) pcTaskName;
    // ( void ) pxTask;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
