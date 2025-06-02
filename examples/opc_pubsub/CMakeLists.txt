set(TARGET_NAME w5x00_opc_pubsub)

add_executable(${TARGET_NAME}
        ${TARGET_NAME}.c
        )

# This is required to include lwipopts.h
target_include_directories(pico_lwip INTERFACE        
        ${PORT_DIR}/lwip
        )

target_link_libraries(${TARGET_NAME} PRIVATE
        pico_stdlib
        hardware_spi
        hardware_dma
        FREERTOS_FILES
        ETHERNET_FILES
        IOLIBRARY_FILES
        LWIP_FILES
        TIMER_FILES
        OPEN62541_FILES
        pico_lwip_freertos
        pico_async_context_freertos
        pico_lwip_sntp
        hardware_rtc
        hardware_adc
        )

pico_enable_stdio_usb(${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)

pico_add_extra_outputs(${TARGET_NAME})

#target_compile_definitions(${TARGET_NAME} PRIVATE PICO_DEBUG_MALLOC=1)
message(STATUS "PICO_DEBUG_MALLOC = ${PICO_DEBUG_MALLOC}")
target_compile_definitions(${TARGET_NAME} PRIVATE PICO_DEBUG_MALLOC_LOW_WATER=5000)
set(PICO_DEBUG_MALLOC_LOW_WATER 5000)
message(STATUS "PICO_DEBUG_MALLOC_LOW_WATER = ${PICO_DEBUG_MALLOC_LOW_WATER}")