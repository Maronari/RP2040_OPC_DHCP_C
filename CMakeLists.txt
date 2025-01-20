# CMake minimum required version
cmake_minimum_required(VERSION 3.12)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_COMPILER "/usr/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/arm-none-eabi-g++")
set(PROJECT_NAME RP2350-HAT-LWIP-C)

# Pull in SDK (must be before project)
if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/libraries/pico-sdk)
endif()

set(PICO_DEOPTIMIZED_DEBUG ON)

# Set board name
set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350)

# Set project name
if (PICO_BOARD EQUAL "pico")
    set(PROJECT_NAME RP2040-HAT-LWIP-C)
elseif(PICO_BOARD EQUAL "pico2")
    set(PROJECT_NAME RP2350-HAT-LWIP-C)
endif()

if(NOT DEFINED FREERTOS_DIR)
    set(FREERTOS_DIR ${CMAKE_SOURCE_DIR}/libraries/FreeRTOS-Kernel)
    message(STATUS "FREERTOS_DIR = ${FREERTOS_DIR}")
endif()

if(NOT DEFINED WIZNET_DIR)
    set(WIZNET_DIR ${CMAKE_SOURCE_DIR}/libraries/ioLibrary_Driver)
    message(STATUS "WIZNET_DIR = ${WIZNET_DIR}")
endif()

if(NOT DEFINED MBEDTLS_DIR)
    set(MBEDTLS_DIR ${CMAKE_SOURCE_DIR}/libraries/mbedtls)
    message(STATUS "MBEDTLS_DIR = ${MBEDTLS_DIR}")
endif()

if(NOT DEFINED PORT_DIR)
    set(PORT_DIR ${CMAKE_SOURCE_DIR}/port)
    message(STATUS "PORT_DIR = ${PORT_DIR}")
endif()

if(NOT DEFINED OPEN62541_DIR)
    set(OPEN62541_DIR ${PORT_DIR}/open62541)
    message(STATUS "OPEN62541_DIR = ${OPEN62541_DIR}")
endif()

add_definitions(-DUA_ARCHITECTURE_FREERTOSLWIP)
#add_definitions(-DOPEN62541_FEERTOS_USE_OWN_MEM)
if (DEFINED UA_ARCHITECTURE_FREERTOSLWIP)
    message(STATUS "open62541 architecture is UA_ARCHITECTURE_FREERTOSLWIP")
endif()

include(rp2040_hat_lwip_c-patch.cmake)
include(rp2040_hat_freertos_c-patch.cmake)
include(rp2040_hat_lwip_c_sdk_version.cmake)
include(rp2040_hat_freertos_c_sdk_version.cmake)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
set(PICOTOOL_FETCH_FROM_GIT_PATH ${CMAKE_SOURCE_DIR}/libraries/picotool)

# Set project informations
project(${PROJECT_NAME} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
# Select the native compile PORT
set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
# Select the cross-compile PORT
if (CMAKE_CROSSCOMPILING)
    set(FREERTOS_PORT "GCC_ARM_CM3" CACHE STRING "" FORCE)
endif()

SET(FREERTOS_KERNEL_PATH "${FREERTOS_DIR}" CACHE STRING "Course Common Lib")
SET(FREERTOS_CONFIG_FILE_DIRECTORY "${PORT_DIR}/FreeRTOS-Kernel" CACHE STRING "Local Config")
include("${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake")
#include(FreeRTOS_Kernel_import.cmake)
include("${FREERTOS_CONFIG_FILE_DIRECTORY}/freertos_config.cmake")

FetchContent_Declare(freertos_kernel)

# Set ethernet chip
set(WIZNET_CHIP W5500)

if(${WIZNET_CHIP} STREQUAL W5100S)
    add_definitions(-D_WIZCHIP_=W5100S)
elseif(${WIZNET_CHIP} STREQUAL W5500)
    add_definitions(-D_WIZCHIP_=W5500)
else()
    message(FATAL_ERROR "WIZNET_CHIP is wrong = ${WIZNET_CHIP}")
endif()

# Turn off mbedtls test mode 
set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbedtls programs")
set(ENABLE_TESTING OFF CACHE BOOL "Build mbedtls testing")
add_definitions(-DMBEDTLS_CONFIG_FILE="${PORT_DIR}/mbedtls/inc/ssl_config.h")
add_definitions(-DSET_TRUSTED_CERT_IN_SAMPLES)

# Hardware-specific examples in subdirectories:
add_subdirectory(examples)

# Add libraries in subdirectories
add_subdirectory(${CMAKE_SOURCE_DIR}/libraries)
#add_subdirectory(${FREERTOS_DIR})
#add_subdirectory(${MBEDTLS_DIR})
add_subdirectory(${PORT_DIR})

# Set compile options
add_compile_options(
        -Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -Wno-dev
)