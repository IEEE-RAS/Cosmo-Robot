
cmake_minimum_required(VERSION 3.13)

if(NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()

if(NOT PICO_SDK_PATH)
    message(FATAL_ERROR
        "Defina PICO_SDK_PATH para o Pico SDK instalado"
    )
endif()

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(cosmo_rp2040 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(cosmo
    main.c
    src/eyes.c
    src/ssd1306.c
    src/botao.c
    src/mic_i2s.c
    src/sensores/hc-sr04.c
)

target_include_directories(cosmo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

pico_generate_pio_header(
    cosmo
    ${CMAKE_CURRENT_LIST_DIR}/src/mic_i2s.pio
)

target_link_libraries(cosmo
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_pio
    hardware_dma
    hardware_irq
    hardware_clocks
)

pico_enable_stdio_usb(cosmo 1)
pico_enable_stdio_uart(cosmo 0)

pico_add_extra_outputs(cosmo)
