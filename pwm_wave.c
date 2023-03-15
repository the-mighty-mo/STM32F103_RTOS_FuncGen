#include "global.h"
#include "pwm_wave.h"
#include "utils.h"

typedef struct _pwm_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
	uint16_t dutyCycle_q0d10;

	// calculated values
	uint16_t onTimeMs;
} pwm_state_t;

static uint16_t curTimeMs = 0;

static osMailQDef(pwm_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_pwm_cfg_id;

static osMutexDef(pwm_state_m);
osMutexId M_pwm_state;

static void pwm_run(void const *arg);

static osTimerDef(pwm_run_timer, pwm_run);
static osTimerId TMR_pwm_run_timer;

void pwm_wave_init(void)
{
	Q_pwm_cfg_id = osMailCreate(osMailQ(pwm_cfg_q), NULL);
	M_pwm_state = osMutexCreate(osMutex(pwm_state_m));
}

static inline void apply_dc(pwm_state_t *state)
{
	state->onTimeMs = ((uint32_t)state->periodMs * state->dutyCycle_q0d10) >> 10;
}

void pwm_wave_thread(void const *arg)
{
	pwm_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.dutyCycle_q0d10 = SCALE_DUTYCYCLE(50),
	};
	apply_dc(&state);

	TMR_pwm_run_timer = osTimerCreate(osTimer(pwm_run_timer), osTimerPeriodic, &state);
	uint8_t bRunning = 0;

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_pwm_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			osMutexWait(M_pwm_state, osWaitForever);
			switch (cfg->type) {
				case PARAM_AMPLITUDE:
				{
					state.amplitude = SCALE_AMPLITUDE(cfg->value);
					break;
				}
				case PARAM_PERIOD_MS:
				{
					state.periodMs = cfg->value;
					apply_dc(&state);
					break;
				}
				case PARAM_DUTYCYCLE:
				{
					state.dutyCycle_q0d10 = SCALE_DUTYCYCLE(cfg->value);
					apply_dc(&state);
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
						osTimerStart(TMR_pwm_run_timer, 1);
					} else {
						osTimerStop(TMR_pwm_run_timer);
						GPIO_Write(WAVEFORM_PORT, 0);
						curTimeMs = 0;
					}
					break;
				}
				default:
					break;
			}
			osMutexRelease(M_pwm_state);

			osMailFree(Q_pwm_cfg_id, cfg);
		}
	}
}

static void pwm_run(void const *arg)
{
	osMutexWait(M_pwm_state, osWaitForever);
	{
		pwm_state_t const *state = arg;

		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		if (curTimeMs == state->onTimeMs) {
			GPIO_Write(WAVEFORM_PORT, 0);
		} else if (curTimeMs == 0) {
			GPIO_Write(WAVEFORM_PORT, state->amplitude);
		}
	}
	osMutexRelease(M_pwm_state);

	++curTimeMs;
}
