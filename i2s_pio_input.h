/*
 * I2S PIO Input Header
 */

#ifndef I2S_PIO_INPUT_H
#define I2S_PIO_INPUT_H

#include <stdint.h>

// Audio-Parameter
#define FRAME_SAMPLES 256

// Globaler PCM-Buffer
extern int16_t i2s_pcm16_buffer[FRAME_SAMPLES];

// Funktionen
void i2s_pio_init(void);
void i2s_read_blocking(void);

#endif