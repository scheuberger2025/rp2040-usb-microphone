cmake_minimum_required(VERSION 3.13)

# Pico SDK importieren BEVOR project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Projekt definieren
project(usb_microphone C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pico SDK initialisieren
pico_sdk_init()

# Executable erstellen
add_executable(usb_microphone
    main.c
    usb_descriptors.c
    usb_microphone.c
    i2s_pio_input.c
)

# Include-Verzeichnisse
target_include_directories(usb_microphone PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# PIO Header generieren
pico_generate_pio_header(usb_microphone ${CMAKE_CURRENT_LIST_DIR}/i2s_rx.pio)

# Bibliotheken linken
target_link_libraries(usb_microphone
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_clocks
    tinyusb_device
    tinyusb_board
)

# Output-Dateien erstellen
pico_add_extra_outputs(usb_microphone)

# USB/UART Debug
pico_enable_stdio_usb(usb_microphone 0)
pico_enable_stdio_uart(usb_microphone 1)
