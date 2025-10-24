/*
 * RP2040 USB-Mikrofon - Hauptprogramm
 * MSM261S4030H0 I2S Mikrofon über PIO
 */

#include "pico/stdlib.h"
#include "usb_microphone.h"
#include <stdio.h>

int main(void) {
    // Standard I/O initialisieren
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n====================================\n");
    printf("🎤 RP2040 USB-Mikrofon gestartet\n");
    printf("====================================\n");
    printf("MSM261S4030H0 via I2S/PIO\n");
    printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
    printf("Buffer: %d samples\n\n", FRAME_SAMPLES);
    
    // USB-Mikrofon initialisieren
    usb_microphone_init();
    
    printf("✅ System bereit!\n\n");
    
    // Haupt-Loop
    while (1) {
        usb_microphone_task();
    }
    
    return 0;
}