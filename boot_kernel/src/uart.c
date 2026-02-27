#include <stdint.h>
#include "uart.h"

#define UART0_BASE 0xFE201000UL
#define DR    (*(volatile uint32_t*)(UART0_BASE + 0x00))
#define FR    (*(volatile uint32_t*)(UART0_BASE + 0x18))
#define IBRD  (*(volatile uint32_t*)(UART0_BASE + 0x24))
#define FBRD  (*(volatile uint32_t*)(UART0_BASE + 0x28))
#define LCRH  (*(volatile uint32_t*)(UART0_BASE + 0x2C))
#define CR    (*(volatile uint32_t*)(UART0_BASE + 0x30))
#define IMSC  (*(volatile uint32_t*)(UART0_BASE + 0x38))
#define MIS   (*(volatile uint32_t*)(UART0_BASE + 0x40))
#define ICR   (*(volatile uint32_t*)(UART0_BASE + 0x44))

#define FR_RXFE (1u << 4)
#define FR_TXFF (1u << 5)

#define INT_RX  (1u << 4)
#define INT_RT  (1u << 6)

#define RXQ_SZ 128
static volatile unsigned rx_head = 0;
static volatile unsigned rx_tail = 0;
static volatile char rxq[RXQ_SZ];

static void rxq_push(char c){
  unsigned next = (rx_head + 1) & (RXQ_SZ - 1);
  if (next != rx_tail) {
    rxq[rx_head] = c;
    rx_head = next;
  }
}

int uart_getc_nonblock(void){
  if (rx_head == rx_tail) return -1;
  char c = rxq[rx_tail];
  rx_tail = (rx_tail + 1) & (RXQ_SZ - 1);
  return (unsigned char)c;
}

void uart_init(void){
  CR = 0;
  ICR = 0x7FF;
  IBRD = 26;
  FBRD = 3;
  LCRH = (1u<<4) | (3u<<5); // FIFO + 8N1
  IMSC = 0;
  CR = (1u<<0) | (1u<<8) | (1u<<9); // UARTEN|TXE|RXE
}

void uart_irq_enable_rx(void){
  ICR = INT_RX | INT_RT;
  IMSC |= (INT_RX | INT_RT);
}

void uart_irq_disable_rx(void){
  IMSC &= ~(INT_RX | INT_RT);
  ICR = INT_RX | INT_RT;
}

void uart_irq_handler(void){
  uint32_t mis = MIS;
  if (mis & (INT_RX | INT_RT)) {
    while ((FR & FR_RXFE) == 0) {
      rxq_push((char)(DR & 0xFF));
    }
    ICR = INT_RX | INT_RT;
  }
}

void uart_putc(char c){ while(FR & FR_TXFF){} DR=(uint32_t)c; }
char uart_getc(void){ while(FR & FR_RXFE){} return (char)(DR & 0xFF); }
void uart_puts(const char*s){ while(*s){ if(*s=='\n') uart_putc('\r'); uart_putc(*s++);} }
void uart_hex8(unsigned v){ const char*h="0123456789ABCDEF"; uart_putc(h[(v>>4)&0xF]); uart_putc(h[v&0xF]); }
void uart_hex32(unsigned v){ for(int i=28;i>=0;i-=4){ unsigned n=(v>>i)&0xF; uart_putc(n<10?('0'+n):('A'+n-10)); } }
