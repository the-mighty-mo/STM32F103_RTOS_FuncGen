#include "global.h"
#include "sawtooth_wave.h"
#include "utils.h"

typedef struct _sawtooth_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;

	// calculated values
	uint16_t periodMaxAmpMs;
} sawtooth_state_t;

static osMailQDef(sawtooth_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_sawtooth_cfg_id;

static osMutexDef(sawtooth_state_m);
osMutexId M_sawtooth_state;

static void sawtooth_run(void const *arg);

static osTimerDef(sawtooth_run_timer, sawtooth_run);
static osTimerId TMR_sawtooth_run_timer;

void sawtooth_wave_init(void)
{
	Q_sawtooth_cfg_id = osMailCreate(osMailQ(sawtooth_cfg_q), NULL);
	M_sawtooth_state = osMutexCreate(osMutex(sawtooth_state_m));
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
	};
	apply_periodMaxAmp(&state);

	TMR_sawtooth_run_timer = osTimerCreate(osTimer(sawtooth_run_timer), osTimerPeriodic, &state);
	uint8_t bRunning = 0;

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
						bRunning = !bRunning;
					} else {
						bRunning = 0;
					}

					if (bRunning) {
						osTimerStart(TMR_sawtooth_run_timer, 1);
					} else {
						osTimerStop(TMR_sawtooth_run_timer);
						GPIO_Write(WAVEFORM_PORT, 0);
					}
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
	static uint16_t curTimeMs = 0;

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
