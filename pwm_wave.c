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

void pwm_wave_thread(void const *arg)
{
	uint32_t amplitude;
	uint32_t onTimeMs;
	uint32_t offTimeMs;
	uint32_t periodMs = 100;

	{
		uint16_t mV = 75;
		uint8_t dutyCycle = 75;

		if (mV > GPIO_MAX_V) mV = GPIO_MAX_V;
		if (dutyCycle > 100) dutyCycle = 100;

		uint16_t dutyCycle_q0d10 = SCALE_DUTYCYCLE(dutyCycle);

		amplitude = SCALE_AMPLITUDE(mV);
		onTimeMs = ((uint64_t)periodMs * dutyCycle_q0d10) >> 10;
		offTimeMs = periodMs - onTimeMs;
	}

	TMR_pwm_on_timer = osTimerCreate(osTimer(pwm_on_timer), osTimerPeriodic, (void *)amplitude);
	TMR_pwm_off_timer = osTimerCreate(osTimer(pwm_off_timer), osTimerPeriodic, NULL);
	TMR_pwm_delay_timer = osTimerCreate(osTimer(pwm_delay_timer), osTimerOnce, (void *)periodMs);

	osTimerStart(TMR_pwm_on_timer, periodMs);
	osTimerStart(TMR_pwm_delay_timer, onTimeMs);
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
