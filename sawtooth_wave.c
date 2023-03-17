#include "global.h"
#include "sawtooth_wave.h"
#include "utils.h"

/** Stores the state of the sawtooth waveform. */
typedef struct _sawtooth_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
	uint8_t bRunning;

	// calculated values
	uint16_t periodMaxAmpMs;
} sawtooth_state_t;

static uint16_t curTimeMs = 0;

// mail queue of configuration parameters to apply
static osMailQDef(sawtooth_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_sawtooth_cfg_id;

// message queue to send cfg param values from a recv request
static osMessageQDef(sawtooth_cfg_recv_q, 0x8, uint32_t);
osMessageQId Q_sawtooth_cfg_recv_id;

// mutex to protect the shared state of the waveform
static osMutexDef(sawtooth_state_m);
osMutexId M_sawtooth_state;

/** Runs the sawtooth waveform assuming the use of a 1ms timer. */
static void sawtooth_run(void const *arg);
// 1ms timer for the waveform
static osTimerDef(sawtooth_run_timer, sawtooth_run);
static osTimerId TMR_sawtooth_run_timer;

void sawtooth_wave_init(void)
{
	// create the mailbox, message queue, and mutex
	Q_sawtooth_cfg_id = osMailCreate(osMailQ(sawtooth_cfg_q), NULL);
	Q_sawtooth_cfg_recv_id = osMessageCreate(osMessageQ(sawtooth_cfg_recv_q), NULL);
	M_sawtooth_state = osMutexCreate(osMutex(sawtooth_state_m));
}

/** Sends the requested config parameter to the message queue. */
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

/** Calculates and applies the 0-MAX period to the waveform. */
static inline void apply_periodMaxAmp(sawtooth_state_t *state)
{
	// since we want to reach max amplitude, (period - 1) should be max
	state->periodMaxAmpMs = state->periodMs - 1;
}

void sawtooth_wave_thread(void const *arg)
{
	// initial state: 100% amplitude, 100ms period, disabled
	sawtooth_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
		.bRunning = 0,
	};
	apply_periodMaxAmp(&state);

	// create the timer using state as the input param
	TMR_sawtooth_run_timer = osTimerCreate(osTimer(sawtooth_run_timer), osTimerPeriodic, &state);

	osEvent retval;
	while (1) {
		// wait for a new config param
		retval = osMailGet(Q_sawtooth_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			// lock access to the shared state
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
						// toggle enable if value is non-zero
						state.bRunning = !state.bRunning;
					} else {
						// otherwise disable output
						state.bRunning = 0;
					}

					if (state.bRunning) {
						// we are now enabled, start the timer
						osTimerStart(TMR_sawtooth_run_timer, 1);
					} else {
						// we are now disabled, stop the timer
						osTimerStop(TMR_sawtooth_run_timer);
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
			osMutexRelease(M_sawtooth_state);

			osMailFree(Q_sawtooth_cfg_id, cfg);
		}
	}
}

static void sawtooth_run(void const *arg)
{
	// lock access to shared state
	osMutexWait(M_sawtooth_state, osWaitForever);
	{
		sawtooth_state_t const *state = arg;

		// if period has elapsed, wrap back around to 0
		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		// linear increasing function, slope based on periodMaxAmpMs so we hit max at periodMs - 1, 0 at periodMs (= 0)
		GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * curTimeMs / state->periodMaxAmpMs);
	}
	osMutexRelease(M_sawtooth_state);

	++curTimeMs;
}
