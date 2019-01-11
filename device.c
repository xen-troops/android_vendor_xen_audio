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

/*
 * This module implements functions for 'struct audio_hw_device',
 * so please see hardware/audio.h for description of function's
 * behavior and expected results.
 */

#define LOG_TAG "xa_device"

/* standard headers */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
/* android headers */
#include <log/log.h>
#include <tinyalsa/asoundlib.h>
/* local headers*/
#include "audio_hw_config.h"
#include "dbg_func_traces.h"
#include "device.h"
#include "stream_out.h"
#include "stream_in.h"

/* Number of supported input devices. Should be in sync with
 * number of input devices in audio_policy_configuration.xml */
#define NUMBER_OF_DEVICES_IN 3

/* Number of supported output devices. Should be in sync with
 * number of output buses in audio_policy_configuration.xml */
#define NUMBER_OF_DEVICES_OUT 8

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
} xa_device_map_t;

/* map for input devices */
xa_device_map_t xa_input_map[NUMBER_OF_DEVICES_IN] =
{
    {AUDIO_DEVICE_IN_BUILTIN_MIC, 0, 0, 0},
    {AUDIO_DEVICE_IN_LINE, 0, 0, 0},
    {AUDIO_DEVICE_IN_BUS, 0, 0, 0},
};


typedef struct x_audio_device {
    /* NOTE: audio_hw_device_t MUST be very first member of structure */
    audio_hw_device_t hw_device;
    pthread_mutex_t lock;
    /* device specific properties */
    uint32_t period_size;
    uint32_t periods_per_buffer;
    bool mic_is_muted;
    bool master_is_muted;
    x_stream_in_t * xin_streams[NUMBER_OF_DEVICES_IN];
    x_stream_out_t * xout_streams[NUMBER_OF_DEVICES_OUT];
} x_audio_device_t;


/* forward declaration required for adev_open() */
int adev_close(hw_device_t *device);


/* internal variables */
static pthread_mutex_t xadev_init_lock = PTHREAD_MUTEX_INITIALIZER;
static x_audio_device_t *xadev = NULL;
static unsigned int ref_counter = 0;


int adev_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    LOG_FN_NAME_WITH_ARGS("(%p, name:'%s')", module, name);

    /* check inputs */
    if ((module == NULL) || (name == NULL) || (device == NULL)) {
        return -EINVAL;
    }
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) {
        return -EINVAL;
    }

    pthread_mutex_lock(&xadev_init_lock);

    if (ref_counter != 0) {
        *device = &xadev->hw_device.common;
        ALOGD("Return existing device:%p", *device);
    } else {
        /* initialize new audio device */
        xadev = calloc(1, sizeof(*xadev));
        if (xadev == NULL) {
            *device = NULL;
            pthread_mutex_unlock(&xadev_init_lock);
            return -ENOMEM;
        }

        pthread_mutex_init(&xadev->lock, NULL);
        xadev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
        xadev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_3_0;
        xadev->hw_device.common.module = (hw_module_t*)module;
        xadev->hw_device.common.close = adev_close;
        *device = &xadev->hw_device.common;
        ALOGD("Created device:%p", *device);

        xadev->hw_device.get_supported_devices = adev_get_supported_devices;
        xadev->hw_device.init_check = adev_init_check;
        xadev->hw_device.set_voice_volume = adev_set_voice_volume;
        xadev->hw_device.set_master_volume = adev_set_master_volume;
        xadev->hw_device.get_master_volume = adev_get_master_volume;
        xadev->hw_device.set_mode = adev_set_mode;
        xadev->hw_device.set_mic_mute = adev_set_mic_mute;
        xadev->hw_device.get_mic_mute = adev_get_mic_mute;
        xadev->hw_device.set_parameters = adev_set_parameters;
        xadev->hw_device.get_parameters = adev_get_parameters;
        xadev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
        xadev->hw_device.open_output_stream = adev_open_output_stream;
        xadev->hw_device.close_output_stream = adev_close_output_stream;
        xadev->hw_device.open_input_stream = adev_open_input_stream;
        xadev->hw_device.close_input_stream = adev_close_input_stream;
        xadev->hw_device.get_microphones = adev_get_microphones;
        xadev->hw_device.dump = adev_dump;
        xadev->hw_device.set_master_mute = adev_set_master_mute;
        xadev->hw_device.get_master_mute = adev_get_master_mute;
        xadev->hw_device.create_audio_patch = adev_create_audio_patch;
        xadev->hw_device.release_audio_patch = adev_release_audio_patch;
        xadev->hw_device.get_audio_port = adev_get_audio_port;
        xadev->hw_device.set_audio_port_config = adev_set_audio_port_config;

        xadev->period_size = HW_PERIOD_SIZE;
        xadev->periods_per_buffer = HW_PERIODS_PER_BUFFER;
        /* following fields are cleared by calloc:
            xadev->mic_is_muted
            xadev->master_is_muted
            xadev->xin_stream
            xadev->xout_streams
        */
    }
    ref_counter++;
    pthread_mutex_unlock(&xadev_init_lock);
    return 0;
}

int adev_close(hw_device_t *device)
{
    int i = 0;

    LOG_FN_NAME_WITH_ARGS("(%p)", device);

    if ((device == NULL) || (device != (hw_device_t*)xadev)) {
        return -EINVAL;
    }

    pthread_mutex_lock(&xadev_init_lock);

    if (ref_counter == 0) {
        ALOGE("Nothing to close (ref_counter==0)");
        pthread_mutex_unlock(&xadev_init_lock);
        return -EINVAL;
    }

    ref_counter--;
    if (ref_counter == 0) {
        /* close streams and release resources */
        for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
            if (xadev->xin_streams[i] != NULL) {
                in_destroy(xadev->xin_streams[i]);
                xadev->xin_streams[i] = NULL;
            }
        }
        for (i = 0; i < NUMBER_OF_DEVICES_OUT; i++) {
            if (xadev->xout_streams[i] != NULL) {
                out_destroy(xadev->xout_streams[i]);
                xadev->xout_streams[i] = NULL;
            }
        }
        pthread_mutex_destroy(&xadev->lock);
        free(xadev);
        xadev = NULL;
    } else {
        ALOGD("Decremented ref_counter:%d", ref_counter);
    }

    pthread_mutex_unlock(&xadev_init_lock);

    return 0;
}

uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    return (AUDIO_DEVICE_OUT_BUS | AUDIO_DEVICE_OUT_DEFAULT |
            AUDIO_DEVICE_IN_BUILTIN_MIC | AUDIO_DEVICE_IN_LINE | AUDIO_DEVICE_IN_BUS |
            AUDIO_DEVICE_IN_DEFAULT);
}

int adev_init_check(const struct audio_hw_device *dev)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    return 0;
}

int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %f)", dev, volume);
    /* can't be implemented on current configuration */
    return -ENOSYS;
}

int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %f)", dev, volume);
    /* can't be implemented on current configuration */
    return -ENOSYS;
}

int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    /* can't be implemented on current configuration */
    return -ENOSYS;
}

int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %d)", dev, mode);
    /* we have no special handling for now */
    return 0;
}

int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    unsigned int i;

    LOG_FN_NAME_WITH_ARGS("(%p, %s)", dev, state ? "mute" : "unmute");
    if (dev == NULL) {
        return -EINVAL;
    }
    pthread_mutex_lock(&xdev->lock);
    xdev->mic_is_muted = state;
    for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
        if (xdev->xin_streams[i] != NULL) {
            in_set_mute(xdev->xin_streams[i], state);
            xdev->xin_streams[i] = NULL;
        }
    }
    pthread_mutex_unlock(&xdev->lock);
    return 0;
}

int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    if ((dev == NULL) || (state == NULL)) {
        return -EINVAL;
    }
    pthread_mutex_lock(&xdev->lock);
    *state = xdev->mic_is_muted;
    pthread_mutex_unlock(&xdev->lock);
    return 0;
}

int adev_set_parameters(struct audio_hw_device *dev, const char *kv_pairs)
{
    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", dev, kv_pairs);
    /* we have no special handling for now */
    return 0;
}

char * adev_get_parameters(const struct audio_hw_device *dev, const char *keys)
{
    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", dev, keys);
    /* we have no special handling for now */
    return strdup("");
}

size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                const struct audio_config *config)
{
    size_t buf_size = 0;
    uint32_t channels = xa_config_default.channels;
    unsigned int sample_size = 0;
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    LOG_FN_NAME_WITH_ARGS(
            "(%p, rate:%d, channel_mask:0x%x, format:0x%x, offload.size:%d, frame_count:%d)",
            dev, config->sample_rate, config->channel_mask, config->format,
            config->offload_info.size, config->frame_count);

    if (!is_config_supported_in(config)) {
        return 0;
    }

    channels = audio_channel_count_from_in_mask(config->channel_mask);
    sample_size = pcm_format_to_bits(config->format)/8;
    buf_size = xdev->period_size * channels * sample_size;

    return buf_size;
}

int adev_open_output_stream(struct audio_hw_device *dev,
                          audio_io_handle_t handle,
                          audio_devices_t devices,
                          audio_output_flags_t flags,
                          struct audio_config *config,
                          struct audio_stream_out **stream_out,
                          const char *address)
{
    int res = 0;
    unsigned int dev_id = NUMBER_OF_DEVICES_OUT;  /* out of range value */
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    LOG_FN_NAME_WITH_ARGS(
            "(%p, handle:%d, devices:0x%x, flags:0x%x, "
            "rate:%d, channel_mask:0x%x, format:0x%x, address:'%s')",
            dev, handle, devices, flags,
            config->sample_rate, config->channel_mask, config->format,
            address);

    pthread_mutex_lock(&xdev->lock);

    /* is requested configuration supported? */

    /* check input parameters*/
    if (!is_config_supported_out(config)) {
        ALOGE("Failed. Not supported audio configuration. -EINVAL");
        *stream_out = NULL;
        pthread_mutex_unlock(&xdev->lock);
        return -EINVAL;
    }
    /* check requested device */
    if ((devices & AUDIO_DEVICE_BIT_IN) != 0) {
        ALOGE("Failed. Only AUDIO_DEVICE_OUT_* device type is supported.");
        *stream_out = NULL;
        pthread_mutex_unlock(&xdev->lock);
        return -EINVAL;
    }
    /* For now we have one card, so card id is always constant. Get device id,
     * taking into consideration two predefined scenarios:
     * - We have buses. In this case get device id from address parameter.
     * - We have no buses. In this case we assume single output device. */
    if ((devices & AUDIO_DEVICE_OUT_BUS) != AUDIO_DEVICE_OUT_BUS) {
        /* If requested device is not bus then open it on pcmC0D0 */
        dev_id = 0;
    } else {
        /* We expect, same as parser in car audio service, that address has format "bus%d_%s".
         * In other words, we expect that bus address starts with 'bus',
         * followed by bus number, which is followed by '_' and voluntary description. */
        if (sscanf(address, "bus%u", &dev_id) != 1) {
            ALOGE("Address format is not supported."
                  "Expect 'bus%%d_%%s': 'bus' word, bus number, '_', description.");
            dev_id = NUMBER_OF_DEVICES_OUT;
        }
    }

    if (dev_id < NUMBER_OF_DEVICES_OUT) {
        if (xdev->xout_streams[dev_id] == 0) {
            /* create new stream on free device */
            res = out_create(dev, handle, devices, CARD_ID, dev_id, config, stream_out);
            if (*stream_out != NULL) {
                xdev->xout_streams[dev_id] = (x_stream_out_t*)(*stream_out);
                if (xdev->master_is_muted) {
                    out_set_mute(xdev->xout_streams[dev_id], true);
                }
                ALOGD("Created stream_out:%p", *stream_out);
            }
            /* if out_create failed then 'res' has 'failed' value and it will be reported up */
        } else {
            res = -EEXIST;
            ALOGE("Output stream for this device already exists.");
        }
    } else {
        res = -EINVAL;
        ALOGE("Can't create output stream on incorrect device.");
    }

    pthread_mutex_unlock(&xdev->lock);
    return res;
}

void adev_close_output_stream(struct audio_hw_device *dev, struct audio_stream_out* stream_out)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    int i = 0;

    LOG_FN_NAME_WITH_ARGS("(%p, %p)", dev, stream_out);
    if ((dev == NULL) || (stream_out == NULL)) {
        return;
    }
    pthread_mutex_lock(&xdev->lock);
    for (i = 0; i < NUMBER_OF_DEVICES_OUT; i++) {
        if (xdev->xout_streams[i] == (x_stream_out_t*)stream_out) {
            out_destroy(xdev->xout_streams[i]);
            xdev->xout_streams[i] = NULL;
            break;
        }
    }
    if (i == NUMBER_OF_DEVICES_OUT) {
        ALOGE("close_output_stream() called for unknown stream: %p", stream_out);
    }
    pthread_mutex_unlock(&xdev->lock);
}

int adev_open_input_stream(struct audio_hw_device *dev,
                         audio_io_handle_t handle,
                         audio_devices_t devices,
                         struct audio_config *config,
                         struct audio_stream_in **stream_in,
                         audio_input_flags_t flags,
                         const char *address,
                         audio_source_t source)
{
    int res = 0;
    unsigned int pcm_card = CARD_ID;
    unsigned int pcm_device = DEVICE_ID_RECORD;
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    unsigned int slot;

    pthread_mutex_lock(&xdev->lock);

    LOG_FN_NAME_WITH_ARGS("(%p, handle:0x%x, devices:0x%x, flags:0x%x, address:'%s', source:%d)",
            dev, handle, devices, flags, address, source);
    LOG_FN_PARAMETERS("Rate:%d, channel_mask:0x%x, format:0x%x, offload.size:%d, frame_count:%d ",
            config->sample_rate, config->channel_mask, config->format,
            config->offload_info.size, config->frame_count);

    /* is requested configuration supported? */

    /* check input parameters */
    if (!is_config_supported_in(config)) {
        ALOGE("Failed. Not supported audio configuration. -EINVAL");
        *stream_in = NULL;
        pthread_mutex_unlock(&xdev->lock);
        return -EINVAL;
    }
    /* check requested device */
    if ((devices & AUDIO_DEVICE_BIT_IN) == 0) {
        ALOGE("Failed. Incorrect device type. -EINVAL");
        *stream_in = NULL;
        pthread_mutex_unlock(&xdev->lock);
        return -EINVAL;
    }

    /* Identify pcm device */
    for (slot = 0; slot < NUMBER_OF_DEVICES_IN; slot++) {
        if ((devices & xa_input_map[slot].device_type_mask) != 0) {
            if (xa_input_map[slot].device_type_mask == AUDIO_DEVICE_IN_BUS) {
                if (sscanf(address, "bus%u", &pcm_device) == 1) {
                    pcm_card = xa_input_map[slot].pcm_card;
                    pcm_device = xa_input_map[slot].pcm_device;
                    /* device is identified, stop scanning of map */
                    break;
                } else {
                    /* if bus address is incorrect, continue scanning of map */
                }
            } else {
                pcm_card = xa_input_map[slot].pcm_card;
                pcm_device = xa_input_map[slot].pcm_device;
                /* device is identified, stop scanning of map */
                break;
            }
        }
    }

    if (slot < NUMBER_OF_DEVICES_IN) {
        if (xdev->xin_streams[slot] == NULL) {
            res = in_create(dev, handle, devices, pcm_card, pcm_device, config, stream_in);
            if (*stream_in != NULL) {
                xdev->xin_streams[slot] = (x_stream_in_t*)(*stream_in);
                if (xdev->mic_is_muted) {
                    in_set_mute(xdev->xin_streams[slot], true);
                }
            }
            LOG_FN_PARAMETERS("Created stream_in:%p", *stream_in);
        } else {
            res = -EEXIST;
            ALOGE("Input stream for this device already exists.");
        }
    } else {
        res = -EINVAL;
        ALOGE("Can't create input stream on incorrect device.");
    }

    pthread_mutex_unlock(&xdev->lock);
    return res;
}

void adev_close_input_stream(struct audio_hw_device *dev, struct audio_stream_in *stream_in)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    unsigned int i;

    LOG_FN_NAME_WITH_ARGS("(%p, %p)", dev, stream_in);
    if ((dev == NULL) || (stream_in == NULL)) {
        return;
    }
    pthread_mutex_lock(&xdev->lock);
    for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
        if (xdev->xin_streams[i] == (x_stream_in_t*)stream_in) {
            in_destroy(xdev->xin_streams[i]);
            xdev->xin_streams[i] = NULL;
            break;
        }
    }
    if (i == NUMBER_OF_DEVICES_IN) {
        ALOGE("close_input_stream() called for unknown stream: %p", stream_in);
    }
    pthread_mutex_unlock(&xdev->lock);
}

int adev_get_microphones(const struct audio_hw_device *dev,
                       struct audio_microphone_characteristic_t *mic_array,
                       size_t *mic_count)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    /* TODO To implement */
    return -ENOSYS;
}

int adev_dump(const struct audio_hw_device *dev, int fd)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    int i;

    LOG_FN_NAME_WITH_ARGS("(%p, fd:%d)", dev, fd);

    pthread_mutex_lock(&xdev->lock);
    dprintf(fd, "    audio_hw_device_t: %p\n", &xdev->hw_device);
    dprintf(fd, "    period_size: %u\n", xdev->period_size);
    dprintf(fd, "    periods_per_buffer: %u\n", xdev->periods_per_buffer);
    dprintf(fd, "    mic_is_muted: %s\n", xdev->mic_is_muted ? "true" : "false");
    dprintf(fd, "    master_is_muted: %s\n", xdev->master_is_muted ? "true" : "false");
    for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
        dprintf(fd, "    in_stream[%d]: %p\n", i, xdev->xin_streams[i]);
    }
    for (i = 0; i < NUMBER_OF_DEVICES_OUT; i++) {
        dprintf(fd, "    out_stream[%d]: %p\n", i, xdev->xout_streams[i]);
    }
    pthread_mutex_unlock(&xdev->lock);

    return 0;
}

int adev_set_master_mute(struct audio_hw_device *dev, bool mute)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    int i = 0;

    LOG_FN_NAME_WITH_ARGS("(%p, %s)", dev, mute ? "mute" : "unmute");
    if (dev == NULL) {
        return -EINVAL;
    }
    pthread_mutex_lock(&xdev->lock);
    xdev->master_is_muted = mute;
    for (i = 0; i < NUMBER_OF_DEVICES_OUT; i++) {
        if (xdev->xout_streams[i] != NULL) {
            out_set_mute(xdev->xout_streams[i], mute);
        }
    }
    pthread_mutex_unlock(&xdev->lock);
    return 0;
}

int adev_get_master_mute(struct audio_hw_device *dev, bool *mute)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    LOG_FN_NAME_WITH_ARGS("(%p)", dev);
    if ((dev == NULL) || (mute == NULL)) {
        return -EINVAL;
    }
    pthread_mutex_lock(&xdev->lock);
    *mute = xdev->master_is_muted;
    pthread_mutex_unlock(&xdev->lock);
    return 0;
}

int adev_create_audio_patch(struct audio_hw_device *dev,
                           unsigned int num_sources,
                           const struct audio_port_config *sources,
                           unsigned int num_sinks,
                           const struct audio_port_config *sinks,
                           audio_patch_handle_t *handle)
{
    unsigned int i = 0;
    LOG_FN_NAME_WITH_ARGS("(%p, #sources:%d, #sinks:%d, *handle:%d)",
            dev, num_sources, num_sinks, *handle);
    for (i = 0; i < num_sources; i++) {
        LOG_FN_PARAMETERS(
                "Source[%d].id:%d, role:%d, type:%d, config_mask:0x%x, "
                "rate:%d, channel_mask:0x%x, format:%d, gain, ext",
                i, sources[i].id, sources[i].role, sources[i].type, sources[i].config_mask,
                sources[i].sample_rate, sources[i].channel_mask, sources[i].format);
    }
    for (i = 0; i < num_sinks; i++) {
        LOG_FN_PARAMETERS("Sink[%d].id:%d, role:%d, type:%d, "
                "config_mask:0x%x, rate:%d, channel_mask:0x%x, format:%d, gain, ext",
                i, sinks[i].id, sources[i].role, sinks[i].type, sinks[i].config_mask,
                sinks[i].sample_rate, sinks[i].channel_mask, sinks[i].format);
    }

    /* for now we can only simulate that we created patch,
       so let's return id of first sink */
    *handle = sinks[0].id;
    return 0;
}

int adev_release_audio_patch(struct audio_hw_device *dev, audio_patch_handle_t handle)
{
    LOG_FN_NAME_WITH_ARGS("(%p, patch:%d)", dev, handle);
    /* can't be implemented for current configuration,
     * so we will simulate success */
    return 0;
}

int adev_get_audio_port(struct audio_hw_device *dev, struct audio_port *port)
{
    LOG_FN_NAME_WITH_ARGS("(%p, port:%p)", dev, port);
    /* TODO To implement */
    return -ENOSYS;
}

int adev_set_audio_port_config(struct audio_hw_device *dev, const struct audio_port_config *config)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    if ((dev == NULL) || (config == NULL)) {
        return -EINVAL;
    }

    LOG_FN_NAME_WITH_ARGS("(%p, id:%d, role:%d, type:%d, config_mask:0x%x)",
            dev, config->id, config->role, config->type, config->config_mask);

    /* check that we are called for correct port type */
    switch (config->type) {
    case AUDIO_PORT_TYPE_DEVICE:
        /* set config for audio_port_config_device_ext */
        break;
    case AUDIO_PORT_TYPE_MIX:
        /* can't set config for audio_port_config_mix_ext */
        LOG_FN_PARAMETERS("Not supported port type.");
        return -ENOSYS;
    case AUDIO_PORT_TYPE_SESSION:
        /* can't set config for audio_port_config_session_ext */
        LOG_FN_PARAMETERS("Not supported port type.");
        return -ENOSYS;
    default:
        /* can't set config for not clear port type */
        LOG_FN_PARAMETERS("Not supported port type.");
        return -ENOSYS;
    }

    pthread_mutex_lock(&xdev->lock);
    /* what should be configured? */
    if ((config->config_mask & AUDIO_PORT_CONFIG_SAMPLE_RATE) != 0) {
        LOG_FN_PARAMETERS("rate:%d", config->sample_rate);
        /* has supported value? */
        /* set new value */
    }
    if ((config->config_mask & AUDIO_PORT_CONFIG_CHANNEL_MASK) != 0) {
        LOG_FN_PARAMETERS("channel_mask:0x%x", config->channel_mask);
        /* has supported value? */
        /* set new value */
    }
    if ((config->config_mask & AUDIO_PORT_CONFIG_FORMAT) != 0) {
        LOG_FN_PARAMETERS("format:%d", config->format);
        /* has supported value? */
        /* set new value */
    }
    if ((config->config_mask & AUDIO_PORT_CONFIG_GAIN) != 0) {
        LOG_FN_PARAMETERS("gain.index:%d, .mode:%d, .channel_mask:0x%x, values[], .ramp:%d",
        config->gain.index, config->gain.mode, config->gain.channel_mask,
        config->gain.ramp_duration_ms);
        /* not possible to set gain in current configuration */
    }
    pthread_mutex_unlock(&xdev->lock);

    return 0;
}
