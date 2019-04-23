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

#ifndef VENDOR_XEN_AUDIO_DEVICE_H_
#define VENDOR_XEN_AUDIO_DEVICE_H_

#include <hardware/audio.h>

/* functions required by audio_hw_device API */

int adev_open(const hw_module_t* module, const char* name, hw_device_t** device);
int adev_close(hw_device_t *device);
uint32_t adev_get_supported_devices(const struct audio_hw_device *dev);
int adev_init_check(const struct audio_hw_device *dev);
int adev_set_voice_volume(struct audio_hw_device *dev, float volume);
int adev_set_master_volume(struct audio_hw_device *dev, float volume);
int adev_get_master_volume(struct audio_hw_device *dev, float *volume);
int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode);
int adev_set_mic_mute(struct audio_hw_device *dev, bool state);
int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state);
int adev_set_parameters(struct audio_hw_device *dev, const char *kv_pairs);
char * adev_get_parameters(const struct audio_hw_device *dev, const char *keys);
size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                const struct audio_config *config);
int adev_open_output_stream(struct audio_hw_device *dev,
                          audio_io_handle_t handle,
                          audio_devices_t devices,
                          audio_output_flags_t flags,
                          struct audio_config *config,
                          struct audio_stream_out **stream_out,
                          const char *address);
void adev_close_output_stream(struct audio_hw_device *dev, struct audio_stream_out* stream_out);
int adev_open_input_stream(struct audio_hw_device *dev,
                         audio_io_handle_t handle,
                         audio_devices_t devices,
                         struct audio_config *config,
                         struct audio_stream_in **stream_in,
                         audio_input_flags_t flags,
                         const char *address,
                         audio_source_t source);
void adev_close_input_stream(struct audio_hw_device *dev, struct audio_stream_in *stream_in);
int adev_get_microphones(const struct audio_hw_device *dev,
                       struct audio_microphone_characteristic_t *mic_array,
                       size_t *mic_count);
int adev_dump(const struct audio_hw_device *dev, int fd);
int adev_set_master_mute(struct audio_hw_device *dev, bool mute);
int adev_get_master_mute(struct audio_hw_device *dev, bool *mute);
int adev_create_audio_patch(struct audio_hw_device *dev,
                           unsigned int num_sources,
                           const struct audio_port_config *sources,
                           unsigned int num_sinks,
                           const struct audio_port_config *sinks,
                           audio_patch_handle_t *handle);
int adev_release_audio_patch(struct audio_hw_device *dev, audio_patch_handle_t handle);
int adev_get_audio_port(struct audio_hw_device *dev, struct audio_port *port);
int adev_set_audio_port_config(struct audio_hw_device *dev, const struct audio_port_config *config);

/* functions for internal purposes */

int adev_is_slot_free(struct audio_hw_device *adev, int slot);

#endif /* VENDOR_XEN_AUDIO_DEVICE_H_ */
