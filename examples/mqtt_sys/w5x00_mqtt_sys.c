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
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/apps/mqtt.h"
#include "lwip/tcpip.h"

#include "hardware/adc.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "pico/lwip_freertos.h"
#include "pico/async_context_freertos.h"

#include <string.h>

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

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Retry count */
#define DHCP_RETRY_COUNT 5
#define DNS_RETRY_COUNT 5

/* MQTT */
#define MQTT_BROKER "192.168.1.108"

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
TaskHandle_t temp_sensor_handle_t = NULL;

/* Temperature sensor */
float g_current_temperature = 0.0f;

/* MQTT */
static mqtt_client_t *mqtt_client = NULL;
static bool mqtt_connected = false;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* FreeRTOS Tasks */
static void spi_task(void *argument);
static void temperature_task(void *argument);
void mqtt_pub_task(void *argument);

/* Other */
static void netif_config(void);
static void s_command_handler(const TaskHandle_t xTask);
void set_system_time(uint32_t s);
uint32_t get_system_time(void);
float read_temperature();

/* MQTT */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_pub_request_cb(void *arg, err_t result);
void init_mqtt();

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
    if (pdPASS != xTaskCreate(mqtt_pub_task, "MQTT_Task", 2048, NULL, 5, NULL))
    {
        printf("[MQTT]\tError creating task - couldn't allocate required memory\n");
    }
    if (pdPASS != xTaskCreate(temperature_task, "TEMP_Task", 2048, NULL, 5, &temp_sensor_handle_t))
    {
        printf("[TEMPERATURE]\tError creating task - couldn't allocate required memory\n");
    }

    vTaskStartScheduler();

    while (1)
    {

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

float read_temperature()
{
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (4096 - 1);
    float result = raw * conversion_factor;
    if (adc_get_selected_input() == 4)
    {

        float temp = 27 - (result - 0.706) / 0.001721;
        return temp;
    }
    else
    {
        float temp = (result - 1.25) / 0.005;
        return temp;
    }
}

void temperature_task(void *argument)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        float temp = read_temperature();
        g_current_temperature = temp;

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void mqtt_pub_task(void *argument)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if ((g_netif.ip_addr.addr > 0) && (g_netif.netmask.addr > 0) && (g_netif.gw.addr > 0) && !mqtt_client)
        {
            init_mqtt();
        }

        if (mqtt_connected && mqtt_client)
        {
            char pub_payload[10];
            sprintf(pub_payload, "%f", g_current_temperature);
            err_t err;
            u8_t qos = 0;    /* 0 1 or 2, see MQTT specification */
            u8_t retain = 0; /* No don't retain such crappy payload... */
            err = mqtt_publish(mqtt_client, "pico", pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, NULL);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }

}

void init_mqtt()
{
    err_t err;
    ip_addr_t broker_addr;
    if (!ipaddr_aton(MQTT_BROKER, &broker_addr))
    {
        printf("Invalid broker IP\n");
        return;
    }
    // struct eth_addr pc_mac;
    // pc_mac.addr[0] = 0x8C;
    // pc_mac.addr[1] = 0xC6;
    // pc_mac.addr[2] = 0x81;
    // pc_mac.addr[3] = 0x87;
    // pc_mac.addr[4] = 0x06;
    // pc_mac.addr[5] = 0xE5;

    // err = etharp_add_static_entry(&broker_addr, &pc_mac);
    // if (err != ERR_OK)
    // {
    //     printf("Failed to add ARP entry: %d\n", err);
    // }

    struct mqtt_connect_client_info_t ci = {
        .client_id = "pico_eth",
        .keep_alive = 60,
        .client_user = NULL,
        .client_pass = NULL};

    mqtt_client = mqtt_client_new();
    if (!mqtt_client)
    {
        printf("Failed to create client\n");
        return;
    }

    err = mqtt_client_connect(mqtt_client, &broker_addr, MQTT_PORT,
                              mqtt_connection_cb, NULL, &ci);
    if (err != ERR_OK)
    {
        printf("Connect error: %d\n", err);
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("MQTT connected!\n");
        const char *pub_payload = "HELLO, it's PICO with FreeRTOS";
        err_t err;
        u8_t qos = 0;    /* 0 1 or 2, see MQTT specification */
        u8_t retain = 0; /* No don't retain such crappy payload... */
        err = mqtt_publish(client, "pico", pub_payload, strlen(pub_payload), qos, retain, mqtt_pub_request_cb, arg);
        if (err != ERR_OK)
        {
            printf("Publish err: %d\n", err);
        }
        mqtt_connected = true;
    } else {
        printf("MQTT connection lost: %d\n", status);
        mqtt_connected = false;
    }
}

// Callback после публикации
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if (result == ERR_OK)
    {
        printf("Message published successfully\n");
    }
    else
    {
        printf("Publish failed: %d\n", result);
    }
}