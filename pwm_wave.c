#include "global.h"
#include "pwm_wave.h"
#include "utils.h"

/** Stores the state of the PWM waveform. */
typedef struct _pwm_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
	uint16_t dutyCycle_q0d10;
	uint8_t bRunning;

	// calculated values
	uint16_t onTimeMs;
} pwm_state_t;

static uint16_t curTimeMs = 0;

// mail queue of configuration parameters to apply
static osMailQDef(pwm_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_pwm_cfg_id;

// message queue to send cfg param values from a recv request
static osMessageQDef(pwm_cfg_recv_q, 0x8, uint32_t);
osMessageQId Q_pwm_cfg_recv_id;

// mutex to protect the shared state of the waveform
static osMutexDef(pwm_state_m);
osMutexId M_pwm_state;

/** Runs the PWM waveform assuming the use of a 1ms timer. */
static void pwm_run(void const *arg);
// 1ms timer for the waveform
static osTimerDef(pwm_run_timer, pwm_run);
static osTimerId TMR_pwm_run_timer;

void pwm_wave_init(void)
{
	// create the mailbox, message queue, and mutex
	Q_pwm_cfg_id = osMailCreate(osMailQ(pwm_cfg_q), NULL);
	Q_pwm_cfg_recv_id = osMessageCreate(osMessageQ(pwm_cfg_recv_q), NULL);
	M_pwm_state = osMutexCreate(osMutex(pwm_state_m));
}

/** Sends the requested config parameter to the message queue. */
static void send_cfg_param(pwm_state_t *state, waveform_cfg_param_t param)
{
	uint32_t value = 0;
	switch (param) {
		case PARAM_AMPLITUDE:
			value = AMPLITUDE_TO_USER(state->amplitude);
			break;
		case PARAM_PERIOD_MS:
			value = state->periodMs;
			break;
		case PARAM_DUTYCYCLE:
			value = DUTYCYCLE_TO_USER(state->dutyCycle_q0d10);
			break;
		case PARAM_ENABLE:
			value = state->bRunning;
			break;
		default:
			break;
	}
	osMessagePut(Q_pwm_cfg_recv_id, value, 0);
}

/** Applies the duty cycle to the waveform. */
static inline void apply_dc(pwm_state_t *state)
{
	state->onTimeMs = ((uint32_t)state->periodMs * state->dutyCycle_q0d10) >> 10;
}

void pwm_wave_thread(void const *arg)
{
	// initial state: 100% amplitude, 100ms period, 50% DC, disabled
	pwm_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.dutyCycle_q0d10 = SCALE_DUTYCYCLE(50),
		.bRunning = 0,
	};
	apply_dc(&state);

	// create the timer using state as the input param
	TMR_pwm_run_timer = osTimerCreate(osTimer(pwm_run_timer), osTimerPeriodic, &state);

	osEvent retval;
	while (1) {
		// wait for a new config param
		retval = osMailGet(Q_pwm_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			// lock access to the shared state
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
						// toggle enable if value is non-zero
						state.bRunning = !state.bRunning;
					} else {
						// otherwise disable output
						state.bRunning = 0;
					}

					if (state.bRunning) {
						// we are now enabled, start the timer
						osTimerStart(TMR_pwm_run_timer, 1);
					} else {
						// we are now disabled, stop the timer
						osTimerStop(TMR_pwm_run_timer);
						// set output to 0 and reset time
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
			osMutexRelease(M_pwm_state);

			osMailFree(Q_pwm_cfg_id, cfg);
		}
	}
}

static void pwm_run(void const *arg)
{
	// lock access to shared state
	osMutexWait(M_pwm_state, osWaitForever);
	{
		pwm_state_t const *state = arg;

		// if period has elapsed, wrap back around to 0
		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		if (curTimeMs == state->onTimeMs) {
			// onTime has elapsed, turn waveform off
			GPIO_Write(WAVEFORM_PORT, 0);
		} else if (curTimeMs == 0) {
			// start of a new period, turn waveform on
			GPIO_Write(WAVEFORM_PORT, state->amplitude);
		}
	}
	osMutexRelease(M_pwm_state);

	++curTimeMs;
}
