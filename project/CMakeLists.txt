cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ext/stm32-cmake/cmake/stm32_gcc.cmake)

project(stm32-template C ASM)

####################################
# Project config
####################################

set(OUTPUT_PATH "/output")
set(CMAKE_C_FLAGS "-O3")

set(HAL_COMP_LIST RCC GPIO CORTEX DMA UART TIM PWR STM32G0)
set(CMSIS_COMP_LIST "")

add_definitions(
	-DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1
)

####################################
# Find packages
####################################

find_package(CMSIS COMPONENTS "${CMSIS_COMP_LIST}" REQUIRED)
find_package(HAL   COMPONENTS "${HAL_COMP_LIST}"   REQUIRED)

####################################
# Generate executable
####################################

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/ext/printf/src
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(PROJECT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/ext/printf/src/printf/printf.c

	${CMAKE_CURRENT_SOURCE_DIR}/src/bsp/pin.c

	${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/cmds.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/lex.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/msg.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/parse.c

	${CMAKE_CURRENT_SOURCE_DIR}/src/func/cmds.c

	${CMAKE_CURRENT_SOURCE_DIR}/src/io/clock.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/io/oneshot_timer.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/io/gpio.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/io/uart.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/io/dmx.c

	${CMAKE_CURRENT_SOURCE_DIR}/src/stm32g0xx_hal_msp.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/stm32g0xx_it.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

####################################
# RE2C stuff
####################################

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/lex.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/lex.re.c
	COMMAND re2c -T -o ${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/lex.c ${CMAKE_CURRENT_SOURCE_DIR}/src/prpc/lex.re.c
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/func/cmds.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/func/cmds.re.c
	COMMAND re2c -T -o ${CMAKE_CURRENT_SOURCE_DIR}/src/func/cmds.c ${CMAKE_CURRENT_SOURCE_DIR}/src/func/cmds.re.c
)

####################################
# Executable targets
####################################

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/stm32g0xx_hal_conf.h)
add_custom_command(
	OUTPUT   ${PROJECT_NAME}.bin
	DEPENDS  ${PROJECT_NAME}.elf
	COMMAND  "${CMAKE_OBJCOPY}" -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
)

add_custom_target(bin ALL
	DEPENDS ${PROJECT_NAME}.bin
	COMMAND  "${CMAKE_OBJCOPY}" -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
)

set(CMSIS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/sys/system_stm32g0xx.c
	${CMAKE_CURRENT_SOURCE_DIR}/sys/startup_stm32g031xx.s
)

target_link_libraries( stm32-template
	HAL::STM32::G0::CORTEX
	HAL::STM32::G0::RCC
	HAL::STM32::G0::PWR
	HAL::STM32::G0::PWREx
	HAL::STM32::G0::DMA
	HAL::STM32::G0::RCCEx
	HAL::STM32::G0::GPIO
	HAL::STM32::G0::UART
	HAL::STM32::G0::UARTEx
	HAL::STM32::G0::TIM
	HAL::STM32::G0::TIMEx
	CMSIS::STM32::G031xx
	STM32::NoSys
)

target_link_options(STM32::NoSys INTERFACE -Wl,--print-memory-usage)

stm32_add_linker_script(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sys/STM32G031K8Tx_FLASH.ld)

stm32_print_size_of_target(stm32-template)

####################################
# Install rules
####################################

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf
	DESTINATION
		${OUTPUT_PATH}
)
