#pragma once

#include "global.h"
#include "waveform_cfg.h"

/** Initialize the sine waveform. */
void sine_wave_init(void);
/** Thread to manage a sine waveform. */
void sine_wave_thread(void const *arg);

/** Sends a configuration parameter to the sine thread. */
static inline osStatus sine_wave_send_cfg(waveform_cfg_t cfg)
{
	extern osMailQId Q_sine_cfg_id;
	// alloc a waveform_cfg_t in the mailbox and copy the cfg passed in
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sine_cfg_id, osWaitForever);
	*alloc_cfg = cfg;
	return osMailPut(Q_sine_cfg_id, alloc_cfg);
}

/** Reads a configuration parameter from the sine thread. */
static osStatus sine_wave_recv_cfg(waveform_cfg_t *cfg)
{
	extern osMailQId Q_sine_cfg_id;
	extern osMessageQId Q_sine_cfg_recv_id;

	// alloc a waveform_cfg_t in the mailbox
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sine_cfg_id, osWaitForever);
	// type is a param recieve, value is the param type we're requesting
	alloc_cfg->type = PARAM_RECV;
	alloc_cfg->value = cfg->type;
	osMailPut(Q_sine_cfg_id, alloc_cfg);

	// pull out the param value from the message queue
	osEvent result = osMessageGet(Q_sine_cfg_recv_id, osWaitForever);
	if (result.status == osEventMessage) {
		cfg->value = result.value.v;
	}
	return result.status;
}
