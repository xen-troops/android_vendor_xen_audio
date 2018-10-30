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

#define CARD_ID  0
#define DEVICE_ID_PLAYBACK  0
#define DEVICE_ID_RECORD  0

/* period size is number of frames per one hardware cycle */
#define HW_PERIOD_SIZE  2048
/* number of periods per one ring buffer */
#define HW_PERIODS_PER_BUFFER  4


/* default configuration used for initialization purposes */
extern const struct pcm_config xa_config_default;

/* return true if config is supported for input stream */
bool is_config_supported_in(const audio_config_t * config);

/* return true if config is supported for output stream */
bool is_config_supported_out(const audio_config_t * config);

#endif /* VENDOR_XEN_AUDIO_AUDIO_HW_CONFIG_H_ */
