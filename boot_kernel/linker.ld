ENTRY(_start)
SECTIONS {
  . = 0x80000;
  .text : { *(.text.boot) *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss (NOLOAD) : { *(.bss*) *(COMMON) }
  . = ALIGN(16);
  . += 0x10000;
  _stack_top = .;
}
