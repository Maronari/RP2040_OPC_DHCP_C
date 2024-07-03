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
#define DHCP_TASK_STACK_SIZE 2048
#define DHCP_TASK_PRIORITY 8

#define DNS_TASK_STACK_SIZE 512
#define DNS_TASK_PRIORITY 10

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

/* Task */
#define DHCP_TASK_STACK_SIZE 2048
#define DHCP_TASK_PRIORITY 8

#define DNS_TASK_STACK_SIZE 512
#define DNS_TASK_PRIORITY 10

#define OPC_TASK_STACK_SIZE 512
#define OPC_TASK_PRIORITY 10

/* Open6241 */
#define UA_ARCHITECTURE_FREERTOSLWIP 1

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_DHCP                         // DHCP enable/disable
};
extern uint8_t mac[6];
static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};

/* LWIP */
struct netif g_netif;

/* DNS */
static uint8_t g_dns_target_domain[] = "www.wiznet.io";
static uint8_t g_dns_target_ip[4] = {
    0,
};
static uint8_t g_dns_get_ip_flag = 0;
static uint32_t g_ip;
static ip_addr_t g_resolved;

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

/* Semaphore */
static xSemaphoreHandle dns_sem = NULL;

/* Timer  */
static volatile uint32_t g_msec_cnt = 0;

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
    stdio_init_all();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    //8000 is the stack size and 8 is priority. This values might need to be changed according to your project

    xTaskCreate(dhcp_task, "DHCP_Task", DHCP_TASK_STACK_SIZE, NULL, DHCP_TASK_PRIORITY, NULL);
    // xTaskCreate(dns_task, "DNS_Task", DNS_TASK_STACK_SIZE, NULL, DNS_TASK_PRIORITY, NULL);
    xTaskCreate(opc_task, "OPC_Task", OPC_TASK_STACK_SIZE, NULL, OPC_TASK_PRIORITY, NULL);

    // if(NULL == sys_thread_new("opcua_thread", opc_task, NULL, 8000, 8))
    //     LWIP_ASSERT("opcua(): Task creation failed.", 0);

    dns_sem = xSemaphoreCreateCounting((unsigned portBASE_TYPE)0x7fffffff, (unsigned portBASE_TYPE)0);

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

    netif_add(&g_netif, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY, NULL, netif_initialize, netif_input);
    g_netif.name[0] = 'e';
    g_netif.name[1] = '0';

    // Assign callbacks for link and status
    netif_set_link_callback(&g_netif, netif_link_callback);
    netif_set_status_callback(&g_netif, netif_status_callback);

    // MACRAW socket open
    retval = socket(SOCKET_MACRAW, Sn_MR_MACRAW, PORT_LWIPERF, 0x00);

    if (retval < 0)
    {
        printf(" MACRAW socket open failed\n");
    }

    if (retval < 0)
    {
        printf(" MACRAW socket open failed\n");
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
                printf(" No packet received\n");
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

        if ((dns_gethostbyname(g_dns_target_domain, &g_resolved, NULL, NULL) == ERR_OK) && (g_dns_get_ip_flag == 0))
        {
            g_ip = g_resolved.u_addr.ip4.addr;

            printf(" DNS success\n");
            printf(" Target domain : %s\n", g_dns_target_domain);
            printf(" IP of target domain : [%03d.%03d.%03d.%03d]\n", g_ip & 0xFF, (g_ip >> 8) & 0xFF, (g_ip >> 16) & 0xFF, (g_ip >> 24) & 0xFF);

            g_dns_get_ip_flag = 1;
        }

        /* Cyclic lwIP timers check */
        sys_check_timeouts();
    }
}

static void opc_task(void *argument)
{
    UA_Server *OPC_Server = UA_Server_new();
    UA_ServerConfig *OPC_Server_Config = UA_Server_getConfig(OPC_Server);

    UA_ServerConfig_setMinimalCustomBuffer(OPC_Server_Config, 4000, 0, (UA_UInt32)8192, (UA_UInt32)8192);

    //задайте IP-адрес сервера
    UA_ServerConfig_setCustomHostname(OPC_Server_Config, UA_STRING((char *)"192.168.0.10"));

    //установите для переменной running значение True
    UA_Boolean running = true;

    //запустите сервер
    UA_StatusCode retval = UA_Server_run(OPC_Server, &running);
    UA_Server_delete(OPC_Server);
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
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
