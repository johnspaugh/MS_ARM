#!/usr/bin/env bash
set -euo pipefail
qemu-system-aarch64 \
  -cpu cortex-a72 \
  -M raspi4b \
  -kernel bootloader.img \
  -m 2G -smp 4 \
  -nographic -monitor none \
  -serial pty
