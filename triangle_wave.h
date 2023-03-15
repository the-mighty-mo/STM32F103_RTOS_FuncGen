#pragma once

#include "global.h"
#include "waveform_cfg.h"

void triangle_wave_init(void);
void triangle_wave_thread(void const *arg);

static inline osStatus triangle_wave_send_cfg(waveform_cfg_t cfg)
{
	extern osMailQId Q_triangle_cfg_id;
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_triangle_cfg_id, osWaitForever);
	*alloc_cfg = cfg;
	return osMailPut(Q_triangle_cfg_id, alloc_cfg);
}
