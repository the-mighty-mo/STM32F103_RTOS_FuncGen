#pragma once

#include "global.h"
#include "waveform_cfg.h"

void sine_wave_init(void);
void sine_wave_thread(void const *arg);

static inline osStatus sine_wave_send_cfg(waveform_cfg_t cfg)
{
	extern osMailQId Q_sine_cfg_id;
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sine_cfg_id, osWaitForever);
	*alloc_cfg = cfg;
	return osMailPut(Q_sine_cfg_id, alloc_cfg);
}

static inline osStatus sine_wave_recv_cfg(waveform_cfg_t *cfg)
{
	extern osMailQId Q_sine_cfg_id;
	extern osMessageQId Q_sine_cfg_recv_id;

	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sine_cfg_id, osWaitForever);
	alloc_cfg->type = PARAM_RECV;
	alloc_cfg->value = cfg->type;
	osMailPut(Q_sine_cfg_id, alloc_cfg);

	osEvent result = osMessageGet(Q_sine_cfg_recv_id, osWaitForever);
	if (result.status == osEventMessage) {
		cfg->value = result.value.v;
	}
	return result.status;
}
