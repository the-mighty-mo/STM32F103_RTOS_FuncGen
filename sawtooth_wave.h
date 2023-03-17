/*
 * CE 426 - Real-Time Embedded Systems
 * Instructor: Dr. Tewolde
 * Author: Benjamin Hall
 * Final Project: Function Generator
 */

#pragma once

#include "global.h"
#include "waveform_cfg.h"

/** Initialize the sawtooth waveform. */
void sawtooth_wave_init(void);
/** Thread to manage a sawtooth waveform. */
void sawtooth_wave_thread(void const *arg);

/** Sends a configuration parameter to the sawtooth thread. */
static inline osStatus sawtooth_wave_send_cfg(waveform_cfg_t cfg)
{
	extern osMailQId Q_sawtooth_cfg_id;
	// alloc a waveform_cfg_t in the mailbox and copy the cfg passed in
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sawtooth_cfg_id, osWaitForever);
	*alloc_cfg = cfg;
	return osMailPut(Q_sawtooth_cfg_id, alloc_cfg);
}

/** Reads a configuration parameter from the sawtooth thread. */
static osStatus sawtooth_wave_recv_cfg(waveform_cfg_t *cfg)
{
	extern osMailQId Q_sawtooth_cfg_id;
	extern osMessageQId Q_sawtooth_cfg_recv_id;

	// alloc a waveform_cfg_t in the mailbox
	waveform_cfg_t *alloc_cfg = osMailAlloc(Q_sawtooth_cfg_id, osWaitForever);
	// type is a param recieve, value is the param type we're requesting
	alloc_cfg->type = PARAM_RECV;
	alloc_cfg->value = cfg->type;
	osMailPut(Q_sawtooth_cfg_id, alloc_cfg);

	// pull out the param value from the message queue
	osEvent result = osMessageGet(Q_sawtooth_cfg_recv_id, osWaitForever);
	if (result.status == osEventMessage) {
		cfg->value = result.value.v;
	}
	return result.status;
}
