#include "global.h"
#include "uart.h"
#include "utils.h"
#include <string.h>

static osMessageQDef(uart_q, 0x10, uint8_t);
static osMessageQId Q_uart_id;

static uint8_t ReadChar(void);
static size_t ReadLine(char *line, size_t line_cap);
static void SendText(char const *text);

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

void uart_handler_thread(void const *arg)
{
	#define LINE_CAP 16
	char line[LINE_CAP] = {0};
	size_t line_len = 0;
	
	while (1) {
		line_len = ReadLine(line, LINE_CAP);
		// process line

		memset(line, 0, sizeof(line));
	}
}

/// UART PROCESSING FUNCTIONS

static uint8_t ReadChar(void)
{
	osEvent result = osMessageGet(Q_uart_id, osWaitForever);
	uint8_t input = result.value.v;
	SendChar(input);
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
			/* UART sends CR, we want to send LF */
			SendChar('\n');
			/* return line length */
			return line_len;
		} else if (line_len < line_cap - 1) {
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

/*-----------------------------------------------------------------------------
	USART1 IRQ Handler
		The hardware automatically clears the interrupt flag, once the ISR is entered
 *---------------------------------------------------------------------------*/
void USART1_IRQHandler(void)
{
	uint8_t intKey = (int8_t)(USART1->DR & 0x1FF);
	osMessagePut(Q_uart_id, intKey, 0);
}