#ifndef UART_HELPERS_H
#define UART_HELPERS_H

// NRF APPS
#include "app_uart.h"   // app_uart_event_handler_t

// Sending and reception buffers sizes.
#define RX_FIFO_SIZE    256
#define TX_FIFO_SIZE    256

/* Initializes the UART channel with predefined parameters and an event
** callback function.
*/
void uart_init(const app_uart_event_handler_t handler);

#endif /* ! UART_HELPERS_H */
