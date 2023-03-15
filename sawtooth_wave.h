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
