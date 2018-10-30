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


/* default configuration used for initialization purposes */
const struct pcm_config xa_config_default = {
    .channels = 2,
    .rate = 48000,
    .period_size = HW_PERIOD_SIZE,
    .period_count = HW_PERIODS_PER_BUFFER,
    .format = PCM_FORMAT_S16_LE,
};

const unsigned int xa_supported_channels[] = {1, 2};
/* sample rates */
const unsigned int xa_supported_rates[] = {48000};
/* android-side formats */
const unsigned int xa_supported_aformats[] = {AUDIO_FORMAT_PCM_16_BIT};


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
    size = sizeof(xa_supported_channels)/sizeof(xa_supported_channels[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_channels[i] == channels) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported number of channels: %d", channels);
        return false;
    }

    /* sample rate */
    size = sizeof(xa_supported_rates)/sizeof(xa_supported_rates[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_rates[i] == config->sample_rate) {
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
    size = sizeof(xa_supported_channels)/sizeof(xa_supported_channels[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_channels[i] == channels) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported number of channels: %d", channels);
        return false;
    }

    /* sample rate */
    size = sizeof(xa_supported_rates)/sizeof(xa_supported_rates[0]);
    for (i = 0; i < size; i++) {
        if (xa_supported_rates[i] == config->sample_rate) {
            break;
        }
    }
    if (i == size) {
        ALOGE("Not supported sample rate: %d", config->sample_rate);
        return false;
    }

    return true;
}

