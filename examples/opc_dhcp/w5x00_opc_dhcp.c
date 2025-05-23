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
#include "timer.h"

#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/apps/sntp.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"

#include "hardware/rtc.h"
#include "hardware/adc.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "pico/lwip_freertos.h"
#include "pico/async_context_freertos.h"

#include "open62541.h"
#include "opc_add_temperature.h"
#include "opc_freertos_status.h"

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
#define DHCP_TASK_STACK_SIZE 1024
#define DHCP_TASK_PRIORITY 2

#define OPC_TASK_STACK_SIZE 15*1024
#define OPC_TASK_PRIORITY 4

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

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

/* LWIP */
struct netif g_netif;
async_context_freertos_t asyncContextFreertos;
async_context_t asyncContext;

/* Timer */
static volatile uint32_t g_msec_cnt = 0;
static datetime_t system_time = {};

/* FreeRTOS Tasks' handles */
TaskHandle_t spi_handle_t = NULL;
TaskHandle_t opc_handle_t = NULL;
TaskHandle_t temp_sensor_handle_t = NULL;

/* Temperature sensor */
float g_current_temperature = 0.0f;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* FreeRTOS Tasks */
static void spi_task(void *argument);
static void opc_task(void *argument);
static void temperature_task(void *argument);

/* Other */
static void netif_config(void);
static void s_command_handler(const TaskHandle_t xTask);
void set_system_time(uint32_t s);
uint32_t get_system_time(void);
float read_temperature();

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
    timer_hw->dbgpause = 0;
    stdio_init_all();

    sleep_ms(1000 * 3); // wait for 3 seconds

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    // Initialize LWIP task in NO_SYS=0 mode
    async_context_freertos_init_with_defaults(&asyncContextFreertos);
    lwip_freertos_init(&asyncContextFreertos.core);
    netif_config();

    // Initialize Real Time Clock
    rtc_init();

    // Initialize ADC and Temperature sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(0);
    
    // Initialize LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    if (pdPASS != xTaskCreate(spi_task, "SPI_Task", DHCP_TASK_STACK_SIZE, NULL, DHCP_TASK_PRIORITY, &spi_handle_t))
    {
        printf("[DHCP]\t\tError creating task - couldn't allocate required memory\n");
    }
    if (pdPASS != xTaskCreate(opc_task, "OPC_Task", OPC_TASK_STACK_SIZE, NULL, OPC_TASK_PRIORITY, &opc_handle_t))
    {
        printf("[OPC UA]\tError creating task - couldn't allocate required memory\n");
    }
    if (pdPASS != xTaskCreate(temperature_task, "TEMP_Task", 2048, NULL, 5, &temp_sensor_handle_t))
    {
        printf("[TEMPERATURE]\tError creating task - couldn't allocate required memory\n");
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
static void netif_config(void)
{
    int8_t retval = 0;

    // Set ethernet chip MAC address
    setSHAR(mac);
    ctlwizchip(CW_RESET_PHY, 0);

    netif_add(&g_netif, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, NULL, netif_initialize, tcpip_input);
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

    // Set the default interface and bring it up
    netif_set_default(&g_netif);
    netif_set_link_up(&g_netif);
    netif_set_up(&g_netif);

    dhcp_start(&g_netif);
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

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    printf("[FreeRTOS] Application stack overflow: %s\n", pcTaskName);
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void set_system_time(uint32_t seconds)
{
    uint32_t a, b, c, d, e, f;

    int8_t _gmt = 3;
    int h, j, k;
    seconds += _gmt * 3600ul;
    system_time.sec = seconds % 60ul;
    seconds /= 60;
    system_time.min = seconds % 60ul;
    seconds /= 60;
    system_time.hour = seconds % 24ul;
    seconds /= 24;
    a = (uint32_t)((4ul * seconds + 102032) / 146097 + 15);
    b = (uint32_t)(seconds + 2442113 + a - (a / 4));
    c = (20 * b - 2442) / 7305;
    d = b - 365 * c - (c / 4);
    e = d * 1000 / 30601;
    f = d - e * 30 - e * 601 / 1000;
    if (e <= 13)
    {
        c -= 4716;
        e -= 1;
    }
    else
    {
        c -= 4715;
        e -= 13;
    }
    system_time.year = c;
    system_time.month = e;
    system_time.day = f;
    if (e <= 2)
    {
        e += 12;
        c -= 1;
    }
    j = c / 100;
    k = c % 100;
    h = f + (26 * (e + 1) / 10) + k + (k / 4) + (5 * j) + (j / 4); // Zeller's congruence
    system_time.dotw = ((h + 5) % 7) + 1;

    rtc_set_datetime(&system_time);
}

uint32_t get_system_time(void)
{
    rtc_get_datetime(&system_time);

    int8_t my = (system_time.month >= 3) ? 1 : 0;
    uint16_t y = system_time.year + my - 1970;
    uint16_t dm = 0;
    int8_t _gmt = 3;

    for (int i = 0; i < system_time.month - 1; i++)
    {
        dm += (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
    }

    uint32_t seconds = (((system_time.day - 1 + \
            dm + ((y + 1) >> 2) - ((y + 69) / 100) + \
            ((y + 369) / 100 / 4) + \
            365 * (y - my)) * 24ul + \
            system_time.hour - _gmt) * 60ul + \
            system_time.min) * 60ul + \
            system_time.sec;
    return seconds;
}

float read_temperature()
{
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (4096 - 1);
    float result = raw * conversion_factor;
    if (adc_get_selected_input() == 4)
    {
        
        float temp = 27 - (result - 0.706)/0.001721;
        return temp;
    }
    else
    {
        float temp = (result - 1.25)/0.005;
        return temp;
    }
}

static void s_command_handler(const TaskHandle_t xTask)
{
    // Show the size of the free heap.
    // Calling this function frequently will show if there is a memory leak.
    // printf("[FreeRTOS]\t\t");
    // printf("Heap Size = %d; ", xPortGetFreeHeapSize());

    // // Show the uptime of the task in seconds. FreeRTOS counts ticks.
    // printf("Uptime = %ds\n", xTaskGetTickCount() / 1000);

    TaskStatus_t xTaskStatus; // Создаем струкуру под статус.

    // Reading the task data by its title
    vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eInvalid);

    // Output information about the task.

    printf("[FreeRTOS]");
    // The name of the task.
    printf("[%s]\t", xTaskStatus.pcTaskName);

    // Show the current status of the task.
    // 0 - running, 1 - waiting to start, ready, 2 - blocked (waiting for something on a timer),
    // 3 - euthanized (blocked with an infinite waiting time for the condition),
    // 4 - deleted, 5 - error.
    printf("Status: %d; ", xTaskStatus.eCurrentState);

    // Show the current priority of the task.
    printf("Priority: %d; ", xTaskStatus.uxCurrentPriority);

    // Show the largest stack fill in the entire execution history. I.e. the top mark.
    // How much free space is left in the worst case.
    printf("Task stack high water mark (freespace): %d\n", xTaskStatus.usStackHighWaterMark);
}

/**
 * ----------------------------------------------------------------------------------------------------
 * FreeRTOS Task Functions
 * ----------------------------------------------------------------------------------------------------
 */
void spi_task(void *argument)
{
    uint16_t pack_len = 0;
    struct pbuf *p = NULL;
    uint8_t *pack = malloc(ETHERNET_MTU);

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
                printf("[WIZ]\t\tNo packet received\n");
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
    }
}

void opc_task(void *argument)
{
    UA_Boolean running = true;
    UA_StatusCode retval;
    // Allows to set smaller buffer for the connections, which can cause problems
    UA_UInt32 sendBufferSize = 16000;
    UA_UInt32 recvBufferSize = 16000;
    UA_UInt16 portNumber = 4840;

    // Start DHCP configuration for an interface
    while ((g_netif.ip_addr.addr == 0) && (g_netif.netmask.addr == 0))
    {
        vTaskDelay(1000);
    }
    sntp_init();
    while (get_system_time() <= 1704056400) //Sun Dec 31 2023 21:00:00 GMT+0000
    {
        vTaskDelay(1000);
    }

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    retval = UA_ServerConfig_setMinimalCustomBuffer(config, portNumber, 0, sendBufferSize, recvBufferSize);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "tUA_ServerConfig_setMinimalCustomBuffer() Status: 0x%x (%s)\n", retval, UA_StatusCode_name(retval));
    }

    UA_String UA_hostname = UA_STRING(ip4addr_ntoa(netif_ip4_addr(&g_netif)));

    UA_String_clear(&config->customHostname);
    UA_String_copy(&UA_hostname, &config->customHostname);

    s_command_handler(opc_handle_t);

    // add a variable node to the adresspace
    addTempVariable(server);
    addGetHeapStatsVariable(server);
    addTaskStatsFolder(server);
    // addTaskStatsVariable(server, opc_handle_t);
    // addTaskStatsVariable(server, spi_handle_t);
    // addTaskStatsVariable(server, temp_sensor_handle_t);
    // addTaskStatsVariable(server, xTaskGetHandle("TCPIP_Task"));

    retval = UA_Server_run(server, &running);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_Server_run() Status: 0x%x (%s)\n", retval, UA_StatusCode_name(retval));
    }
    UA_Server_delete(server);
    UA_ServerConfig_clean(config);
}

void temperature_task(void *argument)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
        float temp = read_temperature();
        g_current_temperature = temp;

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}