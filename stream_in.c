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
 * This module implements functions for 'struct audio_stream_in',
 * so please see hardware/audio.h for description of function's
 * behavior and expected results.
 */

#define LOG_TAG "xa_in"

/* standard headers */
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
/* android headers */
#include <log/log.h>
#include <cutils/str_parms.h>
/* local headers*/
#include "audio_hw_config.h"
#include "dbg_func_traces.h"
#include "stream_in.h"


uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_in_t*)stream)->p_config.rate;
}

int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %d)", stream, rate);
    /* obsolete, not used function */
    return -ENOSYS;
}

size_t in_get_buffer_size(const struct audio_stream *stream)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;
    size_t bytes = 0;

    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    pthread_mutex_lock(&xin->lock);
    bytes = xin->p_config.period_size * xin->frame_size;
    LOG_FN_PARAMETERS("Calculated buffer_size:%zu", bytes);
    pthread_mutex_unlock(&xin->lock);
    return bytes;
}

audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_in_t*)stream)->a_channel_mask;
}

audio_format_t in_get_format(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_in_t*)stream)->a_format;
}

int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %08x)", stream, format);
    /* obsolete, not used function */
    return -ENOSYS;
}

int in_standby(struct audio_stream *stream)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;

    LOG_FN_NAME_WITH_ARGS("(%p)", stream);

    pthread_mutex_lock(&xin->lock);
    if (xin->standby) {
        LOG_FN_PARAMETERS("In standby already.");
    } else {
        xin->standby = true;
        /* close pcm device */
        if (xin->p_handle != NULL) {
            pcm_close(xin->p_handle);
            xin->p_handle = NULL;
        }
    }
    pthread_mutex_unlock(&xin->lock);
    return 0;
}

int in_dump(const struct audio_stream *stream, int fd)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;

    LOG_FN_NAME_WITH_ARGS("(%p, fd:%d)", stream, fd);

    pthread_mutex_lock(&xin->lock);

    dprintf(fd, "    audio_stream_in_t: %p\n", &xin->astream);
    dprintf(fd, "    lock: %p\n", &xin->lock);
    dprintf(fd, "    config.channels: %d\n", xin->p_config.channels);
    dprintf(fd, "      .rate: %d\n", xin->p_config.rate);
    dprintf(fd, "      .period_size: %d\n", xin->p_config.period_size);
    dprintf(fd, "      .period_count: %d\n", xin->p_config.period_count);
    dprintf(fd, "      .format: %d\n", xin->p_config.format);
    dprintf(fd, "      .start_threshold: %d\n", xin->p_config.start_threshold);
    dprintf(fd, "      .stop_threshold: %d\n", xin->p_config.stop_threshold);
    dprintf(fd, "      .silence_threshold: %d\n", xin->p_config.silence_threshold);
    dprintf(fd, "      .silence_size: %d\n", xin->p_config.silence_size);
    dprintf(fd, "      .avail_min: %d\n", xin->p_config.avail_min);
    dprintf(fd, "    pcm handle: %p\n", xin->p_handle);
    dprintf(fd, "    audio_devices_t: %u\n", xin->a_dev);
    dprintf(fd, "    audio_channel_mask_t: %d\n", xin->a_channel_mask);
    dprintf(fd, "    audio_format_t: %d\n", xin->a_format);
    dprintf(fd, "    standby: %s\n", xin->standby ? "true" : "false");
    dprintf(fd, "    frame_size: %zu\n", xin->frame_size);
    dprintf(fd, "    muted: %s\n", xin->muted ? "true" : "false");
    dprintf(fd, "    read_frames: %" PRIu64 "\n", xin->read_frames);
    dprintf(fd, "    last_time: %" PRIu64 "\n", xin->last_time);

    pthread_mutex_unlock(&xin->lock);
    return 0;
}

audio_devices_t in_get_device(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_in_t*)stream)->a_dev;
}

int in_set_device(struct audio_stream *stream, audio_devices_t device)
{
    /* Function is outdated and not used.
       Re-routnig have to be handled in set_parameters() */
    LOG_FN_NAME_WITH_ARGS("(%p, %08x)", stream, device);
    return 0;
}

int in_set_parameters(struct audio_stream *stream, const char *kv_pairs)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;
    struct str_parms * parsed_pairs;
    char value[32];
    char *end_ptr; /* used to confirm that numeric string was obtained */
    long int temp;

    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", stream, kv_pairs);

    pthread_mutex_lock(&xin->lock);

    if (kv_pairs[0] == '\0') {
        /* it's OK to receive empty string */
        pthread_mutex_unlock(&xin->lock);
        return 0;
    }

    if (!xin->standby) {
        /* we do not change parameters during record,
         * so we return ENOSYS according to API */
        pthread_mutex_unlock(&xin->lock);
        return -ENOSYS;
    }

    parsed_pairs = str_parms_create_str(kv_pairs);
    if (parsed_pairs == NULL) {
        pthread_mutex_unlock(&xin->lock);
        return -EINVAL;
    }

    if (str_parms_get_str(parsed_pairs, AUDIO_PARAMETER_STREAM_SAMPLING_RATE, value, sizeof(value)) >= 0) {
        str_parms_del(parsed_pairs, AUDIO_PARAMETER_STREAM_SAMPLING_RATE);
        temp = strtol(value, &end_ptr, 10);
        if ((errno == ERANGE) || (*end_ptr != '\0') || ((int)temp != temp)) {
            str_parms_destroy(parsed_pairs);
            pthread_mutex_unlock(&xin->lock);
            return -EINVAL;
        }
        /* At this point stream is in standby mode, so we can change parameters,
           and they will be used during next re-open of stream by in_read() */
        xin->p_config.rate = temp;
    }

    str_parms_destroy(parsed_pairs);

    pthread_mutex_unlock(&xin->lock);
    return 0;
}

char * in_get_parameters(const struct audio_stream *stream, const char *keys)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;
    struct str_parms * request_keys;
    struct str_parms * response_pairs;
    char temp_str[256];
    bool have_response = false;
    char *result_str = NULL;

    pthread_mutex_lock(&xin->lock);
    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", stream, keys);

    request_keys = str_parms_create_str(keys);
    if (request_keys == NULL) {
        pthread_mutex_unlock(&xin->lock);
        return strdup("");
    }

    response_pairs = str_parms_create();
    if (response_pairs == NULL) {
        pthread_mutex_unlock(&xin->lock);
        return strdup("");
    }

    if (str_parms_has_key(request_keys, AUDIO_PARAMETER_STREAM_SUP_FORMATS)) {
        if (0 == str_parms_add_str(response_pairs,
                                   AUDIO_PARAMETER_STREAM_SUP_FORMATS,
                                   "AUDIO_FORMAT_PCM_16_BIT")) {
            /* we have no possibility to return error code,
               so just do not return incorrect string */
            have_response = true;
        }
    }

    if (str_parms_has_key(request_keys, AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES)) {
        temp_str[0] = 0;
        get_supported_in_rates_as_string((char*)&temp_str, sizeof(temp_str));
        LOG_FN_PARAMETERS("get_supported_in_rates_as_string %s", temp_str);
        if (0 == str_parms_add_str(response_pairs,
                                   AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES,
                                   temp_str)) {
            /* we have no possibility to return error code,
               so just do not return incorrect string */
            have_response = true;
        }
    }

    if (have_response) {
        result_str = str_parms_to_str(response_pairs);
    }

    str_parms_destroy(request_keys);
    str_parms_destroy(response_pairs);

    pthread_mutex_unlock(&xin->lock);

    return result_str;
}

int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %p)", stream, effect);
    /* TODO To implement */
    return -ENOSYS;
}

int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %p)", stream, effect);
    /* TODO To implement */
    return -ENOSYS;
}

int in_set_gain(struct audio_stream_in *stream, float gain)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %f)", stream, gain);
    /* not supported for now */
    return -ENOSYS;
}

ssize_t in_read(struct audio_stream_in *stream, void* buffer, size_t bytes)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;
    int pcm_res = 0;
    ssize_t ret_code = bytes;
    struct timespec time;

    LOG_FN_NAME_WITH_ARGS("(%p, buffer:%p, bytes:%zu)", stream, buffer, bytes);

    if ((stream == NULL) || (buffer == NULL)) {
        return -EINVAL;
    }
    if (bytes == 0) {
        return 0;  /* job done - zero bytes are read */
    }

    pthread_mutex_lock(&xin->lock);

    if (xin->standby) {
        /* turn device on */
        xin->p_handle = pcm_open(xin->p_card_id, xin->p_dev_id, PCM_IN, &(xin->p_config));
        if ((xin->p_handle == NULL) || (!pcm_is_ready(xin->p_handle))) {
            ALOGE("%s() failed (%d). Can't reopen stream on device.", __FUNCTION__, errno);
            if (xin->p_handle != NULL) {
                pcm_close(xin->p_handle);
            }
            pthread_mutex_unlock(&xin->lock);
            return -errno;
        }
        xin->standby = false;
    }
    pcm_res = pcm_read(xin->p_handle, buffer, bytes);

    if (pcm_res < 0) {
        /* depending on case pcm_read can return -errno or -1, so we have to get real errno */
        ALOGE("pcm_read() failed, errno:%d '%s'", -errno, pcm_get_error(xin->p_handle));
        ret_code = -errno;
    } else {
        if (xin->muted) {
            /* return clear buffer if mic is muted,
               but we have to wait for time required for record */
            memset(buffer, 0, bytes);
            ret_code = bytes;
        }
    }

    xin->read_frames += bytes / xin->frame_size;
    clock_gettime(CLOCK_MONOTONIC, &time);
    xin->last_time = time.tv_sec * 1000000000LL + time.tv_nsec;

    pthread_mutex_unlock(&xin->lock);

    return ret_code;
}

uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* is not supported for now */
    return 0;
}

int in_get_capture_position(const struct audio_stream_in *stream,
                            int64_t *frames, int64_t *time)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;

    LOG_FN_NAME_WITH_ARGS("(%p)", stream);

    if (stream == NULL) {
        return -EINVAL;
    }

    if (frames != NULL) {
        *frames = xin->read_frames;
    }
    if (time != NULL) {
        *time = xin->last_time;
    }

    return 0;
}

int in_start(const struct audio_stream_in* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* MMAP is not supported for now */
    return -ENOSYS;
}

int in_stop(const struct audio_stream_in* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* MMAP is not supported for now */
    return -ENOSYS;
}

int in_create_mmap_buffer(const struct audio_stream_in *stream,
                          int32_t min_size_frames,
                          struct audio_mmap_buffer_info *info)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* MMAP is not supported for now */
    return -ENOSYS;
}

int in_get_mmap_position(const struct audio_stream_in *stream,
                         struct audio_mmap_position *position)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* MMAP is not supported for now */
    return -ENOSYS;
}

int in_get_active_microphones(const struct audio_stream_in *stream,
                              struct audio_microphone_characteristic_t *mic_array,
                              size_t *mic_count)
{
    x_stream_in_t *xin = (x_stream_in_t*)stream;

    if ((xin == NULL) || (mic_array == NULL) || (mic_count == NULL)) {
        return -EINVAL;
    }

    pthread_mutex_lock(&xin->lock);
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);

    /* this functon is called only on active stream,
       so  we always have just one mic active */
    *mic_count = 1;
    memset(&(mic_array[0]), 0, sizeof(mic_array[0]));
    /*
    mic_array[0].device_id[AUDIO_MICROPHONE_ID_MAX_LEN] = 0;
    mic_array[0].id = 0;
    */
    mic_array[0].device = xin->a_dev;
    strncpy(mic_array[0].address,
            AUDIO_BOTTOM_MICROPHONE_ADDRESS,
            AUDIO_DEVICE_MAX_ADDRESS_LEN - 1);
    memset(mic_array[0].channel_mapping,
           AUDIO_MICROPHONE_CHANNEL_MAPPING_UNUSED,
           AUDIO_CHANNEL_COUNT_MAX);
    mic_array[0].location = AUDIO_MICROPHONE_LOCATION_MAINBODY;
    mic_array[0].group              = 0;
    mic_array[0].index_in_the_group = 0;
    mic_array[0].sensitivity    = AUDIO_MICROPHONE_SENSITIVITY_UNKNOWN;
    mic_array[0].max_spl        = AUDIO_MICROPHONE_SPL_UNKNOWN;
    mic_array[0].min_spl        = AUDIO_MICROPHONE_SPL_UNKNOWN;
    mic_array[0].directionality = AUDIO_MICROPHONE_DIRECTIONALITY_UNKNOWN;
    mic_array[0].num_frequency_responses = 0;
    /* num_frequency_responses is 0 so no need to set frequency_responses[] */
    mic_array[0].geometric_location.x = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    mic_array[0].geometric_location.y = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    mic_array[0].geometric_location.z = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    mic_array[0].orientation.x        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    mic_array[0].orientation.y        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;
    mic_array[0].orientation.z        = AUDIO_MICROPHONE_COORDINATE_UNKNOWN;

    pthread_mutex_unlock(&xin->lock);
    return 0;
}

void in_update_sink_metadata(struct audio_stream_in *stream,
                             const struct sink_metadata* sink_metadata)
{
    if (sink_metadata == NULL) {
        return;
    }
    LOG_FN_NAME_WITH_ARGS("(%p, tracks:%zu)", stream, sink_metadata->track_count);
    for (unsigned int i = 0; i < sink_metadata->track_count; i++) {
        LOG_FN_PARAMETERS("track[%d].source:%d, .gain:%f",
                i, sink_metadata->tracks[i].source, sink_metadata->tracks[i].gain);
    }
    /* For now we can't set gain, so we have nothing to do here */
}

int in_create(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        unsigned int slot,
        struct audio_config *config,
        struct audio_stream_in **stream_in)
{
    /* supposed to be called by device, so config is always supported
       allocate stream
       set stream parameters
       set function pointers
       connect to specified hardware */

    x_stream_in_t *xin = NULL;
    int error_code;

    xin = (x_stream_in_t*)calloc(1, sizeof(x_stream_in_t));
    if (xin == NULL) {
        ALOGE("%s failed. -ENOMEM", __FUNCTION__);
        *stream_in = NULL;
        return -ENOMEM;
    }

    /* setup stream structure */
    pthread_mutex_init(&xin->lock, NULL);
    xin->a_dev = devices;
    xin->a_channel_mask = config->channel_mask;
    xin->a_format = config->format;
    xin->p_card_id = xa_input_map[slot].pcm_card;
    xin->p_dev_id = xa_input_map[slot].pcm_device;
    /* following fields are cleared by calloc:
        xin->standby
        xin->muted
    */

    xin->p_config.channels = popcount(config->channel_mask);
    xin->p_config.rate = config->sample_rate;
    xin->p_config.period_size = xa_input_map[slot].period_size;
    xin->p_config.period_count = xa_input_map[slot].periods_per_buffer;
    xin->p_config.format = DEFAULT_PCM_FORMAT;

    xin->astream.common.get_sample_rate = in_get_sample_rate;
    xin->astream.common.set_sample_rate = in_set_sample_rate;
    xin->astream.common.get_buffer_size = in_get_buffer_size;
    xin->astream.common.get_channels = in_get_channels;
    xin->astream.common.get_format = in_get_format;
    xin->astream.common.set_format = in_set_format;
    xin->astream.common.standby = in_standby;
    xin->astream.common.dump = in_dump;
    xin->astream.common.get_device = in_get_device;
    xin->astream.common.set_device = in_set_device;
    xin->astream.common.set_parameters = in_set_parameters;
    xin->astream.common.get_parameters = in_get_parameters;
    xin->astream.common.add_audio_effect = in_add_audio_effect;
    xin->astream.common.remove_audio_effect = in_remove_audio_effect;

    xin->astream.set_gain = in_set_gain;
    xin->astream.read = in_read;
    xin->astream.get_input_frames_lost = in_get_input_frames_lost;
    xin->astream.get_capture_position = in_get_capture_position;
    xin->astream.start = in_start;
    xin->astream.stop = in_stop;
    xin->astream.create_mmap_buffer = in_create_mmap_buffer;
    xin->astream.get_mmap_position = in_get_mmap_position;
    xin->astream.get_active_microphones = in_get_active_microphones;
    xin->astream.update_sink_metadata = in_update_sink_metadata;

    /* this can be called only when 'common' fields are initialized
       frame_size changes rarely, so we can store it precalculated */
    xin->frame_size = audio_stream_in_frame_size(&xin->astream);
    LOG_FN_PARAMETERS("Calculated xin->frame_size:%zu", xin->frame_size);

    xin->p_handle = pcm_open(xin->p_card_id, xin->p_dev_id, PCM_IN, &(xin->p_config));
    if ((xin->p_handle == NULL) || (!pcm_is_ready(xin->p_handle))) {
        error_code = errno;  /* store errno to avoid possible overwrite */
        ALOGE("%s() failed (%d). Can't open stream on device.", __FUNCTION__, error_code);
        if (xin->p_handle != NULL) {
            ALOGE("pcm error:'%s'", pcm_get_error(xin->p_handle));
            pcm_close(xin->p_handle);
            xin->p_handle = NULL;
        }
        *stream_in = NULL;
        free(xin);
        return -error_code;
    }

    *stream_in = &(xin->astream);

    return 0;
}

void in_destroy(x_stream_in_t *xin)
{
    pthread_mutex_destroy(&xin->lock);
    if (xin->p_handle != NULL) {
        pcm_close(xin->p_handle);
    }
    free(xin);
}

void in_set_mute(x_stream_in_t *xin, bool mute)
{
    pthread_mutex_lock(&xin->lock);
    xin->muted = mute;
    pthread_mutex_unlock(&xin->lock);
}
