#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir="$repo_dir/esp32"

if [[ -n "${PIO_BIN:-}" ]]; then
    pio_bin=$PIO_BIN
elif command -v pio >/dev/null 2>&1; then
    pio_bin=$(command -v pio)
elif [[ -x "${HOME}/.platformio/penv/bin/pio" ]]; then
    pio_bin=${HOME}/.platformio/penv/bin/pio
else
    echo "PlatformIO was not found. Install it or set PIO_BIN to its path." >&2
    exit 1
fi

port=${PORT:-${1:-}}
upload_args=(run --project-dir "$project_dir" --target upload)
monitor_args=(device monitor --project-dir "$project_dir" --baud 115200)

if [[ -n "$port" ]]; then
    upload_args+=(--upload-port "$port")
    monitor_args+=(--port "$port")
fi

"$pio_bin" run --project-dir "$project_dir"
firmware_elf="$project_dir/.pio/build/esp32-s3-n16r8/firmware.elf"
if [[ -f "$firmware_elf" ]]; then
    echo "Firmware ELF SHA256: $(shasum -a 256 "$firmware_elf" | awk '{print $1}')"
fi
"$pio_bin" "${upload_args[@]}"

echo "Opening the 115200-baud serial monitor; press Ctrl-C to stop."
"$pio_bin" "${monitor_args[@]}"
