.RECIPEPREFIX := >
CROSS ?= 
CC := $(CROSS)gcc
LD := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
CFLAGS := -Iinclude -Wall -Wextra -O2 -ffreestanding -nostdlib -nostartfiles -g
LDFLAGS := -T linker.ld

BOOT_SRC := src/boot_start.S src/bootloader.c src/uart.c src/mailbox.c
BOOT_OBJ := $(BOOT_SRC:.S=.o)
BOOT_OBJ := $(BOOT_OBJ:.c=.o)

PAYLOAD_SRC := src/start.S src/main.c src/uart.c 
PAYLOAD_OBJ := $(PAYLOAD_SRC:.S=.o)
PAYLOAD_OBJ := $(PAYLOAD_OBJ:.c=.o)

all: bootloader.img kernel8.bin

bootloader.elf: $(BOOT_OBJ) linker.ld
>$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJ)

bootloader.img: bootloader.elf
>$(OBJCOPY) -O binary $< $@

kernel8.elf: $(PAYLOAD_OBJ) linker_kernel.ld
>$(LD) -T linker_kernel.ld -o $@ $(PAYLOAD_OBJ)

kernel8.bin: kernel8.elf
>$(OBJCOPY) -O binary $< $@

%.o: %.S
>$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
>$(CC) $(CFLAGS) -c $< -o $@

clean:
>rm -f src/*.o *.elf *.img *.bin

run:
>./scripts/run-qemu.sh

run-pty:
>./scripts/run-qemu-pty.sh
