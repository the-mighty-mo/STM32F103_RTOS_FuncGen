#pragma once

#include "cmsis_os.h"
#include "waveform_cfg.h"

void pwm_wave_init(void);
void pwm_wave_thread(void const *arg);

static inline osStatus pwm_wave_send_cfg(waveform_cfg_t *cfg)
{
	extern osMailQId Q_pwm_cfg_id;
	return osMailPut(Q_pwm_cfg_id, cfg);
}
