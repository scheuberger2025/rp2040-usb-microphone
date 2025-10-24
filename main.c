#include "pico/stdlib.h"
#include "usb_microphone.h"
#include <stdio.h>

int main(void) {
    // System-Clock auf 120 MHz (stabiler für USB)
    set_sys_clock_khz(120000, true);
    
    // Standard I/O
    stdio_init_all();
    
    // WICHTIG: Kurze Pause für USB-Enumeration
    sleep_ms(100);
    
    printf("\n====================================\n");
    printf("🎤 RP2040 USB-Mikrofon\n");
    printf("====================================\n");
    
    // USB-Mikrofon initialisieren
    usb_microphone_init();
    
    printf("✅ System bereit!\n\n");
    
    // Haupt-Loop
    while (1) {
        usb_microphone_task();
    }
    
    return 0;
}
