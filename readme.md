# android_vendor_xen_audio
Android's Audio HAL module for Xen virtual machine

## Summary

Features:
- implementation of Android's Audio HAL 4.0
- automotive related extensions (audio buses)

Sources are divided as folowing modules: audio device, input stream, output stream, config and debug.

## Requirements

Correct work of audio depends on corect settings in following files 'external' to audio HAL:
doma.cfg in meta-xt-prod-devel project with minimum number of audion PCM devices in `[vsnd]` section:
- 8 output devices
- 1 input device

audio_policy_configuration.xml in android_device_xenvm project that corresponds to `[vsnd]` section of doma.cfg

## Configuration

Audio configuration need to be in sync with audio_policy_configuration.xml.
Things that need to be checked and set:
- `xa_input_map[NUMBER_OF_DEVICES_IN]`
- `xa_output_map[NUMBER_OF_DEVICES_OUT]`

These maps need to have correctly set pcm card id and device id.
Period size and number of periods can be set independently for each audio device.

## Debug

Module has it's own debug macroses that enables functional traces.
All functions has traces with received parameters.
To enable these traces set ```LOG_FUNC_TRACES=1``` inside Android.bp.
Pay attention that some functoins may flood logs, so this is reason why traces are commented out for function ```out_get_presentation_position()```.
Also sometimes it's good idea to comment out traces in functions: ```out_write()```, ```in_read()```, ```out_get_latency()```.

LOG_TAG used by subsystems of module:
- xa_config - error messages about not supported configuration (audio_hw_config.c)
- xa_device - functions related to audio device (device.c)
- xa_in - functions related to input stream (stream_in.c)
- xa_out - functions related to output stream (stream_out.c)


Debug macroses used for tracing:
- `LOG_FN_PARAMETERS` - ALOGD wrapped by LOG_FUNC_TRACES switch
- `LOG_FN_NAME_WITH_ARGS` - ALOGD wrapped by LOG_FUNC_TRACES switch, with \_\_FUNCTION\_\_ prefix

There are some value-to-string decoders available inside dbg_func_traces.c to make debug easier.
Decoders are available for port role, port type, port config mask, input flags, output flags, audio format, audio source, audio device.

