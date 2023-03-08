#include "STM32F10x.h"
#include "cmsis_os.h"
#include "utils.h"

static void pwm_on(void const *arg);
static void pwm_off(void const *arg);
static void pwm_delay(void const *arg);

osTimerDef(pwm_on_timer, pwm_on);
osTimerDef(pwm_off_timer, pwm_off);
osTimerDef(pwm_delay_timer, pwm_delay);
osTimerId TMR_pwm_on_timer;
osTimerId TMR_pwm_off_timer;
osTimerId TMR_pwm_delay_timer;

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

void pwm_thread(void const *arg)
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
