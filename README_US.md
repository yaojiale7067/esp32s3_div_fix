

# ESP32-DIV (PlatformIO Port)



Port of [ESP32-DIV](https://github.com/cifertech/ESP32-DIV) to PlatformIO + Arduino framework, with pins, display, touch and the two RF buses rewritten for this board.

The original project is an Arduino IDE `.ino` sketch that relies on PCF8574 physical buttons, a buzzer, WS2812 status LEDs and XPT2046 resistive touch. This board has none of those, so UI interaction was changed to a **pure touch navigation bar**, and external modules are changed to **connect on demand — if missing, the UI directly reports them as unavailable**, so it no longer fails to boot just because one module is missing.

---

## 1. Hardware

| Part | Model / Spec | Notes |
|---|---|---|
| MCU | ESP32-S3 N16R8 | 16MB QIO flash + 8MB OPI PSRAM, dual-core 240MHz |
| Display | 240×320 ILI9341 SPI | native portrait orientation, **no landscape rotation** |
| Touch | FT6336 capacitive touch (I2C) | bonded screen integrated with the display |
| Storage | microSD | shares the SPI bus with the display, only chip select differs |
| 2.4GHz | nRF24L01+ | dedicated chip select, scanning / fixed-frequency jamming |
| Sub-GHz | CC1101 | shares SCK/MISO/MOSI with nRF24, dedicated chip select |

### Wiring



**Display (SPI2)**

| Signal | GPIO |
|---|---|
| SCLK | 12 |
| MOSI | 11 |
| MISO | 13 |
| CS | 10 |
| DC | 9 |
| RST | 8 |
| Backlight BL | 21 (PWM dimming) |

**SD card**: SCLK 12 / MOSI 11 / MISO 13 (fully shared with the display), **CS = 4**.

**FT6336 touch (I2C)**: SCL 1 / SDA 2 / RST 42, INT not connected (polling mode).

**nRF24 (SPI3)**: SCK 16 / MISO 17 / MOSI 18 / **CSN 15** / CE 14, IRQ not connected.

**CC1101 (same SPI3 as nRF24)**: SCK 16 / MISO 17 / MOSI 18 / **CSN 5**, GDO0 6, GDO2 not connected.

```
        ┌──────────── SPI2 (display + SD) ─────────┐
        │  SCLK 12   MOSI 11   MISO 13             │
        │  TFT_CS 10   SD_CS 4                     │
        └──────────────────────────────────────────┘

        ┌──────────── SPI3 (nRF24 + CC1101) ───────┐
        │  SCLK 16   MOSI 18   MISO 17             │
        │  NRF_CS 15   CC1101_CS 5                 │
        └──────────────────────────────────────────┘

        I2C (touch)  SCL 1  SDA 2  RST 42
```

> **CC1101 VCC must be 3V3 only**. The module's operating range is 1.8–3.6V; connecting 5V will destroy it immediately.
>
> Difference between connecting GDO0 or not: **scanning / jamming** are pure SPI operations and work without it; **recording and replaying remote codes** use RCSwitch software OOK encoding/decoding, which goes through GDO0 — without it no codes can be recorded.

### What this board doesn't have

| Missing | Behavior in code |
|---|---|
| PCF8574 button board | `HAS_PCF8574_BUTTONS 0`; navigation relies entirely on the 5-cell touch bar at the bottom of the screen |
| Buzzer | `BUZZER_PIN -1` |
| WS2812 status LED | `STATUS_LED_COUNT 0` |
| Battery divider resistors | `BATTERY_ADC_PIN -1`; `readBatteryVoltage()` returns `NAN` directly |
| PN532 / IR / GPS modules | all pins are `-1`; entering the corresponding feature reports unavailable and no GPIO is touched |

---

## 2. Features

Main menu is 2 columns × 4 rows, 8 items total:

```
┌──────────┬──────────┐
│  WiFi    │  Tools   │
├──────────┼──────────┤
│  More    │  About   │
├──────────┼──────────┤
│ Settings │  2.4GHz  │
├──────────┼──────────┤
│Bluetooth │  SubGHz  │
└──────────┴──────────┘
```

| Main menu | Sub-features |
|---|---|
| WiFi | Packet Monitor / Beacon Spammer / WiFi Deauther / Probe Request Flood / Deauth Detector / WiFi Scanner / Captive Portal |
| 2.4GHz | Scanner (126-channel RPD spectrum) / Proto Kill (8 categories of targeted fixed-frequency and frequency-hopping jamming) |
| SubGHz | Replay Attack / SubGHz Jammer / Saved Profile |
| Bluetooth | BLE Spoofer / Sour Apple / Sniffer / BLE Scanner / BLE Rubber Ducky |
| Tools | Serial Monitor / Update Firmware / SD File Manager |
| More | IR Remote (Record / Saved Profile / Universal Controller), RFID/NFC (9 items), GPS (Wardriver / Satellite Scanner) |
| Settings | Brightness / Theme / Accent / Auto Scan |

**Interaction**: there is a 5-cell touch navigation bar at the bottom of the screen, corresponding from left to right to `Left / Down / Center / Up / Right`. In feature screens the middle cell is always **Exit**, and the other cells are defined per screen (`Next`, `Run`, `Save`, etc.). Main menu items are **entered with a single tap**, no long press needed.

Settings are written to `/config/settings.json` on the SD card; defaults are used when no SD card is present.

---

## 3. Build and Flash


Requires [PlatformIO](https://platformio.org/) (VS Code extension or command line, either works).

```bash
# build
pio run

# flash (replace COM5 with the actual serial port)
pio run -t upload --upload-port COM5

# view boot log (interactive, Ctrl+C to exit)
pio device monitor --port COM5 --baud 115200
```

In non-interactive environments `pio device monitor` refuses to run; use the small script included in the repo instead:

```bash
python tools/serial_probe.py COM5 115200 12 noreset
# args: serial port / baud rate / seconds to read / noreset means do not pulse DTR
```

> **Why `noreset`**: when the script pulses DTR to force a reset, there are glitches on the UART line, and the first few dozen milliseconds of output after reset get swallowed, making it look like it is "stuck in `tft.init()`". When troubleshooting boot problems, always use `noreset`, or hard-reset with esptool first and read immediately.

Current build usage: **Flash 54.7% (about 1.68MB / 3.08MB)**, **RAM 34.8% (about 114KB / 320KB)**.

### Partition table

`partitions_16MB.csv`, dual OTA slots + 64KB coredump:

| Partition | Offset | Size |
|---|---|---|
| nvs | 0x9000 | 20KB |
| otadata | 0xe000 | 8KB |
| app0 (ota_0) | 0x10000 | 3MB |
| app1 (ota_1) | 0x300000 | 3MB |
| spiffs | 0x5F0000 | 2MB |
| coredump | 0x7F0000 | 64KB |

> This CSV **must be ASCII only**. Writing GBK Chinese comments in it makes PlatformIO's partition parser throw `UnicodeDecodeError` and the build aborts.

---

## 4. Directory Structure



```
├── platformio.ini           build configuration (env: esp32-s3-devkitc-1)
├── partitions_16MB.csv      16MB flash partition table
├── src/
│   ├── main.cpp             menu, touch navigation bar, setup/loop main flow
│   ├── shared.h             all pin definitions, UI palette, panel dimensions, feature switches
│   ├── config.h             common includes and namespace declarations
│   ├── radio_bus.cpp/.h     SPI3 shared bus (nRF24 + CC1101 shared)
│   ├── nrf24.cpp/.h         2.4GHz Scanner + Proto Kill
│   ├── subghz.cpp/.h        CC1101 Sub-GHz three features
│   ├── wifi.cpp             WiFi seven features
│   ├── bluetooth.cpp        BLE five features (NimBLE)
│   ├── ir.cpp               IR transmit/receive
│   ├── rfid.cpp             PN532 RFID/NFC
│   ├── gps.cpp              GPS / Wardriver
│   ├── ducky.cpp            BLE Rubber Ducky
│   ├── Touchscreen.cpp/.h   FT6336 driver and coordinate mapping
│   ├── KeyboardUI.cpp/.h    on-screen soft keyboard
│   ├── SettingsStore.cpp/.h settings persistence
│   ├── utils.cpp/.h         status bar, notifications, touch navigation bar, SD init and other common parts
│   └── icon.h               bitmap icons
├── lib/                     all dependencies pinned locally, no network access during build
│   ├── TFT_eSPI/            2.5.43 (with this board's User_Setup.h, S3 patch applied)
│   ├── FT6336-arduino/      capacitive touch driver
│   ├── NimBLE-Arduino/      BLE stack
│   ├── IRremoteESP8266/     IR
│   ├── Adafruit_PN532/      RFID/NFC
│   ├── Adafruit_BusIO/      PN532 I2C/SPI low-level layer
│   ├── SmartRC-CC1101/      CC1101 driver (modified for shared bus)
│   ├── rc-switch/           OOK encoding/decoding (Replay / Profile)
│   ├── ArduinoJson/         config read/write
│   └── arduinoFFT/          WiFi spectrum
├── tools/serial_probe.py    small serial log capture tool
└── ESP32-DIV/               original Arduino project, for reference only, not part of the build
```

---

## 5. Implementation Notes (Pitfalls Hit)


### 1. The two RF buses must not be mixed

The display and SD go through **SPI2**, nRF24 and CC1101 go through **SPI3**. `SPIClass` pin mapping is a one-time binding — `begin()` returns immediately if `_spi` is non-null, and later calls won't change the pins. Therefore:

- `tft.init()` must run **before** `SD.begin()`, otherwise the display pins will never be configured;
- nRF24 and CC1101 must get the **same** `SPIClass` instance (`src/radio_bus.cpp`), otherwise the second `begin()` will call `periph_module_reset(SPI3)` and cut off the channel that is currently communicating;
- `USE_HSPI_PORT` must **never** be defined in `lib/TFT_eSPI/User_Setup.h`. If defined, TFT_eSPI creates its own `SPIClass spi = SPIClass(HSPI)`, holding a separate instance and a separate `_inTransaction` lock from SD, and transaction mutual exclusion is completely broken.

### 2. TFT_eSPI's S3 port number must be patched

Upstream `Processors/TFT_eSPI_ESP32_S3.h` has `#define SPI_PORT FSPI` in its default branch. But Arduino core defines `FSPI` as **0** on the S3, and IDF's `REG_SPI_BASE(i)` is `(i >= 2) ? ... : 0`, so macros like `SPI_USER_REG` get constant-folded to `0x10` and `SPI_W0_REG` to `0x98` — the very first command sent by `tft.init()` writes to address `0x10`, immediately causing `StoreProhibited` and an infinite boot loop.

Changed to `#define SPI_PORT 2` (corresponding to the SPI2 window `0x60024000`). **Do not overwrite this directory with official source code.**

### 3. The CC1101 driver originally tore down the whole bus

The upstream driver calls `SPI.begin(...)` + `SPI.end()` on every register access, and uses the global `SPI` (which is SPI2, the display bus). It has been changed so that `SpiStart()` only prepares its own chip select and calls `beginTransaction()`, `SpiEnd()` only calls `endTransaction()`, and all transfers go through `radioBus()`.

Also, the driver has a global `bool spi`, which has the same name as the `SPIClass& spi` exported by TFT_eSPI, causing a `multiple definition` error at link time. Renamed to `static bool cc1101_customSpi`.

### 4. Touch coordinates are mapped only once

FT6336 always outputs panel native coordinates (`ROTATION_NORMAL`); screen orientation changes are handled uniformly in `mapTouchToScreen()` in `Touchscreen.cpp` according to `TFT_ROTATION`. Do not rotate again in the driver layer, otherwise it is rotated twice.

### 5. Menu indices are column-major

The main menu layout is 2 columns × 4 rows, with the index algorithm `col = i / 4`, `row = i % 4`. When adding a menu item, the following places must all be updated together; missing one causes "tap left, jump right":

- `NUM_MENU_ITEMS` and `menu_items[]` / `bitmap_icons[]`
- the `MAIN_MENU_*_IDX` index constants
- the `case`s in `updateActiveSubmenu()`
- the `case` dispatch for `in_sub_menu` in `handleButtons()`
- the length of `icon_colors[]` in `displayMenu()`

### 6. Feature exit relies on `feature_exit_requested`

Every feature's `loop()` begins with:

```c
if (feature_active && (feature_exit_requested || featureExitButtonPressed())) {
    feature_exit_requested = true;
    return;
}
```

The `while` in the outer `run*Feature()` exits based on this flag, and after exiting, `in_sub_menu` / `feature_active` / the touch navigation bar are all reset in one place. Write new features following this pattern; don't `return` to the main menu on your own.

---

## 6. Known Limitations



- **PN532 / IR / GPS not connected**: pins are `-1`, feature entry reports unavailable. To restore, change the corresponding macros in `src/shared.h` to actual pins.
- **Replay Attack / Saved Profile depend on GDO0**: without GDO0 connected, recording and replay don't work for these two features; SubGHz Jammer is unaffected.
- **Jamming features (Proto Kill / SubGHz Jammer) effectiveness depends on the environment and target device** — don't expect "one button to kill everything".
- **Status bar battery always invalid**: this board has no battery divider circuit; with `BATTERY_ADC_PIN -1`, `readBatteryVoltage()` returns `NAN`.
- **No touch calibration in Settings**: FT6336 factory coordinates are accurate enough; mapping parameters are hardcoded at the top of `Touchscreen.cpp` — adjust `TOUCH_MAP_X1/X2/Y1/Y2` if needed.
- Some legacy compile warnings remain in `src/` (`ICON_NUM` / `MAX_LINES` / `STATUS_BAR_Y_OFFSET` redefinition, etc.); they don't affect operation.

---

## 7. Sources



- Original project: [ESP32-DIV](https://github.com/DeaDHackS/ESP32-DIV) — the menu structure and the UI/logic of the WiFi/BLE/IR/RFID/GPS/Ducky features all come from here
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)
- [SmartRC-CC1101-Driver-Lib](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib) by LSatan
- [rc-switch](https://github.com/sui77/rc-switch) by sui77
- [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532) / [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) by Benoit Blanchon
- [arduinoFFT](https://github.com/kosme/arduinoFFT) by Enrique Condes

The libraries under `lib/` are all locally pinned copies, and TFT_eSPI and SmartRC-CC1101 carry board-specific patches (see Section 5 above). **Before upgrading dependencies, first confirm the patches are still present.**

---

## 8. Disclaimer


These tools actively transmit 2.4GHz / Sub-GHz signals and perform WiFi- and BLE-related operations. Use them only on **devices you own** or in environments where you have **explicit written authorization**. In most countries and regions, jamming public frequency bands and launching authentication attacks against other people's networks is illegal. The author is not responsible for any misuse.