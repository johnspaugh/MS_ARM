#include <stdint.h>
#include "uart.h"
#include "mailbox.h"

#define LOAD_ADDR        0x00200000UL
#define RX_BUF_ADDR      0x01000000UL
#define HDR_MAGIC        0x4C424B52u  // RKBL little-endian bytes: 52 4B 42 4C
#define MAX_KERNEL_SIZE  (16u * 1024u * 1024u)

#define EI_NIDENT 16
#define PT_LOAD   1

typedef void (*kernel_entry_t)(void);

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} Elf64_Phdr;

static uint32_t uart_get_u32_le(void) {
  uint32_t v = 0;
  v |= (uint32_t)(uint8_t)uart_getc();
  v |= (uint32_t)(uint8_t)uart_getc() << 8;
  v |= (uint32_t)(uint8_t)uart_getc() << 16;
  v |= (uint32_t)(uint8_t)uart_getc() << 24;
  return v;
}

static uint32_t checksum32(const uint8_t *buf, uint32_t n) {
  uint32_t s = 0;
  for (uint32_t i = 0; i < n; ++i) s += buf[i];
  return s;
}

static void print_hex_label(const char *label, uint32_t val) {
  uart_puts(label);
  uart_puts("0x");
  uart_hex32(val);
  uart_puts("\n");
}

static void mem_copy(uint8_t *dst, const uint8_t *src, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) dst[i] = src[i];
}

static void mem_zero(uint8_t *dst, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) dst[i] = 0;
}

static int is_elf64_aarch64(const uint8_t *buf, uint32_t size) {
  if (size < sizeof(Elf64_Ehdr)) return 0;
  const Elf64_Ehdr *eh = (const Elf64_Ehdr *)(const void *)buf;
  return (eh->e_ident[0] == 0x7F && eh->e_ident[1] == 0x45 && eh->e_ident[2] == 0x4C && eh->e_ident[3] == 0x46 &&
          eh->e_ident[4] == 2 && eh->e_ident[5] == 1 && eh->e_machine == 183);
}

static int load_elf64_image(const uint8_t *img, uint32_t size, uint64_t *entry_out) {
  const Elf64_Ehdr *eh = (const Elf64_Ehdr *)(const void *)img;

  if (eh->e_phoff >= size) return -1;
  if (eh->e_phentsize != sizeof(Elf64_Phdr)) return -2;

  uint64_t ph_end = eh->e_phoff + (uint64_t)eh->e_phnum * eh->e_phentsize;
  if (ph_end > size) return -3;

  const Elf64_Phdr *ph = (const Elf64_Phdr *)(const void *)(img + eh->e_phoff);

  for (uint16_t i = 0; i < eh->e_phnum; ++i) {
    if (ph[i].p_type != PT_LOAD) continue;

    if (ph[i].p_filesz > ph[i].p_memsz) return -4;
    if (ph[i].p_offset + ph[i].p_filesz > size) return -5;

    uint8_t *dst = (uint8_t *)(uintptr_t)ph[i].p_paddr;
    const uint8_t *src = img + ph[i].p_offset;

    mem_copy(dst, src, (uint32_t)ph[i].p_filesz);
    if (ph[i].p_memsz > ph[i].p_filesz) {
      mem_zero(dst + ph[i].p_filesz, (uint32_t)(ph[i].p_memsz - ph[i].p_filesz));
    }
  }

  *entry_out = eh->e_entry;
  return 0;
}

void boot_main(void) {
  uart_init();
  uart_puts("\n=== UART Bootloader (Pi4/QEMU) ===\n");

  uint32_t board_rev = 0, vc_base = 0;
  if (mailbox_get_board_revision(&board_rev) == 0) {
    print_hex_label("Board revision: ", board_rev);
  } else {
    uart_puts("Board revision: mailbox query failed\n");
  }

  if (mailbox_get_vc_base(&vc_base) == 0) {
    print_hex_label("VC core base address: ", vc_base);
  } else {
    uart_puts("VC core base address: mailbox query failed\n");
  }

  uart_puts("\nSend header [magic,u32 size,u32 checksum] then image bytes...\n");

  for (;;) {
    uart_puts("Waiting for transfer...\n");

    uint32_t magic = uart_get_u32_le();
    uint32_t size = uart_get_u32_le();
    uint32_t expect_sum = uart_get_u32_le();

    if (magic != HDR_MAGIC) {
      uart_puts("ERR bad magic: "); uart_hex32(magic); uart_puts("\n");
      continue;
    }
    if (size == 0 || size > MAX_KERNEL_SIZE) {
      uart_puts("ERR invalid size: "); uart_hex32(size); uart_puts("\n");
      continue;
    }

    uint8_t *rx = (uint8_t *)(uintptr_t)RX_BUF_ADDR;
    for (uint32_t i = 0; i < size; ++i) rx[i] = (uint8_t)uart_getc();

    uint32_t got_sum = checksum32(rx, size);
    if (got_sum != expect_sum) {
      uart_puts("ERR checksum mismatch exp="); uart_hex32(expect_sum);
      uart_puts(" got="); uart_hex32(got_sum); uart_puts("\n");
      uart_putc(0x4E);
      continue;
    }

    uint64_t entry = LOAD_ADDR;
    if (is_elf64_aarch64(rx, size)) {
      uart_puts("ELF image detected, loading PT_LOAD segments...\n");
      int rc = load_elf64_image(rx, size, &entry);
      if (rc != 0) {
        uart_puts("ERR ELF load failed rc="); uart_hex32((uint32_t)rc); uart_puts("\n");
        uart_putc(0x4E);
        continue;
      }
    } else {
      uart_puts("RAW image detected, loading at 0x00200000\n");
      mem_copy((uint8_t *)(uintptr_t)LOAD_ADDR, rx, size);
      entry = LOAD_ADDR;
    }

    uart_putc(0x41);
    uart_puts("\nKernel received: "); uart_hex32(size); uart_puts(" bytes\n");
    uart_puts("Jumping to loaded entry at "); uart_hex32((uint32_t)entry); uart_puts("\n");

    kernel_entry_t k = (kernel_entry_t)(uintptr_t)entry;
    k();

    uart_puts("Returned from loaded kernel (unexpected), waiting again...\n");
  }
}

void irq_handler_c(void) {
  // Bootloader does not use interrupts in this lab.
}
