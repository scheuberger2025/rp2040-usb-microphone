#include "usb_microphone.h"
#include "i2s_pio_input.h"
#include <stdio.h>

bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
uint32_t sampFreq;
uint8_t clkValid;

audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX+1];
audio_control_range_4_n_t(1) sampleFreqRng;

static usb_microphone_tx_ready_handler_t usb_microphone_tx_ready_handler = NULL;

void usb_microphone_init(void) {
    printf("🔧 Initialisiere USB...\n");
    
    // ZUERST TinyUSB initialisieren (wichtig!)
    tusb_init();
    
    sampFreq = SAMPLE_RATE;
    clkValid = 1;
    
    sampleFreqRng.wNumSubRanges = 1;
    sampleFreqRng.subrange[0].bMin = SAMPLE_RATE;
    sampleFreqRng.subrange[0].bMax = SAMPLE_RATE;
    sampleFreqRng.subrange[0].bRes = 0;
    
    printf("✅ USB bereit\n");
    
    // DANN I2S initialisieren
    printf("🔧 Initialisiere I2S...\n");
    i2s_pio_init();
    printf("✅ I2S bereit\n");
}

uint16_t usb_microphone_write(const void * data, uint16_t len) {
    return tud_audio_write((uint8_t *)data, len);
}

void usb_microphone_task(void) {
    // USB Task (MUSS oft aufgerufen werden!)
    tud_task();
    
    // Nur senden wenn USB bereit
    if (tud_audio_write_support_ff(0x01)) {
        i2s_read_blocking();
        usb_microphone_write(i2s_pcm16_buffer, FRAME_SAMPLES * sizeof(int16_t));
    }
    
    // Kleine Pause
    sleep_us(10);
}

void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler) {
    usb_microphone_tx_ready_handler = handler;
}

// TinyUSB Callbacks (vereinfacht)
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff) {
    (void)rhport; (void)pBuff;
    return false;
}

bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff) {
    (void)rhport; (void)pBuff;
    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff) {
    (void)rhport;
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
    
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);
    
    if (entityID == 2) {
        if (ctrlSel == AUDIO_FU_CTRL_MUTE) {
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));
            mute[channelNum] = ((audio_control_cur_1_t*)pBuff)->bCur;
            return true;
        }
        if (ctrlSel == AUDIO_FU_CTRL_VOLUME) {
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));
            volume[channelNum] = ((audio_control_cur_2_t*)pBuff)->bCur;
            return true;
        }
    }
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request) {
    (void)rhport;
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
    
    if (entityID == 2) {
        if (ctrlSel == AUDIO_FU_CTRL_MUTE) {
            return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);
        }
        if (ctrlSel == AUDIO_FU_CTRL_VOLUME) {
            if (p_request->bRequest == AUDIO_CS_REQ_CUR) {
                return tud_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));
            }
            if (p_request->bRequest == AUDIO_CS_REQ_RANGE) {
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &volumeRng[channelNum], sizeof(volumeRng[channelNum]));
            }
        }
    }
    
    if (entityID == 4) {
        if (ctrlSel == AUDIO_CS_CTRL_SAM_FREQ) {
            if (p_request->bRequest == AUDIO_CS_REQ_CUR)
                return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));
            if (p_request->bRequest == AUDIO_CS_REQ_RANGE)
                return tud_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));
        }
        if (ctrlSel == AUDIO_CS_CTRL_CLK_VALID) {
            return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));
        }
    }
    return false;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting) {
    (void)rhport; (void)itf; (void)ep_in; (void)cur_alt_setting;
    if (usb_microphone_tx_ready_handler) usb_microphone_tx_ready_handler();
    return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting) {
    (void)rhport; (void)n_bytes_copied; (void)itf; (void)ep_in; (void)cur_alt_setting;
    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request) {
    (void)rhport; (void)p_request;
    return true;
}
