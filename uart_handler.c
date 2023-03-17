/*
 * CE 426 - Real-Time Embedded Systems
 * Instructor: Dr. Tewolde
 * Author: Benjamin Hall
 * Final Project: Function Generator
 */

#include "uart_handler.h"
#include "global.h"
#include "uart.h"
#include "pwm_wave.h"
#include "sawtooth_wave.h"
#include "sine_wave.h"
#include "triangle_wave.h"
#include "utils.h"
#include "waveform_cfg.h"

/// UART PROCESSING VARIABLES AND PROTOS

// message queue of characters from the user
static osMessageQDef(uart_q, 0x10, uint8_t);
static osMessageQId Q_uart_id;

/** Reads a character from the user. */
static uint8_t ReadChar(void);
/**
 * Reads a line of input from the user into the provided buffer.
 * Returns the length of the string.
 */
static size_t ReadLine(char *line, size_t line_cap);
/** Sends text to the user. */
static void SendText(char const *text);

/** Parses a u16, saturating at u16 MAX (0xFFFF). */
static int32_t parse_u16_saturate(char *str);
/**
 * Converts a u16 to a string, filling the provided buffer.
 * Returns the length of the string, or -1 if there was an error.
 */
static int32_t u16_to_str(uint16_t value, char *str, size_t cap);

/// Program state

/** The state of the program. */
typedef enum _program_state_t {
	/** Menu to select a waveform */
	SELECT_WAVE,
	/** Menu to select a parameter to configure */
	CONFIG_WAVE,
	/** Menu to configure the parameter's value */
	CONFIG_PARAM,
} program_state_t;

/** The selected waveform. */
typedef enum _waveform_t {
	/** No waveform */
	WAVE_NONE,
	/** PWM waveform */
	WAVE_PWM,
	/** Sine waveform */
	WAVE_SIN,
	/** Sawtooth waveform */
	WAVE_SAW,
	/** Triangle waveform */
	WAVE_TRI,
} waveform_t;

/** The selected config parameter. */
typedef enum _param_t {
	NO_PARAM,
	AMPLITUDE,
	PERIOD,
	DUTY_CYCLE,
	ENABLE_OUT,
} param_t;

void uart_handler_init(void)
{
	// initialize USART1
	USART1_Init();

	// Configure USART1 interrupt so we can read user inputs using interrupt
	NVIC->ICPR[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	// clear any previous pending interrupt flag
	NVIC->IP[USART1_IRQn] = 0x80;		// set priority to 0x80
	NVIC->ISER[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	// set interrupt enable bit
	USART1->CR1 |= USART_CR1_RXNEIE; // enable USART receiver not empty interrupt

	// create the message queue
	Q_uart_id = osMessageCreate(osMessageQ(uart_q), NULL);
}

/**
 * Outputs and processes the PWM configuration menu.
 * The selected config parameter is filled into param.
 * Returns the next program state.
 */
static program_state_t process_config_pwm(param_t *param)
{
	SendText("Waveform: PWM\n");

	char line[8] = {0};
	waveform_cfg_t cfg;

	// fetch and output the amplitude of the waveform
	cfg.type = PARAM_AMPLITUDE;
	pwm_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	// fetch and output the period of the waveform
	cfg.type = PARAM_PERIOD_MS;
	pwm_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	// fetch and output the duty cycle of the waveform
	cfg.type = PARAM_DUTYCYCLE;
	pwm_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Duty Cycle: ");
	SendText(line);
	SendText("%\n");

	// fetch the enable state of the waveform
	cfg.type = PARAM_ENABLE;
	pwm_wave_recv_cfg(&cfg);
	uint8_t const bEnabled = cfg.value;

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	if (bEnabled) {
		SendText("[0] Disable Output\n");
	} else {
		SendText("[0] Enable Output\n");
	}
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");
	SendText("[3] Change Duty Cycle\n");

	// read the user selection
	SendText("Selection: ");
	char const selection = ReadChar();
	SendChar('\n');

	switch (selection) {
		case '1':
			*param = AMPLITUDE;
			break;
		case '2':
			*param = PERIOD;
			break;
		case '3':
			*param = DUTY_CYCLE;
			break;
		case '0':
			*param = ENABLE_OUT;
			break;
		case '\e':
		{
			// user wants to switch waveforms
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			pwm_wave_send_cfg(cfg);
			// go back to waveform selection screen
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			// rerun the waveform configuration screen
			return CONFIG_WAVE;
	}

	// go to the parameter configuration screen
	return CONFIG_PARAM;
}

/**
 * Outputs and processes the sawtooth configuration menu.
 * The selected config parameter is filled into param.
 * Returns the next program state.
 */
static program_state_t process_config_saw(param_t *param)
{
	SendText("Waveform: Sawtooth\n");

	char line[8] = {0};
	waveform_cfg_t cfg;

	// fetch and output the amplitude of the waveform
	cfg.type = PARAM_AMPLITUDE;
	sawtooth_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	// fetch and output the period of the waveform
	cfg.type = PARAM_PERIOD_MS;
	sawtooth_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	// fetch the enable state of the waveform
	cfg.type = PARAM_ENABLE;
	sawtooth_wave_recv_cfg(&cfg);
	uint8_t const bEnabled = cfg.value;

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	if (bEnabled) {
		SendText("[0] Disable Output\n");
	} else {
		SendText("[0] Enable Output\n");
	}
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");

	// read the user selection
	SendText("Selection: ");
	char const selection = ReadChar();
	SendChar('\n');

	switch (selection) {
		case '1':
			*param = AMPLITUDE;
			break;
		case '2':
			*param = PERIOD;
			break;
		case '0':
			*param = ENABLE_OUT;
			break;
		case '\e':
		{
			// user wants to switch waveforms
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			sawtooth_wave_send_cfg(cfg);
			// go back to waveform selection screen
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			// rerun the waveform configuration screen
			return CONFIG_WAVE;
	}

	// go to the parameter configuration screen
	return CONFIG_PARAM;
}

/**
 * Outputs and processes the sine configuration menu.
 * The selected config parameter is filled into param.
 * Returns the next program state.
 */
static program_state_t process_config_sin(param_t *param)
{
	SendText("Waveform: Sine\n");

	char line[8] = {0};
	waveform_cfg_t cfg;

	// fetch and output the amplitude of the waveform
	cfg.type = PARAM_AMPLITUDE;
	sine_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	// fetch and output the period of the waveform
	cfg.type = PARAM_PERIOD_MS;
	sine_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	// fetch the enable state of the waveform
	cfg.type = PARAM_ENABLE;
	sine_wave_recv_cfg(&cfg);
	uint8_t const bEnabled = cfg.value;

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	if (bEnabled) {
		SendText("[0] Disable Output\n");
	} else {
		SendText("[0] Enable Output\n");
	}
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");

	// read the user selection
	SendText("Selection: ");
	char const selection = ReadChar();
	SendChar('\n');

	switch (selection) {
		case '1':
			*param = AMPLITUDE;
			break;
		case '2':
			*param = PERIOD;
			break;
		case '0':
			*param = ENABLE_OUT;
			break;
		case '\e':
		{
			// user wants to switch waveforms
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			sine_wave_send_cfg(cfg);
			// go back to waveform selection screen
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			// rerun the waveform configuration screen
			return CONFIG_WAVE;
	}

	// go to the parameter configuration screen
	return CONFIG_PARAM;
}

/**
 * Outputs and processes the triangle configuration menu.
 * The selected config parameter is filled into param.
 * Returns the next program state.
 */
static program_state_t process_config_tri(param_t *param)
{
	SendText("Waveform: Triangle\n");

	char line[8] = {0};
	waveform_cfg_t cfg;

	// fetch and output the amplitude of the waveform
	cfg.type = PARAM_AMPLITUDE;
	triangle_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	// fetch and output the period of the waveform
	cfg.type = PARAM_PERIOD_MS;
	triangle_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	// fetch the enable state of the waveform
	cfg.type = PARAM_ENABLE;
	triangle_wave_recv_cfg(&cfg);
	uint8_t const bEnabled = cfg.value;

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	if (bEnabled) {
		SendText("[0] Disable Output\n");
	} else {
		SendText("[0] Enable Output\n");
	}
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");

	// read the user selection
	SendText("Selection: ");
	char const selection = ReadChar();
	SendChar('\n');

	switch (selection) {
		case '1':
			*param = AMPLITUDE;
			break;
		case '2':
			*param = PERIOD;
			break;
		case '0':
			*param = ENABLE_OUT;
			break;
		case '\e':
		{
			// user wants to switch waveforms
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			triangle_wave_send_cfg(cfg);
			// go back to waveform selection screen
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			// rerun the waveform configuration screen
			return CONFIG_WAVE;
	}

	// go to the parameter configuration screen
	return CONFIG_PARAM;
}

void uart_handler_thread(void const *arg)
{
	// start on the waveform selection screen
	program_state_t state = SELECT_WAVE;
	// initially no waveform is selected
	waveform_t wave = WAVE_NONE;
	// initially no param is selected
	param_t param = NO_PARAM;

	#define LINE_CAP 16
	// buffer for reading lines from user
	char line[LINE_CAP] = {0};
	size_t line_len = 0;

	SendText("WAVEFORM GENERATOR\n");

	while (1) {
		SendChar('\n');

		switch (state) {
			// waveform selection sreen
			case SELECT_WAVE:
			{
				SendText("Select waveform type:\n");
				SendText("[1] PWM\n");
				SendText("[2] Triangle\n");
				SendText("[3] Sawtooth\n");
				SendText("[4] Sine\n");

				// read user selection
				SendText("Selection: ");
				char const selection = ReadChar();
				SendChar('\n');

				switch (selection) {
					case '1':
						wave = WAVE_PWM;
						state = CONFIG_WAVE;
						break;
					case '2':
						wave = WAVE_TRI;
						state = CONFIG_WAVE;
						break;
					case '3':
						wave = WAVE_SAW;
						state = CONFIG_WAVE;
						break;
					case '4':
						wave = WAVE_SIN;
						state = CONFIG_WAVE;
						break;
					default:
						wave = WAVE_NONE;
						SendText("Invalid input\n");
						break;
				}
				break;
			}
			// waveform configuration screen
			case CONFIG_WAVE:
			{
				switch (wave) {
					case WAVE_PWM:
						state = process_config_pwm(&param);
						break;
					case WAVE_SAW:
						state = process_config_saw(&param);
						break;
					case WAVE_SIN:
						state = process_config_sin(&param);
						break;
					case WAVE_TRI:
						state = process_config_tri(&param);
						break;

					default:
						state = SELECT_WAVE;
						break;
				}

				break;
			}
			// parameter configuration screen
			case CONFIG_PARAM:
			{
				waveform_cfg_t cfg;
				uint8_t bValid = 0;

				switch (param) {
					case AMPLITUDE:
					{
						SendText("Amplitude [0-100%]: ");
						line_len = ReadLine(line, LINE_CAP);

						int32_t const value = parse_u16_saturate(line);
						// make sure the value is in the range [0, 100]
						if (value >= 0 && value <= 100) {
							// value is valid
							cfg.type = PARAM_AMPLITUDE;
							cfg.value = value;
							bValid = 1;
						}
						break;
					}
					case PERIOD:
					{
						SendText("Period [0-60000 ms]: ");
						line_len = ReadLine(line, LINE_CAP);

						int32_t const value = parse_u16_saturate(line);
						// make sure the value is in the range [0, 60000]
						if (value >= 0 && value <= 60000) {
							// value is valid
							cfg.type = PARAM_PERIOD_MS;
							cfg.value = value;
							bValid = 1;
						}
						break;
					}
					case DUTY_CYCLE:
					{
						SendText("Duty Cycle [0-100%]: ");
						line_len = ReadLine(line, LINE_CAP);

						int32_t const value = parse_u16_saturate(line);
						// make sure the value is in the range [0, 100]
						if (value >= 0 && value <= 100) {
							// value is valid
							cfg.type = PARAM_DUTYCYCLE;
							cfg.value = value;
							bValid = 1;
						}
						break;
					}
					case ENABLE_OUT:
					{
						// toggle enable (value = 1)
						cfg.type = PARAM_ENABLE;
						cfg.value = 1;
						bValid = 1;
						break;
					}

					default:
						break;
				}
				
				if (bValid) {
					// send config param
					switch (wave) {
						case WAVE_PWM:
							pwm_wave_send_cfg(cfg);
							break;
						case WAVE_SAW:
							sawtooth_wave_send_cfg(cfg);
							break;
						case WAVE_SIN:
							sine_wave_send_cfg(cfg);
							break;
						case WAVE_TRI:
							triangle_wave_send_cfg(cfg);
							break;

						default:
							break;
					}
					// go back to wave configuration screen
					state = CONFIG_WAVE;
				} else {
					SendText("Invalid input\n");
					// request the parameter value again
					state = CONFIG_PARAM;
				}
				break;
			}
		}
	}
}

/// UART PROCESSING FUNCTIONS

static uint8_t ReadChar(void)
{
	// wait for a character in the message queue
	osEvent result = osMessageGet(Q_uart_id, osWaitForever);
	uint8_t input = result.value.v;
	if (input == '\b') {
		// backspace, clear the last character from the screen
		SendText("\b \b");
	} else if (input == '\r') {
		// return key, replace with newline
		input = '\n';
		SendChar(input);
	} else if (input != '\e') {
		// not Esc key, bounce it back to the terminal
		SendChar(input);
	}
	return input;
}

static size_t ReadLine(char *line, size_t line_cap)
{
	osEvent result;
	uint8_t input;
	
	size_t line_len = 0;

	while (1) {
		// wait for a character in the message queue
		result = osMessageGet(Q_uart_id, osWaitForever);
		input = result.value.v;

		if (input == '\b') {
			// backspace; if we have characters in the line, remove the last char
			if (line_len > 0) {
				line[--line_len] = '\0';
				SendText("\b \b");
			}
		} else if (input == '\r') {
			// return key
			if (line_len > 0) {
				// UART sends CR, we want to send LF
				SendChar('\n');
				// ensure null termination
				line[line_len] = '\0';
				// return line length
				return line_len;
			}
		} else if (input != '\e' && line_len < line_cap - 1) {
			// not Esc key, and we haven't hit the capacity of the string
			// add input to the end of the line
			line[line_len++] = input;
			SendChar(input);
		}
	}
}

static void SendText(char const *text)
{
	while (*text) {
		SendChar(*text);
		++text;
	}
}

static int32_t parse_u16_saturate(char *str)
{
	int32_t retval = 0;
	for (; *str; ++str) {
		if (*str < '0' || *str > '9') {
			// invalid input
			return -1;
		}
		retval *= 10;
		retval += *str - '0';
		if (retval >= 0xFFFF) {
			// saturate at u16 max
			return 0xFFFF;
		}
	}
	return retval;
}

static int32_t u16_to_str(uint16_t value, char *str, size_t cap)
{
	int32_t str_len = 0;

	// calculate what the string length should be
	uint16_t value_tmp = value;
	while (value_tmp) {
		value_tmp /= 10;
		++str_len;
	}

	if (str_len >= cap) {
		// string buffer not large enough, return error
		str[0] = '\0';
		return -1;
	}

	// start at end of string (str_len - 1)
	for (int i = str_len - 1; i >= 0; --i) {
		// fill in ones place at end
		str[i] = (value % 10) + '0';
		// then divide by ten and move left one
		value /= 10;
	}
	// ensure null termination
	str[str_len] = '\0';
	return str_len;
}

/*-----------------------------------------------------------------------------
	USART1 IRQ Handler
		The hardware automatically clears the interrupt flag, once the ISR is entered
 *---------------------------------------------------------------------------*/
void USART1_IRQHandler(void)
{
	uint8_t const intKey = (int8_t)(USART1->DR & 0x1FF);
	osMessagePut(Q_uart_id, intKey, 0);
}