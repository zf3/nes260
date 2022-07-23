#ifndef MY_UART_H
#define MY_UART_H

#include "xuartps.h"
#include "xscugic.h"

/*
 * Initialize UART_1 driver and interrupts. Caller needs to setup XSCuGic interrupt
 * controller driver first.
 */
int uart_init(XScuGic *intr);

/*
 * Blocking function to receive a fixed number of bytes from UART.
 * Return: buffer containing the result, or NULL if error.
 */
u8 *uart_recv(int len);

/*
 * Printf through UART1.
 */
void uart_printf(char *fmt,...);

#define prt(fmt,...) uart_printf(fmt,##__VA_ARGS__)


/*
 * Our UART driver instance.
 */
extern XUartPs UartPs;

#endif
