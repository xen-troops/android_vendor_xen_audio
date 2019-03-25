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

#ifndef VENDOR_XEN_AUDIO_STREAM_IN_H_
#define VENDOR_XEN_AUDIO_STREAM_IN_H_

#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>

typedef struct x_stream_in {
    /* NOTE audio_stream_in_t MUST be very first member of structure */
    audio_stream_in_t astream;
    pthread_mutex_t lock;

    /* pcm related data (bellow us) */
    struct pcm_config p_config;
    struct pcm * p_handle;
    /* card and device used for pcm_open
       should correlate with a_dev field */
    unsigned int p_card_id;
    unsigned int p_dev_id;

    /* android related data (above us) */
    audio_devices_t a_dev;  /* see p_card_id above */
    audio_channel_mask_t a_channel_mask;
    audio_format_t a_format;

    /* internal variables */
    bool standby;
    size_t frame_size;
    bool muted;
    int64_t read_frames;
    int64_t last_time;
} x_stream_in_t;

/**
 * Create input stream.
 * Allocate required resources and initialize structures.
 *
 * Function is supposed to be called by audio device only.
 */
int in_create(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        unsigned int slot,
        struct audio_config *config,
        struct audio_stream_in **stream_in);

/**
 * Free resources allocated by stream and destroy stream itself.
 *
 * Function is supposed to be called by audio device only.
 */
void in_destroy(x_stream_in_t *xin);

/**
 * Mute/unmute input stream
 *
 * Function is supposed to be called by audio device only.
 */
void in_set_mute(x_stream_in_t *xin, bool mute);

#endif /* VENDOR_XEN_AUDIO_STREAM_IN_H_ */
