#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);
void uart_hex8(unsigned v);
void uart_hex32(unsigned v);

// IRQ-driven RX support
void uart_irq_enable_rx(void);
void uart_irq_disable_rx(void);
void uart_irq_handler(void);
int  uart_getc_nonblock(void);   // -1 if no data

#endif
