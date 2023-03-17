#pragma once

/** Initialize the UART handler for user IO. */
void uart_handler_init(void);
/** Thread to manage user IO using UART. */
void uart_handler_thread(void const *arg);
