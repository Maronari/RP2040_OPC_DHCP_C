# ioLibrary_Driver
add_library(IOLIBRARY_FILES STATIC)

target_sources(IOLIBRARY_FILES PUBLIC
        ${PORT_DIR}/ioLibrary_Driver/src/w5x00_spi.c
        ${PORT_DIR}/ioLibrary_Driver/src/w5x00_gpio_irq.c
        )

target_include_directories(IOLIBRARY_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${PORT_DIR}/ioLibrary_Driver/inc
        ${PORT_DIR}
        )

target_link_libraries(IOLIBRARY_FILES PRIVATE
        pico_stdlib
        hardware_spi
        hardware_dma
        hardware_clocks
        freertos_config
        FreeRTOS-Kernel-Heap4
        )

#FreeRTOS
# target_sources(FREERTOS_FILES PUBLIC
#         ${FREERTOS_DIR}/portable/ThirdParty/GCC/RP2040/idle_task_static_memory.c
#         )

add_library(FREERTOS_CALLOC STATIC)

target_sources(FREERTOS_CALLOC PUBLIC
        ${PORT_DIR}/FreeRTOS-Kernel/inc/myMalloc/myMalloc.c
        )

target_include_directories(FREERTOS_CALLOC PUBLIC
        ${PORT_DIR}/FreeRTOS-Kernel/inc/myMalloc
        )

target_link_libraries(FREERTOS_CALLOC PUBLIC
        freertos_config
        FreeRTOS-Kernel-Heap4
        pico_stdlib
        )

# lwip
add_library(LWIP_FILES STATIC)

target_sources(LWIP_FILES PUBLIC
        ${PORT_DIR}/lwip/w5x00_lwip.c
        ${PICO_LWIP_PATH}/contrib/ports/freertos/sys_arch.c
        )

target_include_directories(LWIP_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${PORT_DIR}
        ${PORT_DIR}/lwip
        ${PICO_LWIP_PATH}/contrib/ports/freertos/include
        )

target_link_libraries(LWIP_FILES PUBLIC
        freertos_config
        FreeRTOS-Kernel-Heap4
        ETHERNET_FILES
        pico_lwip
        pico_lwip_nosys
        pico_lwip_freertos
        )

# timer
add_library(TIMER_FILES STATIC)

target_sources(TIMER_FILES PUBLIC
        ${PORT_DIR}/timer/timer.c
        )

target_include_directories(TIMER_FILES PUBLIC
        ${PORT_DIR}/timer
        )

target_link_libraries(TIMER_FILES PRIVATE
        pico_stdlib      
        )

#open62541
add_library(OPEN62541_FILES STATIC)

target_sources(OPEN62541_FILES PUBLIC
        ${PORT_DIR}/open62541/open62541.c
        #${PORT_DIR}/open62541/opc_add_temperature.c
        )

target_include_directories(OPEN62541_FILES PUBLIC
        ${PORT_DIR}/open62541
        ${WIZNET_DIR}/Ethernet
        ${PORT_DIR}
        ${PORT_DIR}/lwip
        ${PORT_DIR}/open62541/include
        pico_lwip_contrib_freertos_headers
        )

target_link_libraries(OPEN62541_FILES PRIVATE
        pico_lwip
        pico_lwip_nosys
        freertos_config
        FreeRTOS-Kernel-Heap4
        FREERTOS_CALLOC
        pico_stdlib
        pico_lwip_contrib_freertos
        )