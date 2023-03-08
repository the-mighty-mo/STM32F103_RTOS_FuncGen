#include "STM32F10x.h"
#include "cmsis_os.h"
#include "uart.h"
#include "utils.h"
#include <string.h>

static osMessageQDef(uart_q, 0x10, uint8_t);
static osMessageQId Q_uart_id;

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
	osEvent result;
	uint8_t input;

	#define LINE_CAP 16
	char line[LINE_CAP] = {0};
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

			/* process line */

			/* clear line */
			memset(line, 0, sizeof(line));
			line_len = 0;
		} else if (line_len < LINE_CAP - 1) {
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