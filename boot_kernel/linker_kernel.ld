ENTRY(_start)
SECTIONS {
  . = 0x00200000;
  .text : { *(.text.boot) *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss (NOLOAD) : { *(.bss*) *(COMMON) }
  . = ALIGN(16);
  . += 0x4000;
  _stack_top = .;
}
