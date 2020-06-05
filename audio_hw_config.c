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
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#define LOG_TAG "xa_config"

/* standard headers */
/* android headers */
#include <log/log.h>
#include <tinyalsa/asoundlib.h>
/* local headers*/
#include "audio_hw_config.h"


/* map for input devices */
xa_device_map_t xa_input_map[NUMBER_OF_DEVICES_IN] =
{
    /* Built-In Mic */
    {AUDIO_DEVICE_IN_BUILTIN_MIC, 0, 0, 0, 512, 2},
    /* Line In */
    {AUDIO_DEVICE_IN_LINE, 0, 0, 0, 512, 2},
    /* bus0_in */
    {AUDIO_DEVICE_IN_BUS, 0, 0, 0, 512, 2},
};

/* map for output devices */
/* timings for 48000:
 *    256 frames -   5.33 ms - fast (<= 5 ms)
 *    512 frames -  10.66 ms
 *   1024 frames -  21.33 ms - normal (~20 ms)
 *   2048 frames -  42.67 ms
 *   8192 frames - 170.66 ms - deep (>= 100 ms)
 */
xa_device_map_t xa_output_map[NUMBER_OF_DEVICES_OUT] =
{
    /* media */
    {AUDIO_DEVICE_OUT_BUS, 0, 0, 0, 2048, 4},
    /* navigation */
    {AUDIO_DEVICE_OUT_BUS, 1, 0, 1, 1024, 2},
    /* voice command */
    {AUDIO_DEVICE_OUT_BUS, 2, 0, 2, 1024, 2},
    /* call ring */
    {AUDIO_DEVICE_OUT_BUS, 3, 0, 3, 1024, 2},
    /* call */
    {AUDIO_DEVICE_OUT_BUS, 4, 0, 4, 1024, 2},
    /* alarm */
    {AUDIO_DEVICE_OUT_BUS, 5, 0, 5, 1024, 2},
    /* notification */
    {AUDIO_DEVICE_OUT_BUS, 6, 0, 6, 1024, 2},
    /* system sound */
    {AUDIO_DEVICE_OUT_BUS, 7, 0, 7, 1024, 2},
};

const unsigned int xa_supported_channels_out[] = {1, 2};
const unsigned int xa_supported_channels_in[] = {1, 2};
/* sample rates */
const unsigned int xa_supported_rates_out[] = {8000, 11025, 12000, 16000, 22050, 32000, 44100, 48000};
const unsigned int xa_supported_rates_in[] = {8000, 11025, 12000, 16000, 22050, 32000, 44100, 48000};
/* android-side formats */
const unsigned int xa_supported_aformats[] = {AUDIO_FORMAT_PCM_16_BIT};

void get_supported_in_rates_as_string(char * pbuf, size_t buf_size)
{
    int i;
    char temp[10];
    for (i = 0; i < sizeof(xa_supported_rates_in)/sizeof(xa_supported_rates_in[0]); i++) {
        sprintf(temp, "%d|", xa_supported_rates_in[i]);
        if ((strlen(pbuf) + strlen(temp) + 1) < buf_size) {
            strcat(pbuf, temp);
        }
    }
    if (strlen(pbuf) > 0) {
        /* remove last '|' */
        pbuf[strlen(pbuf)-1] = 0;
    }
}

void get_supported_out_rates_as_string(char * pbuf, size_t buf_size)
{
    int i;
    char temp[10];
    for (i = 0; i < sizeof(xa_supported_rates_out)/sizeof(xa_supported_rates_out[0]); i++) {
        sprintf(temp, "%d|", xa_supported_rates_out[i]);
        if ((strlen(pbuf) + strlen(temp) + 1) < buf_size) {
            strcat(pbuf, temp);
        }
    }
    if (strlen(pbuf) > 0) {
        /* remove last '|' */
        pbuf[strlen(pbuf)-1] = 0;
    }
}

/* check that parameters are supported for input stream */
bool is_config_supported_in(const audio_config_t * config)
{
    uint16_t i = 0;
    uint16_t size = 0;
    uint32_t channels = 0;

    /* format */
    size = sizeof(xa_supported_aformats)/sizeof(xa_supported_aformats[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_aformats[i] == config->format) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported format: %d", config->format);
        return false;
    }

    /* channels */
    channels = audio_channel_count_from_in_mask(config->channel_mask);
    size = sizeof(xa_supported_channels_in)/sizeof(xa_supported_channels_in[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_channels_in[i] == channels) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported number of channels: %d", channels);
        return false;
    }

    /* sample rate */
    size = sizeof(xa_supported_rates_in)/sizeof(xa_supported_rates_in[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_rates_in[i] == config->sample_rate) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported sample rate: %d", config->sample_rate);
        return false;
    }

    return true;
}

/* check that parameters are supported for output stream */
bool is_config_supported_out(const audio_config_t * config)
{
    uint16_t i = 0;
    uint16_t size = 0;
    uint32_t channels = 0;

    /* format */
    size = sizeof(xa_supported_aformats)/sizeof(xa_supported_aformats[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_aformats[i] == config->format) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported format: %d", config->format);
        return false;
    }

    /* channels */
    channels = audio_channel_count_from_out_mask(config->channel_mask);
    size = sizeof(xa_supported_channels_out)/sizeof(xa_supported_channels_out[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_channels_out[i] == channels) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported number of channels: %d", channels);
        return false;
    }

    /* sample rate */
    size = sizeof(xa_supported_rates_out)/sizeof(xa_supported_rates_out[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_rates_out[i] == config->sample_rate) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported sample rate: %d", config->sample_rate);
        return false;
    }

    return true;
}

int find_out_device(audio_devices_t devices, const char *address)
{
    int slot;
    unsigned int bus_number;

    if ((devices & AUDIO_DEVICE_OUT_BUS) == AUDIO_DEVICE_OUT_BUS) {
        /* We expect that address has format "bus%d_%s".
         * In other words, we expect that bus address starts with 'bus',
         * followed by bus number, which is followed by '_' and voluntary description.
         * Parser in car audio service was used as reference. */
        if (sscanf(address, "bus%u", &bus_number) == 1) {
            /* bus number is properly recognized and stored for further comparison */
        } else {
            /* bus address is incorrect, we can't find device */
            ALOGW("%s: 'address' has not supported format and will be skipped."
                  " Expected: 'bus%%d_%%s': 'bus' word, bus number, '_', description.",
                  __FUNCTION__);
            return -1;
        }
    }

    if ((devices & AUDIO_DEVICE_OUT_DEFAULT) == AUDIO_DEVICE_OUT_DEFAULT) {
        /* We consider first device as default */
        return 0;
    }

    /* Identify PCM card and device */
    /* We iterate through xa_output_map looking for device_type.
     * If device type is audio bus, we check bus number also. */
    for (slot = 0; slot < NUMBER_OF_DEVICES_OUT; slot++) {
        if ((devices & xa_output_map[slot].device_type_mask) == xa_output_map[slot].device_type_mask) {
            if (xa_output_map[slot].device_type_mask == AUDIO_DEVICE_OUT_BUS) {
                if (bus_number == xa_output_map[slot].bus_number) {
                    /* device is identified, stop scanning of map */
                    break;
                }
            } else {
                /* device is identified, stop scanning of map */
                break;
            }
        }
    }
    if (slot >=  NUMBER_OF_DEVICES_OUT) {
        slot = -1;
    }
    return slot;
}

int find_in_device(audio_devices_t devices, const char *address)
{
    int slot;
    unsigned int bus_number;

    if ((devices & AUDIO_DEVICE_IN_BUS) == AUDIO_DEVICE_IN_BUS) {
        /* We expect that address has format "bus%d_%s".
         * In other words, we expect that bus address starts with 'bus',
         * followed by bus number, which is followed by '_' and voluntary description.
         * Parser in car audio service was used as reference. */
        if (sscanf(address, "bus%u", &bus_number) == 1) {
            /* bus number is properly recognized */
        } else {
            /* bus address is incorrect, we can't find device */
            ALOGW("%s: 'address' has not supported format and will be skipped."
                  " Expected: 'bus%%d_%%s': 'bus' word, bus number, '_', description.",
                  __FUNCTION__);
            return -1;
        }
    }

    if ((devices & AUDIO_DEVICE_IN_DEFAULT) == AUDIO_DEVICE_IN_DEFAULT) {
        /* We consider first device as default */
        return 0;
    }

    /* Identify PCM card and device */
    /* We iterate through xa_input_map looking for device_type.
     * If device type is audio bus, we check bus number also. */
    for (slot = 0; slot < NUMBER_OF_DEVICES_IN; slot++) {
        if ((devices & xa_input_map[slot].device_type_mask) == xa_input_map[slot].device_type_mask) {
            if (xa_input_map[slot].device_type_mask == AUDIO_DEVICE_IN_BUS) {
                if (bus_number == xa_input_map[slot].bus_number) {
                    /* device is identified, stop scanning of map */
                    break;
                }
            } else {
                /* device is identified, stop scanning of map */
                break;
            }
        }
    }
    if (slot >=  NUMBER_OF_DEVICES_IN) {
        slot = -1;
    }
    return slot;
}
