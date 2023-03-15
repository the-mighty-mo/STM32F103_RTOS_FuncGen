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
