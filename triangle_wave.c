#include "global.h"
#include "triangle_wave.h"
#include "utils.h"

typedef struct _triangle_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;

	// calculated values
	uint16_t halfPeriodMs;
} triangle_state_t;

static uint16_t curTimeMs = 0;

static osMailQDef(triangle_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_triangle_cfg_id;

static osMutexDef(triangle_state_m);
osMutexId M_triangle_state;

static void triangle_run(void const *arg);

static osTimerDef(triangle_run_timer, triangle_run);
static osTimerId TMR_triangle_run_timer;

void triangle_wave_init(void)
{
	Q_triangle_cfg_id = osMailCreate(osMailQ(triangle_cfg_q), NULL);
	M_triangle_state = osMutexCreate(osMutex(triangle_state_m));
}

static inline void apply_halfPeriod(triangle_state_t *state)
{
	state->halfPeriodMs = state->periodMs >> 1;
}

void triangle_wave_thread(void const *arg)
{
	triangle_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
	};
	apply_halfPeriod(&state);

	TMR_triangle_run_timer = osTimerCreate(osTimer(triangle_run_timer), osTimerPeriodic, &state);
	uint8_t bRunning = 0;

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_triangle_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			osMutexWait(M_triangle_state, osWaitForever);
			switch (cfg->type) {
				case PARAM_AMPLITUDE:
				{
					state.amplitude = SCALE_AMPLITUDE(cfg->value);
					break;
				}
				case PARAM_PERIOD_MS:
				{
					state.periodMs = cfg->value;
					apply_halfPeriod(&state);
					break;
				}
				case PARAM_ENABLE:
				{
					if (cfg->value) {
						/* toggle enable if value is non-zero */
						bRunning = !bRunning;
					} else {
						/* otherwise disable output */
						bRunning = 0;
					}

					if (bRunning) {
						osTimerStart(TMR_triangle_run_timer, 1);
					} else {
						osTimerStop(TMR_triangle_run_timer);
						GPIO_Write(WAVEFORM_PORT, 0);
						curTimeMs = 0;
					}
					break;
				}
				default:
					break;
			}
			osMutexRelease(M_triangle_state);

			osMailFree(Q_triangle_cfg_id, cfg);
		}
	}
}

static void triangle_run(void const *arg)
{
	osMutexWait(M_triangle_state, osWaitForever);
	{
		triangle_state_t const *state = arg;

		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		if (curTimeMs <= state->halfPeriodMs) {
			GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * curTimeMs / state->halfPeriodMs);
		} else {
			GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * (state->periodMs - curTimeMs) / state->halfPeriodMs);
		}
	}
	osMutexRelease(M_triangle_state);

	++curTimeMs;
}
