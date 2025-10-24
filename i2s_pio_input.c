/*
 * I2S PIO Input Implementation
 */

#include "i2s_pio_input.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "i2s_rx.pio.h"

// Pin-Konfiguration
#define I2S_BCLK_PIN   2
#define I2S_LRCLK_PIN  3
#define I2S_DATA_PIN   4
#define SAMPLE_RATE    16000

// Globale Variablen
int16_t i2s_pcm16_buffer[FRAME_SAMPLES];

static PIO pio = pio0;
static uint sm = 0;
static int dma_chan;
static uint32_t i2s_raw32[FRAME_SAMPLES];

void i2s_pio_init(void) {
    uint offset = pio_add_program(pio, &i2s_rx_program);
    pio_sm_config c = i2s_rx_program_get_default_config(offset);
    
    sm_config_set_in_pins(&c, I2S_DATA_PIN);
    sm_config_set_jmp_pin(&c, I2S_LRCLK_PIN);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE);
    sm_config_set_clkdiv(&c, 1.0f);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCLK_PIN, 3, false);
    
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dc = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, false));
    
    dma_channel_configure(dma_chan, &dc, i2s_raw32,
                          &pio->rxf[sm], FRAME_SAMPLES, false);
    
    pio_sm_set_enabled(pio, sm, true);
}

void i2s_read_blocking(void) {
    dma_channel_set_write_addr(dma_chan, i2s_raw32, true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    
    for (uint i = 0; i < FRAME_SAMPLES; i++) {
        int32_t sample32 = (int32_t)i2s_raw32[i];
        i2s_pcm16_buffer[i] = (int16_t)(sample32 >> 16);
    }
}