.section .text.boot
.global _start
.extern loaded_main
_start:
  ldr x0, =_stack_top
  mov sp, x0
  bl main
1: wfe
   b 1b
