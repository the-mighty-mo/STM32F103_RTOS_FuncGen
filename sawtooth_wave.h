#pragma once

#include "global.h"
#include "waveform_cfg.h"

void sawtooth_wave_init(void);
void sawtooth_wave_thread(void const *arg);

static inline osStatus sawtooth_wave_send_cfg(waveform_cfg_t cfg)
{
	extern osMailQId Q_sawtooth_cfg_id;
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sawtooth_cfg_id, osWaitForever);
	*alloc_cfg = cfg;
	return osMailPut(Q_sawtooth_cfg_id, alloc_cfg);
}

static osStatus sawtooth_wave_recv_cfg(waveform_cfg_t *cfg)
{
	extern osMailQId Q_sawtooth_cfg_id;
	extern osMessageQId Q_sawtooth_cfg_recv_id;

	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sawtooth_cfg_id, osWaitForever);
	alloc_cfg->type = PARAM_RECV;
	alloc_cfg->value = cfg->type;
	osMailPut(Q_sawtooth_cfg_id, alloc_cfg);

	osEvent result = osMessageGet(Q_sawtooth_cfg_recv_id, osWaitForever);
	if (result.status == osEventMessage) {
		cfg->value = result.value.v;
	}
	return result.status;
}
