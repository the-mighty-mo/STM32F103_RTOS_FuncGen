#include "global.h"
#include "triangle_wave.h"
#include "utils.h"

/** Stores the state of the triangle waveform. */
typedef struct _triangle_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
	uint8_t bRunning;

	// calculated values
	uint16_t halfPeriodMs;
} triangle_state_t;

static uint16_t curTimeMs = 0;

// mail queue of configuration parameters to apply
static osMailQDef(triangle_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_triangle_cfg_id;

// message queue to send cfg param values from a recv request
static osMessageQDef(triangle_cfg_recv_q, 0x8, uint32_t);
osMessageQId Q_triangle_cfg_recv_id;

// mutex to protect the shared state of the waveform
static osMutexDef(triangle_state_m);
osMutexId M_triangle_state;

/** Runs the triangle waveform assuming the use of a 1ms timer. */
static void triangle_run(void const *arg);
// 1ms timer for the waveform
static osTimerDef(triangle_run_timer, triangle_run);
static osTimerId TMR_triangle_run_timer;

void triangle_wave_init(void)
{
	// create the mailbox, message queue, and mutex
	Q_triangle_cfg_id = osMailCreate(osMailQ(triangle_cfg_q), NULL);
	Q_triangle_cfg_recv_id = osMessageCreate(osMessageQ(triangle_cfg_recv_q), NULL);
	M_triangle_state = osMutexCreate(osMutex(triangle_state_m));
}

/** Sends the requested config parameter to the message queue. */
static void send_cfg_param(triangle_state_t *state, waveform_cfg_param_t param)
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
	osMessagePut(Q_triangle_cfg_recv_id, value, 0);
}

/** Calculates and applies the half-period to the waveform. */
static inline void apply_halfPeriod(triangle_state_t *state)
{
	state->halfPeriodMs = state->periodMs >> 1;
}

void triangle_wave_thread(void const *arg)
{
	// initial state: 100% amplitude, 100ms period, disabled
	triangle_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.bRunning = 0,
	};
	apply_halfPeriod(&state);

	// create the timer using state as the input param
	TMR_triangle_run_timer = osTimerCreate(osTimer(triangle_run_timer), osTimerPeriodic, &state);

	osEvent retval;
	while (1) {
		// wait for a new config param
		retval = osMailGet(Q_triangle_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			// lock access to the shared state
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
						// toggle enable if value is non-zero
						state.bRunning = !state.bRunning;
					} else {
						// otherwise disable output
						state.bRunning = 0;
					}

					if (state.bRunning) {
						// we are now enabled, start the timer
						osTimerStart(TMR_triangle_run_timer, 1);
					} else {
						// we are now disabled, stop the timer
						osTimerStop(TMR_triangle_run_timer);
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
			osMutexRelease(M_triangle_state);

			osMailFree(Q_triangle_cfg_id, cfg);
		}
	}
}

static void triangle_run(void const *arg)
{
	// lock access to shared state
	osMutexWait(M_triangle_state, osWaitForever);
	{
		triangle_state_t const *state = arg;

		// if period has elapsed, wrap back around to 0
		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		if (curTimeMs <= state->halfPeriodMs) {
			// first half period, linear increasing function from 0
			GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * curTimeMs / state->halfPeriodMs);
		} else {
			// second half period, linear decreasing function from periodMs
			GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * (state->periodMs - curTimeMs) / state->halfPeriodMs);
		}
	}
	osMutexRelease(M_triangle_state);

	++curTimeMs;
}
