cmake_minimum_required(VERSION 3.12)

# ==== USB Microphone Project (I2S Version) ====
add_executable(usb_microphone
    main.c
    usb_descriptors.c
    usb_microphone.c
    i2s_pio_input.c                # <-- hinzugefügt: unsere neue Datei
)

# Include-Verzeichnis
target_include_directories(usb_microphone PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# ==== PIO Header aus i2s_rx.pio generieren ====
pico_generate_pio_header(usb_microphone ${CMAKE_CURRENT_LIST_DIR}/i2s_rx.pio)

# ==== Linker-Bibliotheken ====
target_link_libraries(usb_microphone
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_clocks
    tinyusb_device
    tinyusb_board
)

# ==== Ausgabedateien (map/bin/hex/uf2) ====
pico_add_extra_outputs(usb_microphone)
