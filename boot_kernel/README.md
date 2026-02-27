# Bootloader and Mini-Kernel (Raspberry Pi 4 on QEMU)

1. Initializes UART early
2. Queries mailbox property channel and prints:
   - Board revision
   - VideoCore (VC) core base address
3. Waits for a UART transfer (`magic + size + checksum + payload`)
4. Verifies checksum and jumps to loaded kernel image at `0x00200000`

---

## Build

```bash
make clean
make
```

Artifacts:
- `bootloader.img`            -> bootloader (load by QEMU firmware path)
- `kernel8.bin`     -> image to send over UART

---

## Run on QEMU (Pi 4)

### Option A: serial on stdio (quick bootloader output)

```bash
./scripts/run-qemu.sh
```

### Option B: serial PTY (recommended for UART transfer testing)

```bash
./scripts/run-qemu-pty.sh
```

QEMU prints a line like:

```text
char device redirected to /dev/pts/5 (label serial0)
```

---

## Send kernel image over UART

From another terminal, run:

```bash
python3 scripts/uart_send.py --pty /dev/pts/5 --image kernel_payload.bin
```

(Replace `/dev/pts/5` with your actual PTY.)

Protocol used by bootloader:
- Header: 12 bytes, little-endian
  - `u32 magic` (`0x4C424B52`)
  - `u32 size`
  - `u32 checksum` (sum of payload bytes mod 2^32)
- Followed by `size` raw bytes payload
- Bootloader replies with:
  - `A` on success (ACK), then jumps to payload
  - `N` on checksum error (NAK), waits again

## Use that PTY path for sender/terminal attachment.
```bash
sudo apt update
sudo apt install screen
```
```bash
screen /dev/pts/5 115200
```
---

## Demo procedure (end-to-end)

1. Build:
   ```bash
   make clean && make
   ```
2. Start QEMU PTY:
   ```bash
   ./scripts/run-qemu-pty.sh
   ```
3. Observe bootloader banner + mailbox info + waiting prompt.
4. Send payload:
   ```bash
   python3 scripts/uart_send.py --pty /dev/pts/X --image kernel8.bin
   ```
5. Verify serial output shows:
   - mailbox values printed
   - transfer accepted
   - `Jumping to loaded kernel ...`
   - payload message: `*** LOADED KERNEL: ... ***`

---

## Pi 3 -> Pi 4 adaptation notes

- MMIO peripheral base uses Pi 4 value `0xFE000000`.
- PL011 UART and mailbox base offsets are kept from BCM design docs.
- QEMU machine model is `-M raspi4b`.


## GDB debugging for kernel8
Run QEMU with GDB server enabled:
```bash
./scripts/run-qemu-gdb.sh
```
Then attach from another terminal:

```bash
gdb kernel8.elf
(gdb) target remote :1234
(gdb) b main
(gdb) c
```
