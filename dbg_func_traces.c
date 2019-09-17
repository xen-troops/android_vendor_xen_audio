/*
 * Copyright (C) 2013-2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright (C) 2018-2019 EPAM Systems Inc.
 */

#define LOG_TAG "xa_debug"

#include <log/log.h>
#include <hardware/audio.h>


typedef struct xa_debug_message_map
{
    int key;
    char * message;
} xa_debug_message_map_t;

#define XA_DBG_EMPTY ""

const char * xa_dbgstr(int value, xa_debug_message_map_t * map)
{
    int i = 0;
    for (i = 0; i < INT_MAX; i++) {
        if (map[i].message == NULL) {
            return XA_DBG_EMPTY;
        }
        if (map[i].key == value) {
            return map[i].message;
        }
    }
    return XA_DBG_EMPTY;
}

/* audio_port_role_t */
xa_debug_message_map_t xa_dbg_port_role[] = {
    { AUDIO_PORT_ROLE_NONE, "NONE" },
    { AUDIO_PORT_ROLE_SOURCE, "SOURCE" },
    { AUDIO_PORT_ROLE_SINK, "SINK" },
    { 0, NULL }
};
const char * xa_dbgstr_port_role(audio_port_role_t value)
{
    return xa_dbgstr(value, &xa_dbg_port_role[0]);
}

/* audio_port_type_t */
xa_debug_message_map_t xa_dbg_port_type[] = {
    { AUDIO_PORT_TYPE_NONE, "NONE" },
    { AUDIO_PORT_TYPE_DEVICE, "DEVICE" },
    { AUDIO_PORT_TYPE_MIX, "MIX" },
    { AUDIO_PORT_TYPE_SESSION, "SESSION" },
    { 0, NULL }
};
const char * xa_dbgstr_port_type(audio_port_type_t value)
{
    return xa_dbgstr(value, &xa_dbg_port_type[0]);
}

/* port config_mask */
#define XA_DBG_ITEMS_IN_PORT_CONFIG 4  /* number of items in AUDIO_PORT_CONFIG_XXX */
static char xa_dbgbuf_config_mask[XA_DBG_ITEMS_IN_PORT_CONFIG + 1];  /* +1 for terminator */
/* Function returns easy to read presentation of port configuration.
 * Resulting string contains some char if bit is set, and dash ('-') otherwise.
 * Bits are ordered from MSB to LSB.
 * Examples: "GFCS", "G---", "--C-" etc.
 * String is returned in static internal buffer, so it will be overwritten
 * on next call to this function. */
const char * xa_dbgstr_port_config_mask(unsigned int value)
{
    memset(xa_dbgbuf_config_mask, '-', sizeof(xa_dbgbuf_config_mask)-1);
    if ((value & AUDIO_PORT_CONFIG_GAIN) != 0) {
        xa_dbgbuf_config_mask[0] = 'G';
    }
    if ((value & AUDIO_PORT_CONFIG_FORMAT) != 0) {
        xa_dbgbuf_config_mask[1] = 'F';
    }
    if ((value & AUDIO_PORT_CONFIG_CHANNEL_MASK) != 0) {
        xa_dbgbuf_config_mask[2] = 'C';
    }
    if ((value & AUDIO_PORT_CONFIG_SAMPLE_RATE) != 0) {
        xa_dbgbuf_config_mask[3] = 'S';
    }
    xa_dbgbuf_config_mask[XA_DBG_ITEMS_IN_PORT_CONFIG] = 0;

    return xa_dbgbuf_config_mask;
}

/* audio_input_flags_t */
#define XA_DBG_ITEMS_IN_INPUT_FLAGS 7  /* number of non-zero items in audio_input_flags_t */
static char xa_dbgbuf_input_flags[XA_DBG_ITEMS_IN_INPUT_FLAGS + 1];  /* +1 for terminator */
/* Function returns easy to read presentation of input flags.
 * Resulting string contains some char if bit is set, and dash ('-') otherwise.
 * Flags are ordered from MSB to LSB.
 * Examples: "AVMSRHF", "------F", "-------" etc.
 * String is returned in static internal buffer, so it will be overwritten
 * on next call to this function. */
const char * xa_dbgstr_input_flags(unsigned int value)
{
    memset(xa_dbgbuf_input_flags, '-', sizeof(xa_dbgbuf_input_flags)-1);
    if ((value & AUDIO_INPUT_FLAG_HW_AV_SYNC) != 0) {
        xa_dbgbuf_input_flags[0] = 'A';
    }
    if ((value & AUDIO_INPUT_FLAG_VOIP_TX) != 0) {
        xa_dbgbuf_input_flags[1] = 'V';
    }
    if ((value & AUDIO_INPUT_FLAG_MMAP_NOIRQ) != 0) {
        xa_dbgbuf_input_flags[2] = 'M';
    }
    if ((value & AUDIO_INPUT_FLAG_SYNC) != 0) {
        xa_dbgbuf_input_flags[3] = 'S';
    }
    if ((value & AUDIO_INPUT_FLAG_RAW) != 0) {
        xa_dbgbuf_input_flags[4] = 'R';
    }
    if ((value & AUDIO_INPUT_FLAG_HW_HOTWORD) != 0) {
        xa_dbgbuf_input_flags[5] = 'H';
    }
    if ((value & AUDIO_INPUT_FLAG_FAST) != 0) {
        xa_dbgbuf_input_flags[6] = 'F';
    }
    xa_dbgbuf_input_flags[XA_DBG_ITEMS_IN_INPUT_FLAGS] = 0;

    return xa_dbgbuf_input_flags;
}

/* audio_output_flags_t */
#define XA_DBG_ITEMS_IN_OUTPUT_FLAGS 15  /* number of non-zero items in audio_output_flags_t */
static char xa_dbgbuf_output_flags[XA_DBG_ITEMS_IN_OUTPUT_FLAGS + 1];  /* +1 for terminator */
/* Function returns easy to read presentation of output flags.
 * Resulting string contains some char if bit is set, and dash ('-') otherwise.
 * Flags are ordered from MSB to LSB.
 * Examples: "IVMd9SRTHNCDFPD", "-------------P-" etc.
 * String is returned in static internal buffer, so it will be overwritten
 * on next call to this function. */
const char * xa_dbgstr_output_flags(unsigned int value)
{
    memset(xa_dbgbuf_output_flags, '-', sizeof(xa_dbgbuf_output_flags)-1);
    if ((value & AUDIO_OUTPUT_FLAG_INCALL_MUSIC) != 0) {
        xa_dbgbuf_output_flags[0] = 'I';
    }
    if ((value & AUDIO_OUTPUT_FLAG_VOIP_RX) != 0) {
        xa_dbgbuf_output_flags[1] = 'V';
    }
    if ((value & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) != 0) {
        xa_dbgbuf_output_flags[2] = 'M';
    }
    if ((value & AUDIO_OUTPUT_FLAG_DIRECT_PCM) != 0) {
        xa_dbgbuf_output_flags[3] = 'd';
    }
    if ((value & AUDIO_OUTPUT_FLAG_IEC958_NONAUDIO) != 0) {
        xa_dbgbuf_output_flags[4] = '9';
    }
    if ((value & AUDIO_OUTPUT_FLAG_SYNC) != 0) {
        xa_dbgbuf_output_flags[5] = 'S';
    }
    if ((value & AUDIO_OUTPUT_FLAG_RAW) != 0) {
        xa_dbgbuf_output_flags[6] = 'R';
    }
    if ((value & AUDIO_OUTPUT_FLAG_TTS) != 0) {
        xa_dbgbuf_output_flags[7] = 'T';
    }
    if ((value & AUDIO_OUTPUT_FLAG_HW_AV_SYNC) != 0) {
        xa_dbgbuf_output_flags[8] = 'H';
    }
    if ((value & AUDIO_OUTPUT_FLAG_NON_BLOCKING) != 0) {
        xa_dbgbuf_output_flags[9] = 'N';
    }
    if ((value & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) {
        xa_dbgbuf_output_flags[10] = 'C';
    }
    if ((value & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) != 0) {
        xa_dbgbuf_output_flags[11] = 'D';
    }
    if ((value & AUDIO_OUTPUT_FLAG_FAST) != 0) {
        xa_dbgbuf_output_flags[12] = 'F';
    }
    if ((value & AUDIO_OUTPUT_FLAG_PRIMARY) != 0) {
        xa_dbgbuf_output_flags[13] = 'P';
    }
    if ((value & AUDIO_OUTPUT_FLAG_DIRECT) != 0) {
        xa_dbgbuf_output_flags[14] = 'D';
    }
    xa_dbgbuf_output_flags[XA_DBG_ITEMS_IN_OUTPUT_FLAGS] = 0;

    return xa_dbgbuf_output_flags;
}

/* audio_format_t */
xa_debug_message_map_t xa_dbg_format[] = {
    { AUDIO_FORMAT_INVALID, "INVALID" },
    { AUDIO_FORMAT_DEFAULT, "DEFAULT" },
    { AUDIO_FORMAT_PCM_16_BIT, "PCM_16_BIT" },
    { AUDIO_FORMAT_PCM_8_BIT, "PCM_8_BIT" },
    { AUDIO_FORMAT_PCM_32_BIT, "PCM_32_BIT" },
    { AUDIO_FORMAT_PCM_8_24_BIT, "PCM_8_24_BIT" },
    { AUDIO_FORMAT_PCM_FLOAT, "PCM_FLOAT" },
    { AUDIO_FORMAT_PCM_24_BIT_PACKED, "PCM_24_BIT_PACKED" },
    { AUDIO_FORMAT_MP3, "MP3" },
    { AUDIO_FORMAT_AMR_NB, "AMR_NB" },
    { AUDIO_FORMAT_AMR_WB, "AMR_WB" },
    { AUDIO_FORMAT_AAC_MAIN, "AAC_MAIN" },
    { AUDIO_FORMAT_AAC_LC, "AAC_LC" },
    { AUDIO_FORMAT_AAC_SSR, "AAC_SSR" },
    { AUDIO_FORMAT_AAC_LTP, "AAC_LTP" },
    { AUDIO_FORMAT_AAC_HE_V1, "AAC_HE_V1" },
    { AUDIO_FORMAT_AAC_SCALABLE, "AAC_SCALABLE" },
    { AUDIO_FORMAT_AAC_ERLC, "AAC_ERLC" },
    { AUDIO_FORMAT_AAC_LD, "AAC_LD" },
    { AUDIO_FORMAT_AAC_HE_V2, "AAC_HE_V2" },
    { AUDIO_FORMAT_AAC_ELD, "AAC_ELD" },
    { AUDIO_FORMAT_AAC_XHE, "AAC_XHE" },
    { AUDIO_FORMAT_VORBIS, "VORBIS" },
    { AUDIO_FORMAT_AAC_ADTS_MAIN, "AAC_ADTS_MAIN" },
    { AUDIO_FORMAT_AAC_ADTS_LC, "AAC_ADTS_LC" },
    { AUDIO_FORMAT_AAC_ADTS_SSR, "AAC_ADTS_SSR" },
    { AUDIO_FORMAT_AAC_ADTS_LTP, "AAC_ADTS_LTP" },
    { AUDIO_FORMAT_AAC_ADTS_HE_V1, "AAC_ADTS_HE_V1" },
    { AUDIO_FORMAT_AAC_ADTS_SCALABLE, "AAC_ADTS_SCALABLE" },
    { AUDIO_FORMAT_AAC_ADTS_ERLC, "AAC_ADTS_ERLC" },
    { AUDIO_FORMAT_AAC_ADTS_LD, "AAC_ADTS_LD" },
    { AUDIO_FORMAT_AAC_ADTS_HE_V2, "AAC_ADTS_HE_V2" },
    { AUDIO_FORMAT_AAC_ADTS_ELD, "AAC_ADTS_ELD" },
    { AUDIO_FORMAT_AAC_ADTS_XHE, "AAC_ADTS_XHE" },
    { AUDIO_FORMAT_E_AC3_JOC, "E_AC3_JOC" },
    { AUDIO_FORMAT_MAT_1_0, "MAT_1_0" },
    { AUDIO_FORMAT_MAT_2_0, "MAT_2_0" },
    { AUDIO_FORMAT_MAT_2_0, "MAT_2_1" },
    { 0, NULL }
};
const char * xa_dbgstr_format(audio_format_t value)
{
    return xa_dbgstr(value, &xa_dbg_format[0]);
}

/* audio_source_t */
xa_debug_message_map_t xa_dbg_source[] = {
    { AUDIO_SOURCE_DEFAULT, "DEFAULT" },
    { AUDIO_SOURCE_MIC, "MIC" },
    { AUDIO_SOURCE_VOICE_UPLINK, "VOICE_UPLINK" },
    { AUDIO_SOURCE_VOICE_DOWNLINK, "VOICE_DOWNLINK" },
    { AUDIO_SOURCE_VOICE_CALL, "VOICE_CALL" },
    { AUDIO_SOURCE_CAMCORDER, "CAMCORDER" },
    { AUDIO_SOURCE_VOICE_RECOGNITION, "VOICE_RECOGNITION" },
    { AUDIO_SOURCE_VOICE_COMMUNICATION, "VOICE_COMMUNICATION" },
    { AUDIO_SOURCE_REMOTE_SUBMIX, "REMOTE_SUBMIX" },
    { AUDIO_SOURCE_UNPROCESSED, "UNPROCESSED" },
    { AUDIO_SOURCE_FM_TUNER, "FM_TUNER" },
#ifndef AUDIO_NO_SYSTEM_DECLARATIONS
    { AUDIO_SOURCE_HOTWORD, "HOTWORD" },
#endif /* AUDIO_NO_SYSTEM_DECLARATIONS */
    { 0, NULL }
};
const char * xa_dbgstr_source(audio_source_t value)
{
    return xa_dbgstr(value, &xa_dbg_source[0]);
}

/* audio device */
xa_debug_message_map_t xa_dbg_device[] = {
    { AUDIO_DEVICE_NONE, "NONE" },
    { AUDIO_DEVICE_OUT_EARPIECE, "OUT_EARPIECE" },
    { AUDIO_DEVICE_OUT_SPEAKER, "OUT_SPEAKER" },
    { AUDIO_DEVICE_OUT_WIRED_HEADSET, "OUT_WIRED_HEADSET" },
    { AUDIO_DEVICE_OUT_WIRED_HEADPHONE, "OUT_WIRED_HEADPHONE" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_SCO, "OUT_BLUETOOTH_SCO" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET, "OUT_BLUETOOTH_SCO_HEADSET" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT, "OUT_BLUETOOTH_SCO_CARKIT" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_A2DP, "OUT_BLUETOOTH_A2DP" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES, "OUT_BLUETOOTH_A2DP_HEADPHONES" },
    { AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER, "OUT_BLUETOOTH_A2DP_SPEAKER" },
    { AUDIO_DEVICE_OUT_HDMI, "OUT_HDMI" },
    { AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET, "OUT_ANLG_DOCK_HEADSET" },
    { AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET, "OUT_DGTL_DOCK_HEADSET" },
    { AUDIO_DEVICE_OUT_USB_ACCESSORY, "OUT_USB_ACCESSORY" },
    { AUDIO_DEVICE_OUT_USB_DEVICE, "OUT_USB_DEVICE" },
    { AUDIO_DEVICE_OUT_REMOTE_SUBMIX, "OUT_REMOTE_SUBMIX" },
    { AUDIO_DEVICE_OUT_TELEPHONY_TX, "OUT_TELEPHONY_TX" },
    { AUDIO_DEVICE_OUT_LINE, "OUT_LINE" },
    { AUDIO_DEVICE_OUT_HDMI_ARC, "OUT_HDMI_ARC" },
    { AUDIO_DEVICE_OUT_SPDIF, "OUT_SPDIF" },
    { AUDIO_DEVICE_OUT_FM, "OUT_FM" },
    { AUDIO_DEVICE_OUT_AUX_LINE, "OUT_AUX_LINE" },
    { AUDIO_DEVICE_OUT_SPEAKER_SAFE, "OUT_SPEAKER_SAFE" },
    { AUDIO_DEVICE_OUT_IP, "OUT_IP" },
    { AUDIO_DEVICE_OUT_BUS, "OUT_BUS" },
    { AUDIO_DEVICE_OUT_PROXY, "OUT_PROXY" },
    { AUDIO_DEVICE_OUT_USB_HEADSET, "OUT_USB_HEADSET" },
    { AUDIO_DEVICE_OUT_HEARING_AID, "OUT_HEARING_AID" },
    { AUDIO_DEVICE_OUT_ECHO_CANCELLER, "OUT_ECHO_CANCELLER" },
    { AUDIO_DEVICE_OUT_DEFAULT, "OUT_DEFAULT" },
    { AUDIO_DEVICE_IN_COMMUNICATION, "IN_COMMUNICATION" },
    { AUDIO_DEVICE_IN_AMBIENT, "IN_AMBIENT" },
    { AUDIO_DEVICE_IN_BUILTIN_MIC, "IN_BUILTIN_MIC" },
    { AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET, "IN_BLUETOOTH_SCO_HEADSET" },
    { AUDIO_DEVICE_IN_WIRED_HEADSET, "IN_WIRED_HEADSET" },
    { AUDIO_DEVICE_IN_HDMI, "IN_HDMI" },
    { AUDIO_DEVICE_IN_TELEPHONY_RX, "IN_TELEPHONY_RX" },
    { AUDIO_DEVICE_IN_BACK_MIC, "IN_BACK_MIC" },
    { AUDIO_DEVICE_IN_REMOTE_SUBMIX, "IN_REMOTE_SUBMIX" },
    { AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET, "IN_ANLG_DOCK_HEADSET" },
    { AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET, "IN_DGTL_DOCK_HEADSET" },
    { AUDIO_DEVICE_IN_USB_ACCESSORY, "IN_USB_ACCESSORY" },
    { AUDIO_DEVICE_IN_USB_DEVICE, "IN_USB_DEVICE" },
    { AUDIO_DEVICE_IN_FM_TUNER, "IN_FM_TUNER" },
    { AUDIO_DEVICE_IN_TV_TUNER, "IN_TV_TUNER" },
    { AUDIO_DEVICE_IN_LINE, "IN_LINE" },
    { AUDIO_DEVICE_IN_SPDIF, "IN_SPDIF" },
    { AUDIO_DEVICE_IN_BLUETOOTH_A2DP, "IN_BLUETOOTH_A2DP" },
    { AUDIO_DEVICE_IN_LOOPBACK, "IN_LOOPBACK" },
    { AUDIO_DEVICE_IN_IP, "IN_IP" },
    { AUDIO_DEVICE_IN_BUS, "IN_BUS" },
    { AUDIO_DEVICE_IN_PROXY, "IN_PROXY" },
    { AUDIO_DEVICE_IN_USB_HEADSET, "IN_USB_HEADSET" },
    { AUDIO_DEVICE_IN_BLUETOOTH_BLE, "IN_BLUETOOTH_BLE" },
    { AUDIO_DEVICE_IN_DEFAULT, "IN_DEFAULT" },
    { 0, NULL }
};

const char * xa_dbgstr_device(audio_devices_t value)
{
    return xa_dbgstr(value, &xa_dbg_device[0]);
}
