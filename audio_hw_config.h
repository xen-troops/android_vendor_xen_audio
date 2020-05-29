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

#ifndef VENDOR_XEN_AUDIO_AUDIO_HW_CONFIG_H_
#define VENDOR_XEN_AUDIO_AUDIO_HW_CONFIG_H_

#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>

/* period size is number of frames per one hardware cycle */
#define HW_PERIOD_SIZE  2048
/* number of periods per one ring buffer */
#define HW_PERIODS_PER_BUFFER  4

/* Number of supported input devices. Should be in sync with
 * number of input devices in audio_policy_configuration.xml */
#define NUMBER_OF_DEVICES_IN 3

/* Number of supported output devices. Should be in sync with
 * number of output buses in audio_policy_configuration.xml */
#define NUMBER_OF_DEVICES_OUT 8

#define DEFAULT_PCM_FORMAT PCM_FORMAT_S16_LE

/* This structure is used for mapping of android's audio devices to 'hardware' devices.
 * So, request for open stream on specified device is checked using this map.
 * If device type and bus number (for in/out buses) match to request then
 * stream will be open on specified card and device (pcmC*D*). */
typedef struct xa_device_map
{
    /* One of the AUDIO_DEVICE_XXX */
    audio_devices_t device_type_mask;
    /* Used only for in/out buses. If device type is AUDIO_DEVICE_*_BUS,
     * then bus number will be checked (e.g. 5 for bus5_out_alarm) */
    unsigned int bus_number;
    /* pcm card index, as used in /dev/snd/pcmC*D* */
    unsigned int pcm_card;
    /* pcm device index, as used in /dev/snd/pcmC*D* */
    unsigned int pcm_device;
    /* period size is specified in frames, not bytes
     * sizeof(period) = period_size * sizeof(frame);
     * sizeof(frame) = sizeof(sample) * number_of_channels */
    unsigned int period_size;
    /* number of periods per buffer */
    unsigned int periods_per_buffer;
} xa_device_map_t;


/* map for input devices */
extern xa_device_map_t xa_input_map[NUMBER_OF_DEVICES_IN];

/* map for output devices */
extern xa_device_map_t xa_output_map[NUMBER_OF_DEVICES_OUT];

/* return true if config is supported for input stream */
bool is_config_supported_in(const audio_config_t * config);

/* return true if config is supported for output stream */
bool is_config_supported_out(const audio_config_t * config);

/* find appropriate slot for specified device in xa_output_map
 * devices - can be bitmask
 * address - parsed only for AUDIO_DEVICE_OUT_BUS, expected format 'bus0_media' */
int find_out_device(audio_devices_t devices, const char *address);

/* find appropriate slot for specified device in xa_input_map
 * devices - can be bitmask
 * address - parsed only for AUDIO_DEVICE_IN_BUS, expected format 'bus0_in' */
int find_in_device(audio_devices_t devices, const char *address);

#endif /* VENDOR_XEN_AUDIO_AUDIO_HW_CONFIG_H_ */
