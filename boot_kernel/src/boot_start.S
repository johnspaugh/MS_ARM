.section .text.boot
.global _start
.global vectors
.extern boot_main
.extern irq_handler_c

_start:
    ldr x0, =_stack_top
    mov sp, x0

    // If booted in EL2, drop to EL1h.
    mrs x1, CurrentEL
    cmp x1, #0x8
    b.ne 1f

    mov x2, #(1 << 31)      // HCR_EL2.RW = 1 (EL1 is AArch64)
    orr x2, x2, #(1 << 1)
    msr hcr_el2, x2

    // Allow EL1 access to generic timer/counter.
    mov x2, #3
    msr cnthctl_el2, x2
    mov x2, #0
    msr cntvoff_el2, x2

    msr sp_el1, x0

    mov x2, #0x3C5          // EL1h, DAIF masked on entry
    msr spsr_el2, x2
    adr x2, 1f
    msr elr_el2, x2
    eret

1:
    ldr x0, =vectors
    msr vbar_el1, x0
    isb

    bl boot_main
hang:
    wfe
    b hang

// AArch64 vector table: 16 entries, each on a 0x80-byte slot.
.balign 0x800
vectors:
    // Current EL with SP0
    b .
    .balign 0x80
    b .
    .balign 0x80
    b .
    .balign 0x80
    b .

    // Current EL with SPx (EL1h uses this)
    .balign 0x80
    b .              // sync
    .balign 0x80
    b irq_entry      // irq
    .balign 0x80
    b .              // fiq
    .balign 0x80
    b .              // serror

    // Lower EL using AArch64
    .balign 0x80
    b .
    .balign 0x80
    b irq_entry
    .balign 0x80
    b .
    .balign 0x80
    b .

    // Lower EL using AArch32
    .balign 0x80
    b .
    .balign 0x80
    b .
    .balign 0x80
    b .
    .balign 0x80
    b .

irq_entry:
    sub sp, sp, #16*10
    stp x0,  x1,  [sp, #16*0]
    stp x2,  x3,  [sp, #16*1]
    stp x4,  x5,  [sp, #16*2]
    stp x6,  x7,  [sp, #16*3]
    stp x8,  x9,  [sp, #16*4]
    stp x10, x11, [sp, #16*5]
    stp x12, x13, [sp, #16*6]
    stp x14, x15, [sp, #16*7]
    stp x16, x17, [sp, #16*8]
    stp x18, x30, [sp, #16*9]

    bl irq_handler_c

    ldp x0,  x1,  [sp, #16*0]
    ldp x2,  x3,  [sp, #16*1]
    ldp x4,  x5,  [sp, #16*2]
    ldp x6,  x7,  [sp, #16*3]
    ldp x8,  x9,  [sp, #16*4]
    ldp x10, x11, [sp, #16*5]
    ldp x12, x13, [sp, #16*6]
    ldp x14, x15, [sp, #16*7]
    ldp x16, x17, [sp, #16*8]
    ldp x18, x30, [sp, #16*9]
    add sp, sp, #16*10
    eret
