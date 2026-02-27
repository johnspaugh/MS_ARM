#!/usr/bin/env python3
import argparse, os, struct, sys, time, select

MAGIC = 0x4C424B52  # must match bootloader


def checksum32(data: bytes) -> int:
    return sum(data) & 0xFFFFFFFF


def read_ack(fd: int, timeout: float) -> bytes:
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if not r:
            continue
        b = os.read(fd, 1)
        if not b:
            continue
        if b in (b"A", b"N"):
            return b
    return b""


def main():
    ap = argparse.ArgumentParser(description="Send kernel payload over UART PTY")
    ap.add_argument("--pty", required=True, help="e.g. /dev/pts/3")
    ap.add_argument("--image", default="kernel_payload.bin", help=".bin or .elf")
    ap.add_argument("--wait", type=float, default=0.2)
    ap.add_argument("--ack-timeout", type=float, default=2.0)
    args = ap.parse_args()

    data = open(args.image, "rb").read()
    hdr = struct.pack("<III", MAGIC, len(data), checksum32(data))

    fd = os.open(args.pty, os.O_RDWR | os.O_NOCTTY)
    try:
        time.sleep(args.wait)
        os.write(fd, hdr)
        os.write(fd, data)

        ack = read_ack(fd, args.ack_timeout)
        if ack:
            sys.stdout.write(f"ACK byte: {ack!r}\n")
        else:
            sys.stdout.write("ACK byte: <timeout>\n")

        print(f"Sent {len(data)} bytes from {args.image} to {args.pty}")
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
