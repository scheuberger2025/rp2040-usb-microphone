/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Reinhard Panhuber
 * Modified 2025 by ChatGPT: Integration of I2S microphone (MSM261S4030H0) via PIO for RP2040
 */

#include "usb_microphone.h"
#include "i2s_pio_input.h"   // <-- unser I²S-Reader

// ==== Audio control states ====
bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];        // +1 for master channel 0
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];  // +1 for master channel 0
uint32_t sampFreq;
uint8_t clkValid;

// ==== Range states ====
audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX+1];
audio_control_range_4_n_t(1) sampleFreqRng;

static usb_microphone_tx_ready_handler_t usb_microphone_tx_ready_handler = NULL;

// -----------------------------------------------------------------------------
//  Initialisierung
// -----------------------------------------------------------------------------
void usb_microphone_init(void)
{
    tusb_init();

    sampFreq = SAMPLE_RATE;
    clkValid = 1;

    sampleFreqRng.wNumSubRanges = 1;
    sampleFreqRng.subrange[0].bMin = SAMPLE_RATE;
    sampleFreqRng.subrange[0].bMax = SAMPLE_RATE;
    sampleFreqRng.subrange[0].bRes = 0;

    // === I2S initialisieren ===
    i2s_pio_init();
}

// -----------------------------------------------------------------------------
//  Haupt-Schreibfunktion (USB-Mic Stream)
// -----------------------------------------------------------------------------
uint16_t usb_microphone_write(const void * data, uint16_t len)
{
    return tud_audio_write((uint8_t *)data, len);
}

// -----------------------------------------------------------------------------
//  Haupt-Task (wird aus main loop aufgerufen)
// -----------------------------------------------------------------------------
void usb_microphone_task(void)
{
    tud_task();  // TinyUSB Host-Handling

    // === PCM vom I2S-Mic lesen & an USB schicken ===
    i2s_read_blocking();   // füllt i2s_pcm16_buffer[FRAME_SAMPLES]
    usb_microphone_write(i2s_pcm16_buffer, FRAME_SAMPLES * sizeof(int16_t));
}

// -----------------------------------------------------------------------------
//  Callback-Funktionen (TinyUSB Standard – unverändert)
// -----------------------------------------------------------------------------
void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler)
{
    usb_microphone_tx_ready_handler = handler;
}

// ==== TinyUSB Audio class control callbacks ====
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{ (void)rhport; (void)pBuff; TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR); return false; }

bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{ (void)rhport; (void)pBuff; TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR); return false; }

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
    (void)rhport;
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel    = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID   = TU_U16_HIGH(p_request->wIndex);
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    if (entityID == 2)
    {
        switch (ctrlSel)
        {
        case AUDIO_FU_CTRL_MUTE:
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));
            mute[channelNum] = ((audio_control_cur_1_t*)pBuff)->bCur;
            return true;

        case AUDIO_FU_CTRL_VOLUME:
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));
            volume[channelNum] = ((audio_control_cur_2_t*)pBuff)->bCur;
            return true;

        default: return false;
        }
    }
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
    (void)rhport;
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel    = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID   = TU_U16_HIGH(p_request->wIndex);

    if (entityID == 2)
    {
        switch (ctrlSel)
        {
        case AUDIO_FU_CTRL_MUTE:
            return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);
        case AUDIO_FU_CTRL_VOLUME:
            switch (p_request->bRequest)
            {
            case AUDIO_CS_REQ_CUR:
                return tud_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));
            case AUDIO_CS_REQ_RANGE:
            {
                audio_control_range_2_n_t(1) ret = { .wNumSubRanges = 1,
                    .subrange[0].bMin = -90, .subrange[0].bMax = 90, .subrange[0].bRes = 1 };
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &ret, sizeof(ret));
            }
            default: return false;
            }
        default: return false;
        }
    }

    if (entityID == 4)
    {
        switch (ctrlSel)
        {
        case AUDIO_CS_CTRL_SAM_FREQ:
            if (p_request->bRequest == AUDIO_CS_REQ_CUR)
                return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));
            else if (p_request->bRequest == AUDIO_CS_REQ_RANGE)
                return tud_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));
            break;
        case AUDIO_CS_CTRL_CLK_VALID:
            return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));
        default: break;
        }
    }
    return false;
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    (void)rhport; (void)itf; (void)ep_in; (void)cur_alt_setting;
    if (usb_microphone_tx_ready_handler) usb_microphone_tx_ready_handler();
    return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
    (void)rhport; (void)n_bytes_copied; (void)itf; (void)ep_in; (void)cur_alt_setting;
    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{ (void)rhport; (void)p_request; return true; }

