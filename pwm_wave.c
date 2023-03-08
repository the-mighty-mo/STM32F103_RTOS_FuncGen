#include "STM32F10x.h"
#include "pwm_wave.h"
#include "utils.h"

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

static inline void apply_dc(uint16_t dutyCycle_q0d10, uint32_t periodMs, uint32_t *onTimeMs, uint32_t *offTimeMs)
{
	*onTimeMs = ((uint64_t)periodMs * dutyCycle_q0d10) >> 10;
	*offTimeMs = periodMs - *onTimeMs;
}

void pwm_wave_thread(void const *arg)
{
	uint32_t amplitude = SCALE_AMPLITUDE(100);
	uint32_t periodMs = 100;
	uint16_t dutyCycle_q0d10 = SCALE_DUTYCYCLE(50);
	uint32_t onTimeMs;
	uint32_t offTimeMs;
	apply_dc(dutyCycle_q0d10, periodMs, &onTimeMs, &offTimeMs);

	TMR_pwm_on_timer = osTimerCreate(osTimer(pwm_on_timer), osTimerPeriodic, (void *)amplitude);
	TMR_pwm_off_timer = osTimerCreate(osTimer(pwm_off_timer), osTimerPeriodic, NULL);
	TMR_pwm_delay_timer = osTimerCreate(osTimer(pwm_delay_timer), osTimerOnce, (void *)periodMs);

	osTimerStart(TMR_pwm_on_timer, periodMs);
	osTimerStart(TMR_pwm_delay_timer, onTimeMs);

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_pwm_cfg_id, osWaitForever);
		waveform_cfg_t *cfg = retval.value.p;
		
		switch (cfg->type) {
			case PARAM_AMPLITUDE:
			{
				amplitude = SCALE_AMPLITUDE(cfg->value);
				break;
			}
			case PARAM_PERIOD_MS:
			{
				periodMs = cfg->value;
				apply_dc(dutyCycle_q0d10, periodMs, &onTimeMs, &offTimeMs);
				break;
			}
			case PARAM_DUTYCYCLE:
			{
				dutyCycle_q0d10 = SCALE_DUTYCYCLE(cfg->value);
				apply_dc(dutyCycle_q0d10, periodMs, &onTimeMs, &offTimeMs);
				break;
			}
		}
	}
}

static void pwm_on(void const *arg)
{
	uint32_t amplitude = (uint32_t)arg;
	GPIO_Write(GPIOA, amplitude);
}

static void pwm_off(void const *arg)
{
	GPIO_Write(GPIOA, 0x0000);
}

static void pwm_delay(void const *arg)
{
	uint32_t periodMs = (uint32_t)arg;
	osTimerStart(TMR_pwm_off_timer, periodMs);
}
