# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(TrackbotPico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(TrackbotPico
    src/main.c
    src/registers.c
    src/encoder.c
    src/sampler.c
    src/i2c_target.c
    src/commands.c
    )

# TODO (encoder.c): add quadrature_encoder.pio from pico-examples, then
# pico_generate_pio_header(TrackbotPico ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)

pico_set_program_name(TrackbotPico "TrackbotPico")
pico_set_program_version(TrackbotPico "0.1")

# stdio over USB serial (disable UART so pins 0/1 are free for other uses)
pico_enable_stdio_uart(TrackbotPico 0)
pico_enable_stdio_usb(TrackbotPico 1)

target_include_directories(TrackbotPico PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    )

target_link_libraries(TrackbotPico
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_pio
    hardware_i2c
    hardware_watchdog
    pico_i2c_slave
    pico_bootrom
    pico_cyw43_arch_none
    )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(TrackbotPico)
