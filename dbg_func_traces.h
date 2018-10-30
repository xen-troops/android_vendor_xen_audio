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

#ifndef VENDOR_XEN_AUDIO_DBG_FUNC_TRACES_H_
#define VENDOR_XEN_AUDIO_DBG_FUNC_TRACES_H_

#include <log/log.h>

/* No need to check LOG_NDEBUG, because it will disable ALOGD inside logging subsystem */

#if LOG_FUNC_TRACES
#define LOG_FN_NAME()  ALOGD("%s", __FUNCTION__)
#define LOG_FN_PARAMETERS  ALOGD
#define LOG_FN_NAME_WITH_ARGS(format, ...)  ALOGD("%s" format, __FUNCTION__, __VA_ARGS__)
#else  /* LOG_FUNC_TRACES */
#define LOG_FN_NAME() (void)(0)
#define LOG_FN_PARAMETERS(...) (void)(0)
#define LOG_FN_NAME_WITH_ARGS(...) (void)(0)
#endif  /* LOG_FUNC_TRACES */


#endif /* VENDOR_XEN_AUDIO_DBG_FUNC_TRACES_H_ */
