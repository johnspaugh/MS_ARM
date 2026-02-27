#include <stdint.h>
#include "uart.h"

#define CMD_BUF_SZ 64

static int streq(const char *a, const char *b) {
  while (*a && *b) {
    if (*a != *b) return 0;
    a++; b++;
  }
  return (*a == 0 && *b == 0);
}

static void print_help(void) {
  uart_puts("Commands:\n");
  uart_puts("  help       - show this help\n");
  uart_puts("  ping       - reply with pong\n");
  uart_puts("  info       - show loaded kernel info\n");
  uart_puts("  reboot     - jump to loaded kernel entry again (self-loop)\n");
}

static void process_cmd(const char *cmd) {
  if (cmd[0] == 0) {
    return;
  } else if (streq(cmd, "help")) {
    print_help();
  } else if (streq(cmd, "ping")) {
    uart_puts("pong\n");
  } else if (streq(cmd, "info")) {
    uart_puts("loaded kernel @ 0x00200000, cmd buffer=64 bytes\n");
  } else if (streq(cmd, "reboot")) {
    uart_puts("Re-entering loaded kernel main loop...\n");
  } else {
    uart_puts("unknown command: ");
    uart_puts(cmd);
    uart_puts("\n");
  }
}

void main(void) {
  uart_init();
  uart_puts("*** LOADED KERNEL: command shell ready ***\n");
  uart_puts("Type help for commands.\n");

  char buf[CMD_BUF_SZ];
  unsigned n = 0;

  for (;;) {
    uart_puts("shell> ");
    n = 0;

    for (;;) {
      char c = uart_getc();

      if (c == 13 || c == 10) {
        uart_puts("\n");
        break;
      }

      if (c == 0x08 || c == 0x7f) {
        if (n > 0) {
          n--;
          uart_puts("\b \b");
        }
        continue;
      }

      if (c >= 32 && c <= 126) {
        if (n < CMD_BUF_SZ - 1) {
          buf[n++] = c;
          uart_putc(c);
        }
      }
    }

    buf[n] = 0;
    process_cmd(buf);
  }
}
