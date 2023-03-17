/*
 * CE 426 - Real-Time Embedded Systems
 * Instructor: Dr. Tewolde
 * Author: Benjamin Hall
 * Final Project: Function Generator
 */

#pragma once

/** Initialize the UART handler for user IO. */
void uart_handler_init(void);
/** Thread to manage user IO using UART. */
void uart_handler_thread(void const *arg);
