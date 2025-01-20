cmake_minimum_required(VERSION 3.15)

add_library(freertos_config INTERFACE)

target_sources(freertos_config PUBLIC   
        ${CMAKE_CURRENT_LIST_DIR}/idleMemory.c
        #${CMAKE_CURRENT_LIST_DIR}/cppMemory.cpp
    )
target_include_directories(freertos_config SYSTEM INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
	) # The config file directory\n"

target_compile_definitions(freertos_config INTERFACE
	projCOVERAGE_TEST=0
	)

target_link_libraries(freertos_config INTERFACE
	FreeRTOS-Kernel
	pico_stdlib
	)

target_compile_definitions(freertos_config INTERFACE ${definitions})
target_compile_options(freertos_config INTERFACE ${options})