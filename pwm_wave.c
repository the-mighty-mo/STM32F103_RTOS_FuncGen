#include "STM32F10x.h"
#include "pwm_wave.h"
#include "utils.h"

typedef struct _pwm_state_t {
	uint32_t amplitude;
	uint32_t periodMs;
	uint16_t dutyCycle_q0d10;
	uint32_t onTimeMs;
	uint32_t offTimeMs;
} pwm_state_t;

static osMailQDef(pwm_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_pwm_cfg_id;

static void pwm_on(void const *arg);
static void pwm_off(void const *arg);
static void pwm_delay(void const *arg);

static osTimerDef(pwm_on_timer, pwm_on);
static osTimerDef(pwm_off_timer, pwm_off);
static osTimerDef(pwm_delay_timer, pwm_delay);
static osTimerId TMR_pwm_on_timer;
static osTimerId TMR_pwm_off_timer;
static osTimerId TMR_pwm_delay_timer;

void pwm_wave_init(void)
{
	Q_pwm_cfg_id = osMailCreate(osMailQ(pwm_cfg_q), NULL);
}

static inline void apply_dc(pwm_state_t *state)
{
	state->onTimeMs = ((uint64_t)state->periodMs * state->dutyCycle_q0d10) >> 10;
	state->offTimeMs = state->periodMs - state->onTimeMs;
}

void pwm_wave_thread(void const *arg)
{
	pwm_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.dutyCycle_q0d10 = SCALE_DUTYCYCLE(50),
	};
	apply_dc(&state);

	TMR_pwm_on_timer = osTimerCreate(osTimer(pwm_on_timer), osTimerPeriodic, &state);
	TMR_pwm_off_timer = osTimerCreate(osTimer(pwm_off_timer), osTimerPeriodic, &state);
	TMR_pwm_delay_timer = osTimerCreate(osTimer(pwm_delay_timer), osTimerOnce, &state);

	osTimerStart(TMR_pwm_on_timer, state.periodMs);
	osTimerStart(TMR_pwm_delay_timer, state.onTimeMs);

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_pwm_cfg_id, osWaitForever);
		waveform_cfg_t *cfg = retval.value.p;
		
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
		}
	}
}

static void pwm_on(void const *arg)
{
	pwm_state_t const *state = arg;
	GPIO_Write(GPIOA, state->amplitude);
}

static void pwm_off(void const *arg)
{
	(void)arg;
	GPIO_Write(GPIOA, 0x0000);
}

static void pwm_delay(void const *arg)
{
	pwm_state_t const *state = arg;
	osTimerStart(TMR_pwm_off_timer, state->periodMs);
}
