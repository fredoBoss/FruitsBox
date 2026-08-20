# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Read temperature and humidity inside a fruit storage box using a DHT-series sensor on an **Arduino Nano** (ATmega328P, AVR).

**The physical sensor is a DHT11, not a DHT22** — verified on hardware 2026-08-19. `DHTTYPE` in the sketch is set to `DHT11` to match. If a real DHT22 is fitted later, change that one `#define`; nothing else needs to move.

The sketch lives in `fruitsBox/fruitsBox.ino` (Arduino requires the sketch dir and `.ino` to share a name). It polls the sensor every 2 s and prints one CSV line per reading — `millis,temperature_C,humidity_pct` — at 9600 baud, so the serial log can be captured straight to a file. Failed reads emit a row with empty value fields rather than being dropped, keeping gaps visible in the log.

Not a git repo yet.

## Toolchain

Arduino IDE 2.x is installed but `arduino-cli` is **not on PATH**. Use the copy bundled with the IDE:

```sh
CLI="/c/Users/alfre/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
```

It runs standalone (v1.4.1) and shares the IDE's data dir (`~/AppData/Local/Arduino15`), so cores and libraries installed via the IDE are visible to it and vice versa.

Installed cores: `arduino:avr` 1.8.7, `arduino:esp32`, `esp8266:esp8266`.

Sketch dependencies (already installed): `DHT sensor library` 1.4.7, `Adafruit Unified Sensor` 1.1.15. To restore them on another machine:

```sh
"$CLI" lib install "DHT sensor library" "Adafruit Unified Sensor"
```

## Commands

```sh
# Compile (sketch dir name must match the .ino filename)
"$CLI" compile --fqbn arduino:avr:nano fruitsBox

# Upload
"$CLI" upload -p COM4 --fqbn arduino:avr:nano fruitsBox

# Serial monitor
"$CLI" monitor -p COM4 -c baudrate=9600

# Which board is on which port
"$CLI" board list
```

## Board / port notes

- The port number is **not stable** — the Nano appeared as COM4, then COM5 after a replug. Always confirm with `board list` before uploading; a missing port fails with "cannot find the file specified".
- `board list` reports the board as `Unknown` — clone Nanos use a CH340 USB chip that carries no Arduino USB VID/PID, so the FQBN must always be passed explicitly; auto-detection will not fill it in.
- Nano clones usually ship the **old** bootloader. If upload fails with a sync/timeout error (`stk500_recv(): programmer is not responding`), the FQBN is the fix, not the wiring:
  `--fqbn arduino:avr:nano:cpu=atmega328old`

- Uploads fail with **"Access is denied"** when the Arduino IDE's Serial Monitor holds the port. Close the monitor tab (or kill the `serial-monitor` helper process) before uploading from the CLI. This is distinct from the port-missing error above.

## Sensor notes

- **Diagnosing a type mismatch:** a DHT11 read as a DHT22 yields absurd but *stable* values with no `NaN` — the checksum still passes, because the bits are read correctly and only the decoding differs. Observed: 691.2 °C / 2073.7 %RH, which is raw bytes `27,0` and `81,1` = 27.0 °C / 81.1 %RH under DHT11 encoding. Wild-but-steady readings mean wrong `DHTTYPE`; `NaN` or garbage that jumps around means wiring or pull-up.
- DHT11 vs DHT22 matters for this project: the DHT11 covers 20–90 %RH at ±5 % and 0–50 °C at ±2 °C. A fruit box held at high humidity can sit at or above the top of that range, where the DHT11 saturates. The DHT22 covers 0–100 %RH at ±2–5 % and −40–80 °C at ±0.5 °C.

- DHT22 is slow: minimum **2 s** between reads. Polling faster returns stale or `NaN` values rather than erroring — always check `isnan()` on both readings before using them.
- The library's temperature getter is `readTemperature()` (Celsius by default; pass `true` for Fahrenheit). There is no `readCelsius()`.
- The first read after `begin()` commonly returns `NaN`; the sketch discards it during setup.
- The data line needs a **10k pull-up to VCC**; the sensor works at 3.3 V or 5 V, but on a 5 V Nano use 5 V for both VCC and the pull-up.
