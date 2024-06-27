cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(single-slope-adc C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

#include(example_auto_set_url.cmake)

add_executable(${PROJECT_NAME}
        main.c
        utility.c
        )

target_link_libraries(${PROJECT_NAME} pico_stdlib pico_multicore hardware_pwm hardware_pio)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/measurement.pio)

pico_add_extra_outputs(${PROJECT_NAME})
