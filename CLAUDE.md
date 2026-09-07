# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Read temperature and humidity inside a fruit storage box using a DHT-series sensor on an **Arduino Nano** (ATmega328P, AVR).

**The physical sensor is a DHT11, not a DHT22** — verified on hardware 2026-08-19. `DHTTYPE` in the sketch is set to `DHT11` to match. If a real DHT22 is fitted later, change that one `#define`; nothing else needs to move.

The sketch lives in `fruitsBox/fruitsBox.ino` (Arduino requires the sketch dir and `.ino` to share a name). It polls the sensor at the fitted sensor's rated sampling rate (1 Hz for the DHT11, 0.5 Hz for a DHT22) and prints one labelled line per reading at 9600 baud — `Temperature: 24.3 C   |   Humidity: 61.0 %` — values padded so the columns stay aligned. The same reading is mirrored on a 20x4 I2C LCD. Failed reads print a `--.-` row on both outputs rather than being dropped, keeping gaps visible.

## Toolchain

Arduino IDE 2.x is installed but `arduino-cli` is **not on PATH**. Use the copy bundled with the IDE:

```sh
CLI="/c/Users/alfre/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
```

It runs standalone (v1.4.1) and shares the IDE's data dir (`~/AppData/Local/Arduino15`), so cores and libraries installed via the IDE are visible to it and vice versa.

Installed cores: `arduino:avr` 1.8.7, `arduino:esp32`, `esp8266:esp8266`.

Sketch dependencies (already installed): `DHT sensor library` 1.4.7, `Adafruit Unified Sensor` 1.1.15, `LiquidCrystal I2C` 1.1.2. To restore them on another machine:

```sh
"$CLI" lib install "DHT sensor library" "Adafruit Unified Sensor" "LiquidCrystal I2C"
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

## Display notes

- **The screen shows five things only: header, uptime clock, temperature, humidity, mould risk.** Trend marks, a spinner and a generic status line were each tried and removed at the user's request (2026-08-20) — do not re-add liveness indicators to the LCD. The uptime clock is the sole liveness cue; everything else diagnostic belongs on serial. Row 3 was reclaimed for the mould risk verdict, which is information rather than an indicator.
- Above ~90 %RH the DHT11 pins at a fixed value that will not move however fast you poll. That is no longer flagged on screen, so a humidity reading stuck near 90+ is the sensor's ceiling, not a bug.
- **The display is a 20x4 (2004), not a 16x2** — confirmed visually 2026-08-20. `LCD_COLS`/`LCD_ROWS` are set to match.
- 20x4 LCD on a PCF8574 I2C backpack: `5V`/`GND` plus `SDA -> A4`, `SCL -> A5` (the Nano's fixed I2C pins — they are not remappable).
- This board's backpack answers at **`0x27`**, confirmed with an I2C scan on 2026-08-20.
- **Diagnosing a blank-but-backlit screen, in order:** run an I2C scan first — if it finds a device, the wiring is proven good (all four of VCC/GND/SDA/SCL must be right for anything to answer) and rewiring is wasted effort. If the scan finds the device at an address other than `LCD_ADDR`, fix the define (`0x27` = PCF8574, `0x3F` = PCF8574A). If the address is already correct, the cause is **contrast** — the blue trim pot on the backpack. Flashing a screen full of the `0xFF` block glyph makes that easy to dial in, since blocks show up well before text does.
- `lcdPrintLine()` blanks the tail of each row on every write. Without that, a 3-digit reading leaves a stray digit behind when the value drops back to 2 digits (`100.0` -> `61.0` would show `61.00`).
- **The HD44780's stock A00 character ROM is not ASCII above 0x5B.** `0x5C` is the **yen sign**, not a backslash, so a literal `\\` prints a stray `¥`. This caused a rotating-bar spinner to show a currency symbol every fourth frame.
- A `createChar()` CGRAM glyph was tried as a fix and **did not render on this display** — the stray character survived it, with the build stamp proving the new binary was running. Root cause never pinned down; the working fix was to stop needing a custom glyph at all. **Keep LCD text to plain ASCII below 0x5B** rather than reaching for CGRAM — the one non-ASCII byte on screen is the `0xDF` degree glyph, which comes from ROM and does render.
- The serial banner prints `__DATE__ " " __TIME__`. When the display contradicts the code, read that stamp first: it separates a stale flash from a real bug, and the IDE's open editor tab can re-upload older source over yours.
- The degree glyph is HD44780 code `0xDF`, not UTF-8 `°`. It is written as `"\xDF" "C"` — split into two string literals because a hex escape swallows following hex digits, so `"\xDFC"` would compile as one out-of-range character.
- The LCD library is `LiquidCrystal_I2C` by Frank de Brabander; it initialises with `init()`, not `begin()`. The backlight is off until `backlight()` is called.
- The AVR's cut-down `snprintf` has no reliable printf **field widths** (`%-12s`), so columns are aligned with padded literals and `dtostrf()` instead. It also cannot print floats with `%f` at all — `dtostrf()` is the only route.
- The uptime clock is redrawn on a 1 s throttle, not every `loop()` pass; unthrottled it floods the I2C bus and flickers the row.

## Mould risk matrix

The risk matrix was supplied by the user 2026-08-20 and lives in `moldRiskIndex()`.

| Temperature | RH | Risk | Index |
|---|---|---|---|
| 10–15 °C | 50–60 % | Low | 0 |
| 16–20 °C | 60–70 % | Low–Moderate | 1 |
| 21–25 °C | 70–80 % | Moderate–High | 2 |
| 26–30 °C | 80–90 % | High | 3 |
| >30 °C | >90 % | High | 4 |

- **The matrix is not actually two-dimensional.** Each row advances temperature *and* humidity together, which only describes a box where warmth and damp rise in step. A real box can be warm and dry, or cool and damp, and the table says nothing about those.
- **Resolution: score each axis independently, return the worse band.** No reading can score lower than either axis alone would, so the warning fails safe. The deliberate cost is that warm-and-dry (32 °C / 45 %RH) reports `HIGH` on temperature alone, though mould needs moisture — unlikely inside a closed box of respiring fruit. If a more accurate model is ever wanted, gate on humidity first and use temperature only to raise the band.
- Bands are **upper-inclusive**, which resolves both defects in the printed table: the gap between temperature rows (15 then 16) and the overlap between humidity rows (50–60 then 60–70).
- Below the matrix floors (under 10 °C, under 50 %RH) falls in band 0 — cooler and drier than its safest row.
- Verified on hardware 2026-08-20 by lifting `moldRiskIndex()` verbatim into a test sketch: all five rows reproduce, edges land where intended. Re-run that harness if the bands are ever edited.
- **This box normally reads `HIGH`** (~26 °C / ~85 %RH sits squarely in row 4), so `HIGH` is the expected steady state here, not an alarm condition.

## Sensor notes

- **Diagnosing a type mismatch:** a DHT11 read as a DHT22 yields absurd but *stable* values with no `NaN` — the checksum still passes, because the bits are read correctly and only the decoding differs. Observed: 691.2 °C / 2073.7 %RH, which is raw bytes `27,0` and `81,1` = 27.0 °C / 81.1 %RH under DHT11 encoding. Wild-but-steady readings mean wrong `DHTTYPE`; `NaN` or garbage that jumps around means wiring or pull-up.
- DHT11 vs DHT22 matters for this project: the DHT11 covers 20–90 %RH at ±5 % and 0–50 °C at ±2 °C. A fruit box held at high humidity can sit at or above the top of that range, where the DHT11 saturates. The DHT22 covers 0–100 %RH at ±2–5 % and −40–80 °C at ±0.5 °C.

- Rated sampling rates differ by part: the **DHT11 is 1 Hz**, the **DHT22 0.5 Hz**. `READ_INTERVAL_MS` is selected by `#if DHTTYPE == DHT11` so swapping the sensor cannot leave the Nano polling a DHT22 at twice its datasheet rate.
- The library imposes its own blanket **2 s frame cache** (`MIN_INTERVAL`, `DHT.cpp:239`) sized for the DHT22. At a 1 s poll it silently returns the *previous* frame instead of erroring, so the display looks frozen while appearing to work. `readHumidity(true)` forces past it.
- Only the humidity getter is forced; `readTemperature()` is left unforced so it reuses that same cached frame. Forcing both would read the sensor twice and let the two halves of one on-screen row come from different moments.
- Measured on hardware 2026-08-20 at a 1 s poll: **28 readings in ~30 s, zero failed reads** — the DHT11 keeps up fine at its rated rate.
- Polling faster than rated returns stale or `NaN` values rather than erroring — always check `isnan()` on both readings before using them.
- The library's temperature getter is `readTemperature()` (Celsius by default; pass `true` for Fahrenheit). There is no `readCelsius()`.
- The first read after `begin()` commonly returns `NaN`; the sketch discards it during setup.
- The data line needs a **10k pull-up to VCC**; the sensor works at 3.3 V or 5 V, but on a 5 V Nano use 5 V for both VCC and the pull-up.
