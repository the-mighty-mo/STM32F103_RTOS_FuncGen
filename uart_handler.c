#include "global.h"
#include "uart_handler.h"
#include "uart.h"
#include "pwm_wave.h"
#include "sawtooth_wave.h"
#include "sine_wave.h"
#include "triangle_wave.h"
#include "utils.h"
#include "waveform_cfg.h"
#include <string.h>

/// UART PROCESSING VARIABLES AND PROTOS

static osMessageQDef(uart_q, 0x10, uint8_t);
static osMessageQId Q_uart_id;

static uint8_t ReadChar(void);
static size_t ReadLine(char *line, size_t line_cap);
static void SendText(char const *text);

static int32_t parse_u16_saturate(char *str);
static int32_t u16_to_str(uint16_t value, char *str, size_t cap);

/// Program state

typedef enum _program_state_t {
	SELECT_WAVE,
	CONFIG_WAVE,
	CONFIG_PARAM,
} program_state_t;

typedef enum _waveform_t {
	WAVE_NONE,
	WAVE_PWM,
	WAVE_SIN,
	WAVE_SAW,
	WAVE_TRI,
} waveform_t;

typedef enum _param_t {
	NO_PARAM,
	AMPLITUDE,
	PERIOD,
	DUTY_CYCLE,
	ENABLE_OUT,
} param_t;

void uart_handler_init(void)
{
	USART1_Init();

	//Configure USART interrupt so we can read user inputs using interrupt
	//Configure and enable USART1 interrupt
	NVIC->ICPR[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	//clear any previous pending interrupt flag
	NVIC->IP[USART1_IRQn] = 0x80;		//set priority to 0x80
	NVIC->ISER[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);	//set interrupt enable bit
	USART1->CR1 |= USART_CR1_RXNEIE; //enable USART receiver not empty interrupt

	Q_uart_id = osMessageCreate(osMessageQ(uart_q), NULL);
}

static program_state_t process_config_pwm(param_t *param)
{
	SendText("Waveform: PWM\n");

	char line[8] = {0};
	u16_to_str(100, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	u16_to_str(100, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	u16_to_str(50, line, sizeof(line));
	SendText("Duty Cycle: ");
	SendText(line);
	SendText("%\n");

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	SendText("[0] Enable Output\n");
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");
	SendText("[3] Change Duty Cycle\n");
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
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			pwm_wave_send_cfg(cfg);
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			return CONFIG_WAVE;
	}
	
	return CONFIG_PARAM;
}

static program_state_t process_config_saw(param_t *param)
{
	SendText("Waveform: Sawtooth\n");

	char line[8] = {0};
	u16_to_str(100, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	u16_to_str(100, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	SendText("[0] Enable Output\n");
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");
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
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			sawtooth_wave_send_cfg(cfg);
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			return CONFIG_WAVE;
	}
	
	return CONFIG_PARAM;
}

static program_state_t process_config_sin(param_t *param)
{
	SendText("Waveform: Sine\n");

	char line[8] = {0};
	waveform_cfg_t cfg;

	cfg.type = PARAM_AMPLITUDE;
	sine_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	cfg.type = PARAM_PERIOD_MS;
	sine_wave_recv_cfg(&cfg);
	u16_to_str(cfg.value, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	SendText("[0] Enable Output\n");
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");
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
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			sine_wave_send_cfg(cfg);
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			return CONFIG_WAVE;
	}
	
	return CONFIG_PARAM;
}

static program_state_t process_config_tri(param_t *param)
{
	SendText("Waveform: Triangle\n");

	char line[8] = {0};
	u16_to_str(100, line, sizeof(line));
	SendText("Amplitude: ");
	SendText(line);
	SendText("%\n");

	u16_to_str(100, line, sizeof(line));
	SendText("Period: ");
	SendText(line);
	SendText(" ms\n");

	SendChar('\n');

	SendText("[Esc] Switch waveform\n");
	SendText("[0] Enable Output\n");
	SendText("[1] Change Amplitude\n");
	SendText("[2] Change Period\n");
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
			*param = NO_PARAM;
			waveform_cfg_t const cfg = {
				.type = PARAM_ENABLE,
				.value = 0,
			};
			triangle_wave_send_cfg(cfg);
			return SELECT_WAVE;
		}
		default:
			*param = NO_PARAM;
			SendText("Invalid input\n");
			return CONFIG_WAVE;
	}
	
	return CONFIG_PARAM;
}

void uart_handler_thread(void const *arg)
{
	program_state_t state = SELECT_WAVE;
	waveform_t wave = WAVE_NONE;
	param_t param = NO_PARAM;

	#define LINE_CAP 16
	char line[LINE_CAP] = {0};
	size_t line_len = 0;

	SendText("WAVEFORM GENERATOR\n");

	while (1) {
		SendChar('\n');

		switch (state) {
			case SELECT_WAVE:
			{
				SendText("Select waveform type:\n");
				SendText("[1] PWM\n");
				SendText("[2] Triangle\n");
				SendText("[3] Sawtooth\n");
				SendText("[4] Sine\n");
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
						if (value >= 0 && value <= 100) {
							SendText("Invalid input\n");
							cfg.type = PARAM_AMPLITUDE;
							cfg.value = value;
							bValid = 1;
						}

						memset(line, 0, sizeof(line));
						line_len = 0;
						break;
					}
					case PERIOD:
					{
						SendText("Period [0-60000 ms]: ");
						line_len = ReadLine(line, LINE_CAP);

						int32_t const value = parse_u16_saturate(line);
						if (value >= 0 && value <= 60000) {
							cfg.type = PARAM_PERIOD_MS;
							cfg.value = value;
							bValid = 1;
						}

						memset(line, 0, sizeof(line));
						line_len = 0;
						break;
					}
					case DUTY_CYCLE:
					{
						SendText("Duty Cycle [0-100%]: ");
						line_len = ReadLine(line, LINE_CAP);

						int32_t const value = parse_u16_saturate(line);
						if (value >= 0 && value <= 100) {
							cfg.type = PARAM_DUTYCYCLE;
							cfg.value = value;
							bValid = 1;
						}

						memset(line, 0, sizeof(line));
						line_len = 0;
						break;
					}
					case ENABLE_OUT:
					{
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
					state = CONFIG_WAVE;
				} else {
					SendText("Invalid input\n");
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
	osEvent result = osMessageGet(Q_uart_id, osWaitForever);
	uint8_t input = result.value.v;
	if (input == '\b') {
		SendText("\b \b");
	} else if (input == '\r') {
		input = '\n';
		SendChar(input);
	} else if (input != '\e') {
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
		result = osMessageGet(Q_uart_id, osWaitForever);
		input = result.value.v;

		if (input == '\b') {
			/* if we have characters in the line, remove the last char */
			if (line_len > 0) {
				line[--line_len] = '\0';
				SendText("\b \b");
			}
		} else if (input == '\r') {
			if (line_len > 0) {
				/* UART sends CR, we want to send LF */
				SendChar('\n');
				/* return line length */
				return line_len;
			}
		} else if (input != '\e' && line_len < line_cap - 1) {
			/* add input to the end of the line */
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
			/* invalid input */
			return -1;
		}
		retval *= 10;
		retval += *str - '0';
		if (retval > 0xFFFF) {
			return 0xFFFF;
		}
	}
	return retval;
}

static int32_t u16_to_str(uint16_t value, char *str, size_t cap)
{
	int32_t str_len = 0;

	uint16_t value_tmp = value;
	while (value_tmp) {
		value_tmp /= 10;
		++str_len;
	}

	if (str_len >= cap) {
		str[0] = '\0';
		return -1;
	}

	for (int i = str_len - 1; i >= 0; --i) {
		str[i] = (value % 10) + '0';
		value /= 10;
	}
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