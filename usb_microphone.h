#ifndef _USB_MICROPHONE_H_
#define _USB_MICROPHONE_H_

#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 16000
#endif

#ifndef FRAME_SAMPLES
#define FRAME_SAMPLES 256
#endif

#ifndef BYTES_PER_SAMPLE
#define BYTES_PER_SAMPLE 2
#endif

#ifndef SAMPLE_BUFFER_SIZE
#define SAMPLE_BUFFER_SIZE FRAME_SAMPLES
#endif

typedef void (*usb_microphone_tx_ready_handler_t)(void);

void usb_microphone_init(void);
void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler);
void usb_microphone_task(void);
uint16_t usb_microphone_write(const void * data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif