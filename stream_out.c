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
 * This module implements functions for 'struct audio_stream_out',
 * so please see hardware/audio.h for description of function's
 * behavior and expected results.
 */

#define LOG_TAG "xa_out"

/* standard headers */
#include <stdlib.h>
#include <inttypes.h>
/* android headers */
#include <log/log.h>
/* local headers*/
#include "audio_hw_config.h"
#include "dbg_func_traces.h"
#include "stream_out.h"


uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_out_t*)stream)->p_config.rate;
}

int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %d)", stream, rate);
    /* obsolete, not used function */
    return -ENOSYS;
}

size_t out_get_buffer_size(const struct audio_stream *stream)
{
    x_stream_out_t *xout = (x_stream_out_t*)stream;
    size_t res = 0;

    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    pthread_mutex_lock(&xout->lock);
    res = xout->p_config.period_size * xout->frame_size;
    pthread_mutex_unlock(&xout->lock);
    return res;
}

audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_out_t*)stream)->a_channel_mask;
}

audio_format_t out_get_format(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_out_t*)stream)->a_format;
}

int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %08x)", stream, format);
    /* obsolete, not used function */
    return -ENOSYS;
}

int out_standby(struct audio_stream *stream)
{
    x_stream_out_t *xout = (x_stream_out_t*)stream;

    LOG_FN_NAME_WITH_ARGS("(%p)", stream);

    pthread_mutex_lock(&xout->lock);
    if (xout->standby) {
        LOG_FN_PARAMETERS("In standby already.");
    } else {
        xout->standby = true;
        /* close pcm device */
        if (xout->p_handle != NULL) {
            pcm_close(xout->p_handle);
            xout->p_handle = NULL;
        }
    }
    pthread_mutex_unlock(&xout->lock);
    return 0;
}

int out_dump(const struct audio_stream *stream, int fd)
{
    x_stream_out_t *xout = (x_stream_out_t*)stream;

    LOG_FN_NAME_WITH_ARGS("(%p, fd:%d)", stream, fd);

    pthread_mutex_lock(&xout->lock);

    dprintf(fd, "    audio_stream_out_t: %p\n", &xout->astream);
    dprintf(fd, "    lock: %p\n", &xout->lock);
    dprintf(fd, "    config.channels: %d\n", xout->p_config.channels);
    dprintf(fd, "      .rate: %d\n", xout->p_config.rate);
    dprintf(fd, "      .period_size: %d\n", xout->p_config.period_size);
    dprintf(fd, "      .period_count: %d\n", xout->p_config.period_count);
    dprintf(fd, "      .format: %d\n", xout->p_config.format);
    dprintf(fd, "      .start_threshold: %d\n", xout->p_config.start_threshold);
    dprintf(fd, "      .stop_threshold: %d\n", xout->p_config.stop_threshold);
    dprintf(fd, "      .silence_threshold: %d\n", xout->p_config.silence_threshold);
    dprintf(fd, "      .silence_size: %d\n", xout->p_config.silence_size);
    dprintf(fd, "      .avail_min: %d\n", xout->p_config.avail_min);
    dprintf(fd, "    pcm handle: %p\n", xout->p_handle);
    dprintf(fd, "    audio_devices_t: %u\n", xout->a_dev);
    dprintf(fd, "    audio_channel_mask_t: %d\n", xout->a_channel_mask);
    dprintf(fd, "    audio_format_t: %d\n", xout->a_format);
    dprintf(fd, "    standby: %s\n", xout->standby ? "true" : "false");
    dprintf(fd, "    written_frames: %" PRIu64 "\n", xout->written_frames);
    dprintf(fd, "    last_timestamp: %ld.%ld\n", xout->last_timestamp.tv_sec, xout->last_timestamp.tv_nsec);
    dprintf(fd, "    frame_size: %zu\n", xout->frame_size);
    dprintf(fd, "    buffer_latency: %u\n", xout->buffer_latency);
    dprintf(fd, "    muted: %s\n", xout->muted ? "true" : "false");

    pthread_mutex_unlock(&xout->lock);
    return 0;
}

audio_devices_t out_get_device(const struct audio_stream *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_out_t*)stream)->a_dev;
}

int out_set_device(struct audio_stream *stream, audio_devices_t device)
{
    /* Function is outdated and not used.
       Re-routing have to be handled in set_parameters() */
    LOG_FN_NAME_WITH_ARGS("(%p, %08x)", stream, device);
    return 0;
}

int out_set_parameters(struct audio_stream *stream, const char *kv_pairs)
{
    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", stream, kv_pairs);

    /* TODO To implement */

    /* TODO recalculate frame_size if format or channels are changed */
    /* TODO recalculate buffer latency if sample rate is changed */
    /* TODO pause and re-route if device is changed */

    return 0;
}

char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    LOG_FN_NAME_WITH_ARGS("(%p, '%s')", stream, keys);
    /* TODO To implement */
    return strdup("");
}

int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %p)", stream, effect);
    /* TODO To implement */
    return -ENOSYS;
}

int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    LOG_FN_NAME_WITH_ARGS("(%p, %p)", stream, effect);
    /* TODO To implement */
    return -ENOSYS;
}

uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    return ((x_stream_out_t*)stream)->buffer_latency;
}

int out_set_volume(struct audio_stream_out *stream, float left, float right)
{
    LOG_FN_NAME_WITH_ARGS("(%p, left:%f, right:%f)", stream, left, right);
    /* Not supported on current configuration. */
    return -ENOSYS;
}

ssize_t out_write(struct audio_stream_out *stream, const void* buffer, size_t bytes)
{
    /* check parameters
       check that we are initialized
       lock stream
       reopen if standby
       write to device
       handle errors */
    /* TODO handle non-blocking mode */
    int ret_pcm = 0;
    x_stream_out_t *xout = (x_stream_out_t*)stream;
    useconds_t sleep_us = 0;
    size_t frames = 0;

    LOG_FN_NAME_WITH_ARGS("(%p, buffer:%p, bytes:%zu)", stream, buffer, bytes);

    if ((stream == NULL) || (buffer == NULL)) {
        return -EINVAL;
    }
    if (bytes == 0) {
        return 0;  /* job done - zero bytes are written */
    }

    pthread_mutex_lock(&xout->lock);

    if (xout->standby) {
        /* turn device on */
        xout->p_handle = pcm_open(xout->p_card_id, xout->p_dev_id, PCM_OUT, &(xout->p_config));
        if ((xout->p_handle == NULL) || (!pcm_is_ready(xout->p_handle))) {
            ALOGE("%s() failed (%d). Can't reopen stream on device.", __FUNCTION__, errno);
            if (xout->p_handle != NULL) {
                pcm_close(xout->p_handle);
                xout->p_handle = NULL;
            }
            pthread_mutex_unlock(&xout->lock);
            return -errno;
        }
        xout->standby = false;
    }

    frames = bytes / xout->frame_size;
    if (xout->muted) {
        /* just sleep, and write nothing */
        sleep_us = frames * 1000000 / xout->p_config.rate;
        usleep(sleep_us);
    } else {
        ret_pcm = pcm_write(xout->p_handle, buffer, bytes);
        xout->written_frames += frames;
        clock_gettime(CLOCK_MONOTONIC, &xout->last_timestamp);
    }

    pthread_mutex_unlock(&xout->lock);

    if (ret_pcm != 0) {
        ALOGE("pcm_write() failed with %d '%s'", ret_pcm, pcm_get_error(xout->p_handle));
        /* sleep for buffer duration in us */
        sleep_us = frames * 1000000 / xout->p_config.rate;
        usleep(sleep_us);
        return ret_pcm;
    }

    return bytes;
}

int out_get_render_position(const struct audio_stream_out *stream, uint32_t *dsp_frames)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for offloaded (DSP) playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_get_next_write_timestamp(const struct audio_stream_out *stream, int64_t *timestamp)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Function is optional and it's usage is not clear */
    return -ENOSYS;
}

int out_set_callback(struct audio_stream_out *stream, stream_callback_t callback, void *cookie)
{
    LOG_FN_NAME_WITH_ARGS("(%p, callback:%p, cookie:%p)", stream, callback, cookie);
    /* TODO To implement */
    return -ENOSYS;
}

int out_pause(struct audio_stream_out* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for offloaded (DSP) playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_resume(struct audio_stream_out* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for offloaded (DSP) playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_drain(struct audio_stream_out* stream, audio_drain_type_t type )
{
    LOG_FN_NAME_WITH_ARGS("(%p, type:%d)", stream, type);
    /* Applicable only for offloaded (DSP) playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_flush(struct audio_stream_out* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for offloaded (DSP) playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_get_presentation_position(const struct audio_stream_out *stream,
                           uint64_t *frames, struct timespec *timestamp)
{
    x_stream_out_t *xout = (x_stream_out_t*)stream;

    /* this function is called too often during playback,
       it's better to not enable traces in it */
    /* LOG_FN_NAME_WITH_ARGS("(%p)", stream); */

    pthread_mutex_lock(&xout->lock);
    if (frames != NULL) {
        *frames = xout->written_frames;
    }
    if (timestamp != NULL) {
        memcpy(timestamp, &xout->last_timestamp, sizeof(struct timespec));
    }
    pthread_mutex_unlock(&xout->lock);

    return 0;
}

int out_start(const struct audio_stream_out* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for MMAP playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_stop(const struct audio_stream_out* stream)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for MMAP playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_create_mmap_buffer(const struct audio_stream_out *stream,
                          int32_t min_size_frames,
                          struct audio_mmap_buffer_info *info)
{
    LOG_FN_NAME_WITH_ARGS("(%p, min_size_frames:%d, info:%p)", stream, min_size_frames, info);
    /* Applicable only for MMAP playback. Not supported on current configuration. */
    return -ENOSYS;
}

int out_get_mmap_position(const struct audio_stream_out *stream,
                         struct audio_mmap_position *position)
{
    LOG_FN_NAME_WITH_ARGS("(%p)", stream);
    /* Applicable only for MMAP playback. Not supported on current configuration. */
    return -ENOSYS;
}

void out_update_source_metadata(struct audio_stream_out *stream,
                               const struct source_metadata* source_metadata)
{

    if (source_metadata == NULL) {
        LOG_FN_NAME_WITH_ARGS("(%p, NULL)", stream);
    } else {
        LOG_FN_NAME_WITH_ARGS("(%p, track_count:%zu)", stream, source_metadata->track_count);
        for (size_t i = 0; i < source_metadata->track_count; i++) {
            LOG_FN_PARAMETERS("track[%zu].usage:%d, .content_type:%d, .gain:%f",
                    i,
                    source_metadata->tracks[i].usage,
                    source_metadata->tracks[i].content_type,
                    source_metadata->tracks[i].gain);
        }
    }

    /* We do not handle these parameters for now */
}

int out_create(struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        unsigned int hw_card_id,
        unsigned int hw_device_id,
        struct audio_config *config,
        struct audio_stream_out **stream_out)
{
    /* supposed to be called by device, so config is always supported
       allocate stream
       set stream parameters
       set function pointers
       connect to hardware */

    x_stream_out_t *xout = NULL;

    xout = (x_stream_out_t*)calloc(1, sizeof(x_stream_out_t));
    if (xout == NULL) {
        ALOGE("%s failed. -ENOMEM", __FUNCTION__);
        *stream_out = NULL;
        return -ENOMEM;
    }

    /* setup stream structure */
    pthread_mutex_init(&xout->lock, NULL);
    xout->a_dev = devices;
    xout->a_channel_mask = config->channel_mask;
    xout->a_format = config->format;
    xout->p_card_id = hw_card_id;
    xout->p_dev_id = hw_device_id;
    /* following fields are cleared by calloc:
       xout->standby
       xout->written_frames
       xout->last_timestamp
       xout->muted
    */

    xout->p_config.channels = popcount(config->channel_mask);
    xout->p_config.rate = config->sample_rate;
    xout->p_config.period_size = xa_config_default.period_size;
    xout->p_config.period_count = xa_config_default.period_count;
    xout->p_config.format = xa_config_default.format;
    /* precalculate buffer related latency */
    xout->buffer_latency = (xout->p_config.period_size * xout->p_config.period_count * 1000) / xout->p_config.rate;
    LOG_FN_PARAMETERS("Calculated buffer_latency:%d", xout->buffer_latency);

    xout->astream.common.get_sample_rate = out_get_sample_rate;
    xout->astream.common.set_sample_rate = out_set_sample_rate;
    xout->astream.common.get_buffer_size = out_get_buffer_size;
    xout->astream.common.get_channels = out_get_channels;
    xout->astream.common.get_format = out_get_format;
    xout->astream.common.set_format = out_set_format;
    xout->astream.common.standby = out_standby;
    xout->astream.common.dump = out_dump;
    xout->astream.common.get_device = out_get_device;
    xout->astream.common.set_device = out_set_device;
    xout->astream.common.set_parameters = out_set_parameters;
    xout->astream.common.get_parameters = out_get_parameters;
    xout->astream.common.add_audio_effect = out_add_audio_effect;
    xout->astream.common.remove_audio_effect = out_remove_audio_effect;

    xout->astream.get_latency = out_get_latency;
    xout->astream.set_volume = out_set_volume;
    xout->astream.write = out_write;
    xout->astream.get_render_position = out_get_render_position;
    xout->astream.get_next_write_timestamp = out_get_next_write_timestamp;
    xout->astream.set_callback = out_set_callback;
    xout->astream.pause = out_pause;
    xout->astream.resume = out_resume;
    xout->astream.drain = out_drain;
    xout->astream.flush = out_flush;
    xout->astream.get_presentation_position = out_get_presentation_position;
    xout->astream.start = out_start;
    xout->astream.stop = out_stop;

    /* this can be called only when 'common' fields are initialized
       frame_size changes rarely, so we can store it precalculated */
    xout->frame_size = audio_stream_out_frame_size(&xout->astream);
    LOG_FN_PARAMETERS("Calculated xout->frame_size:%zu", xout->frame_size);

    /* connect to hardware */
    xout->p_handle = pcm_open(xout->p_card_id, xout->p_dev_id, PCM_OUT, &(xout->p_config));
    if ((xout->p_handle == NULL) || (!pcm_is_ready(xout->p_handle))) {
        ALOGE("%s() failed (%d). Can't open stream on device.", __FUNCTION__, errno);
        if (xout->p_handle != NULL) {
            pcm_close(xout->p_handle);
            xout->p_handle = NULL;
        }
        *stream_out = NULL;
        free(xout);
        return -ENOMEM;
    }

    *stream_out = &(xout->astream);

    return 0;
}

void out_destroy(x_stream_out_t *xout)
{
    pthread_mutex_destroy(&xout->lock);
    if (xout->p_handle != NULL) {
        pcm_close(xout->p_handle);
    }
    free(xout);
}

void out_set_mute(x_stream_out_t *xout, bool mute)
{
    pthread_mutex_lock(&xout->lock);
    xout->muted = mute;
    pthread_mutex_unlock(&xout->lock);
}
