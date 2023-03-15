#pragma once

#include <stdint.h>

typedef enum _waveform_cfg_param_t {
	PARAM_RECV,
	PARAM_AMPLITUDE,
	PARAM_PERIOD_MS,
	PARAM_DUTYCYCLE,
	PARAM_ENABLE,
} waveform_cfg_param_t;

typedef struct _waveform_cfg_t {
	waveform_cfg_param_t type;
	uint32_t value;
} waveform_cfg_t;
