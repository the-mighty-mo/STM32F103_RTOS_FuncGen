#include "global.h"
#include "sawtooth_wave.h"
#include "utils.h"

typedef struct _sawtooth_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
	uint8_t bRunning;

	// calculated values
	uint16_t periodMaxAmpMs;
} sawtooth_state_t;

static uint16_t curTimeMs = 0;

static osMailQDef(sawtooth_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_sawtooth_cfg_id;

static osMessageQDef(sawtooth_cfg_recv_q, 0x8, uint32_t);
osMessageQId Q_sawtooth_cfg_recv_id;

static osMutexDef(sawtooth_state_m);
osMutexId M_sawtooth_state;

static void sawtooth_run(void const *arg);

static osTimerDef(sawtooth_run_timer, sawtooth_run);
static osTimerId TMR_sawtooth_run_timer;

void sawtooth_wave_init(void)
{
	Q_sawtooth_cfg_id = osMailCreate(osMailQ(sawtooth_cfg_q), NULL);
	Q_sawtooth_cfg_recv_id = osMessageCreate(osMessageQ(sawtooth_cfg_recv_q), NULL);
	M_sawtooth_state = osMutexCreate(osMutex(sawtooth_state_m));
}

static void send_cfg_param(sawtooth_state_t *state, waveform_cfg_param_t param)
{
	uint32_t value = 0;
	switch (param) {
		case PARAM_AMPLITUDE:
			value = AMPLITUDE_TO_USER(state->amplitude);
			break;
		case PARAM_PERIOD_MS:
			value = state->periodMs;
			break;
		case PARAM_ENABLE:
			value = state->bRunning;
			break;
		default:
			break;
	}
	osMessagePut(Q_sawtooth_cfg_recv_id, value, 0);
}

static inline void apply_periodMaxAmp(sawtooth_state_t *state)
{
	state->periodMaxAmpMs = state->periodMs - 1;
}

void sawtooth_wave_thread(void const *arg)
{
	sawtooth_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.bRunning = 0,
	};
	apply_periodMaxAmp(&state);

	TMR_sawtooth_run_timer = osTimerCreate(osTimer(sawtooth_run_timer), osTimerPeriodic, &state);

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_sawtooth_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			osMutexWait(M_sawtooth_state, osWaitForever);
			switch (cfg->type) {
				case PARAM_AMPLITUDE:
				{
					state.amplitude = SCALE_AMPLITUDE(cfg->value);
					break;
				}
				case PARAM_PERIOD_MS:
				{
					state.periodMs = cfg->value;
					apply_periodMaxAmp(&state);
					break;
				}
				case PARAM_ENABLE:
				{
					if (cfg->value) {
						/* toggle enable if value is non-zero */
						state.bRunning = !state.bRunning;
					} else {
						/* otherwise disable output */
						state.bRunning = 0;
					}

					if (state.bRunning) {
						osTimerStart(TMR_sawtooth_run_timer, 1);
					} else {
						osTimerStop(TMR_sawtooth_run_timer);
						GPIO_Write(WAVEFORM_PORT, 0);
						curTimeMs = 0;
					}
					break;
				}
				case PARAM_RECV:
				{
					send_cfg_param(&state, cfg->value);
					break;
				}
				default:
					break;
			}
			osMutexRelease(M_sawtooth_state);

			osMailFree(Q_sawtooth_cfg_id, cfg);
		}
	}
}

static void sawtooth_run(void const *arg)
{
	osMutexWait(M_sawtooth_state, osWaitForever);
	{
		sawtooth_state_t const *state = arg;

		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * curTimeMs / state->periodMaxAmpMs);
	}
	osMutexRelease(M_sawtooth_state);

	++curTimeMs;
}
