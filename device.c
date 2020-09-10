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
#include <cutils/str_parms.h>
#include <tinyalsa/asoundlib.h>
/* local headers*/
#include "audio_hw_config.h"
#include "dbg_func_traces.h"
#include "device.h"
#include "stream_out.h"
#include "stream_in.h"

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
        LOG_FN_PARAMETERS("Return existing device:%p", *device);
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
        LOG_FN_PARAMETERS("Created device:%p", *device);

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
        LOG_FN_PARAMETERS("Decremented ref_counter:%d", ref_counter);
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

/* Define some strings used for setting of parameters.
 * 'restarting=true' is hardcoded inside onAudioServerDied() in AudioService.java */
#define DEVICE_AUDIOSERVER_RESTARTING "restarting"
#define STRING_TRUE "true"

int adev_set_parameters(struct audio_hw_device *dev, const char *kv_pairs)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    struct str_parms * parsed_pairs;
    char value[32];
    int i;

    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", dev, kv_pairs);

    pthread_mutex_lock(&xdev->lock);

    parsed_pairs = str_parms_create_str(kv_pairs);
    if (parsed_pairs == NULL) {
        pthread_mutex_unlock(&xdev->lock);
        return 0;
    }
    if (str_parms_get_str(parsed_pairs, DEVICE_AUDIOSERVER_RESTARTING, value, sizeof(value)) >= 0) {
        str_parms_del(parsed_pairs, DEVICE_AUDIOSERVER_RESTARTING);
        if (strncmp(value, STRING_TRUE, sizeof(STRING_TRUE)) == 0) {
            /* audioserver is restarting, so close all streams.
             * See onAudioServerDied() in AudioService.java */
            for (i = 0; i < NUMBER_OF_DEVICES_OUT; i++) {
                if (xdev->xout_streams[i] != NULL) {
                    out_destroy(xdev->xout_streams[i]);
                    xdev->xout_streams[i] = NULL;
                }
            }
            for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
                if (xdev->xin_streams[i] != NULL) {
                    in_destroy(xdev->xin_streams[i]);
                    xdev->xin_streams[i] = NULL;
                }
            }
        }
    }

    str_parms_destroy(parsed_pairs);

    pthread_mutex_unlock(&xdev->lock);
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
    size_t buf_size;
    uint32_t channels;
    unsigned int sample_size;
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
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    int slot;

    LOG_FN_NAME_WITH_ARGS(
            "(%p, handle:%d, devices:%s(0x%x), flags:%s(0x%x), "
            "rate:%d, channel_mask:0x%x, format:0x%x, address:'%s')",
            dev, handle, xa_dbgstr_device(devices), devices, xa_dbgstr_output_flags(flags), flags,
            config->sample_rate, config->channel_mask, config->format,
            address);

    pthread_mutex_lock(&xdev->lock);

    /* handle case with default device without any config */
    if ((devices & AUDIO_DEVICE_OUT_DEFAULT) == AUDIO_DEVICE_OUT_DEFAULT) {
        if (config->format == 0) {
            config->format = AUDIO_FORMAT_PCM_16_BIT;
            ALOGW("Format is not specified, set it to %d", config->format);
        }
        if (audio_channel_count_from_out_mask(config->channel_mask) == 0) {
            config->channel_mask = audio_channel_out_mask_from_count(2);
            ALOGW("Channel mask is not specified, set it to %d", config->channel_mask);
        }
        if (config->sample_rate == 0) {
            config->sample_rate = 44100;
            ALOGW("Sample rate is not specified, set it to %d", config->sample_rate);
        }
    }

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

    slot = find_out_device(devices, address);
    if (slot >= 0) {
        if (xdev->xout_streams[slot] == NULL) {
            /* create new stream on free device */
            res = out_create(dev, handle, devices, slot, config, stream_out);
            if (*stream_out != NULL) {
                xdev->xout_streams[slot] = (x_stream_out_t*)(*stream_out);
                if (xdev->master_is_muted) {
                    out_set_mute(xdev->xout_streams[slot], true);
                }
                LOG_FN_PARAMETERS("Created stream_out:%p", *stream_out);
            }
        } else {
            res = -EEXIST;
            ALOGE("Output stream for this device already exists.");
        }
    } else {
        res = -EINVAL;
        ALOGE("Can't create output stream. Corresponding device was not found.");
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
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    unsigned int slot;

    pthread_mutex_lock(&xdev->lock);

    LOG_FN_NAME_WITH_ARGS("(%p, handle:0x%x, devices:%s(0x%x),",
            dev, handle, xa_dbgstr_device(devices), devices);
    LOG_FN_PARAMETERS("flags:%s(0x%x), address:'%s', source:%s(%d))",
            xa_dbgstr_input_flags(flags), flags, address, xa_dbgstr_source(source), source);
    LOG_FN_PARAMETERS(
            "Rate:%d, channel_mask:0x%x, format:0x%x, offload.size:%d, frame_count:%d ",
            config->sample_rate, config->channel_mask, config->format,
            config->offload_info.size, config->frame_count);

    /* handle case with default device without any config */
    if ((devices & AUDIO_DEVICE_IN_DEFAULT) == AUDIO_DEVICE_IN_DEFAULT) {
        if (config->format == 0) {
            config->format = AUDIO_FORMAT_PCM_16_BIT;
            ALOGW("Format is not specified, set it to %d", config->format);
        }
        if (audio_channel_count_from_out_mask(config->channel_mask) == 0) {
            config->channel_mask = audio_channel_in_mask_from_count(1);
            ALOGW("Channel mask is not specified, set it to %d", config->channel_mask);
        }
        if (config->sample_rate == 0) {
            config->sample_rate = 44100;
            ALOGW("Sample rate is not specified, set it to %d", config->sample_rate);
        }
    }

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

    slot = find_in_device(devices, address);
    if (slot >= 0) {
        if (xdev->xin_streams[slot] == NULL) {
            /* create new stream on free device */
            res = in_create(dev, handle, devices, slot, config, stream_in);
            if (*stream_in != NULL) {
                xdev->xin_streams[slot] = (x_stream_in_t*)(*stream_in);
                if (xdev->mic_is_muted) {
                    in_set_mute(xdev->xin_streams[slot], true);
                }
                LOG_FN_PARAMETERS("Created stream_in:%p", *stream_in);
            }
        } else {
            res = -EEXIST;
            ALOGE("Input stream for this device already exists.");
        }
    } else {
        res = -EINVAL;
        ALOGE("Can't create input stream. Corresponding device was not found.");
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
    x_audio_device_t *xdev = (x_audio_device_t*)dev;
    unsigned int i;

    if ((xdev == NULL) || (mic_array == NULL) || (mic_count == NULL)) {
        return -EINVAL;
    }

    pthread_mutex_lock(&xdev->lock);
    LOG_FN_NAME_WITH_ARGS("(%p)", dev);

    *mic_count = NUMBER_OF_DEVICES_IN;
    for (i = 0; i < NUMBER_OF_DEVICES_IN; i++) {
        memset(&(mic_array[i]), 0, sizeof(mic_array[0]));
        /*
        TODO: do we need to store device_id in xa_input_map ?
        mic_array[i].device_id[AUDIO_MICROPHONE_ID_MAX_LEN] = 0;
        mic_array[i].id = 0;
        */
        mic_array[i].device = xa_input_map[i].device_type_mask;
        strncpy(mic_array[i].address,
                AUDIO_BOTTOM_MICROPHONE_ADDRESS,
                AUDIO_DEVICE_MAX_ADDRESS_LEN - 1);
        memset(mic_array[i].channel_mapping,
               AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED,
               AUDIO_CHANNEL_COUNT_MAX);
        mic_array[i].location = AUDIO_MICROPHONE_LOCATION_MAINBODY;
        mic_array[i].group              = 0;
        mic_array[i].index_in_the_group = 0;
        mic_array[i].sensitivity    = AUDIO_MICROPHONE_SENSITIVITY_UNKNOWN;
        mic_array[i].max_spl        = AUDIO_MICROPHONE_SPL_UNKNOWN;
        mic_array[i].min_spl        = AUDIO_MICROPHONE_SPL_UNKNOWN;
        mic_array[i].directionality = AUDIO_MICROPHONE_DIRECTIONALITY_UNKNOWN;
        mic_array[i].num_frequency_responses = 0;
        /* num_frequency_responses is 0 so no need to set frequency_responses[] */
        mic_array[i].geometric_location.x = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        mic_array[i].geometric_location.y = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        mic_array[i].geometric_location.z = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        mic_array[i].orientation.x        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        mic_array[i].orientation.y        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
        mic_array[i].orientation.z        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    }

    pthread_mutex_unlock(&xdev->lock);
    return 0;
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

    LOG_FN_NAME_WITH_ARGS("(%p, id:%d, role:%s(%d), type:%s(%d), config_mask:%s(0x%x))",
            dev,
            config->id,
            xa_dbgstr_port_role(config->role),
            config->role,
            xa_dbgstr_port_type(config->type),
            config->type,
            xa_dbgstr_port_config_mask(config->config_mask),
            config->config_mask);

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
        LOG_FN_PARAMETERS("format:%s(%d)", xa_dbgstr_format(config->format), config->format);
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

/* functions for internal purposes */

int adev_is_slot_free(struct audio_hw_device *dev, int slot)
{
    x_audio_device_t *xdev = (x_audio_device_t*)dev;

    if (xdev == NULL) {
        return 0;
    }
    return (xdev->xout_streams[slot] == NULL) ? 1 : 0;
}
