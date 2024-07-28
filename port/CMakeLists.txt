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
        FREERTOS_FILES
        )

#FreeRTOS
target_sources(FREERTOS_FILES PUBLIC
        ${FREERTOS_DIR}/portable/ThirdParty/GCC/RP2040/idle_task_static_memory.c
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
        FREERTOS_FILES
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
        )

target_include_directories(OPEN62541_FILES PUBLIC
        ${PORT_DIR}/open62541
        ${WIZNET_DIR}/Ethernet
        ${PORT_DIR}
        ${PORT_DIR}/lwip
        )

target_link_libraries(OPEN62541_FILES PRIVATE
        pico_lwip
        pico_lwip_nosys
        FREERTOS_FILES
        pico_stdlib
        )

