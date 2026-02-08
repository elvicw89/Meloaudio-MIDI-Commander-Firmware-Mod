# MeloAudio MIDI Commander Custom Firmware

This repository contains a custom firmware build for the MeloAudio MIDI Commander, plus tooling and notes for configuring, flashing, and maintaining it. It combines:

- Firmware sources under `MIDI_Commander_Custom/`
- DFU artifacts under `DFU/`
- A CSV-to-flash configuration tool (see `python/CSV_to_Flash.py`)
- A specific “YEAHWAAH” mod variant documented below
- Third-party display driver sources under `MIDI_Commander_Custom/Middlewares/`

## Status and Scope

This firmware is intended for custom requirements that the stock firmware does not address. It does not aim to replace the original firmware for general use. Builds in `DFU/DFU_OUT/` are development builds tested on the maintainer’s device; use at your own risk and keep a stock firmware fallback.

## Feature Summary (Base Firmware)

- Spreadsheet-driven configuration (no device menu scrolling)
- Supports Program Change, Control Change, Note, Pitch Bend, and Start/Stop
- Per-command MIDI channel selection
- 8 banks of 8 switches
- Up to 10 chained commands per switch
- Momentary, toggle, or timed CC/Note/Pitch Bend actions (up to 2.5s)
- Optional Bank Select (LSB and/or MSB) for Program Change
- USB-to-Serial MIDI Start/Stop pass-through
- Configuration stored in internal flash (does not overwrite stock EEPROM config)

## Configuration Workflow

Configuration is done with a spreadsheet. A public template exists and a copy is included in the repo as:

- `python/MeloConfig_10_Cmds - RC-600.csv`

The CSV format supports:

- Message type: PC/CC/Note/PB/Start/Stop
- MIDI channel
- Value numbers and on/off values
- Bank Select MSB/LSB for Program Change
- Toggle mode for CC/Note/PB
- Note velocity and duration

Lines beginning with `#` or `*` are treated as comments.

### Loading a CSV to the Device

1. Install Python and dependencies:

```
cd /path/to/midi-commander-custom
python3 -m pip install -r python/requirements.txt
python3 python/CSV_to_Flash.py -h
```

2. Put the device in normal mode (not DFU), connect USB, then run:

```
cd /path/to/midi-commander-custom
python3 python/CSV_to_Flash.py /path/to/your/configuration-file.csv
```

The device reboots after the upload and loads the new configuration.

## Building and Flashing the Firmware

### Build Environment

Use STM32CubeIDE and import the project from `MIDI_Commander_Custom/`.

Build targets:

- `DFU Release`: for DFU uploads
- `Debug`: for ST-Link debugging

### DFU (macOS)

You can flash using `dfu-util`:

```
brew install dfu-util
```

Enter DFU mode by holding `bank down` + `D` while pressing power. The display remains blank; LED 3 turns on.

List devices:

```
dfu-util --list
```

Flash a DFU image from `DFU/DFU_OUT/`:

```
dfu-util --alt 0 --download ./DFU/DFU_OUT/generated-*.dfu
```

If you have a `.bin` (from `DFU Release`), specify the address:

```
dfu-util --alt 0 -s 0x8003000 --download "./MIDI_Commander_Custom/DFU Release/MIDI_Commander_Custom.bin"
```

Power cycle the device to return to normal mode.

## YEAHWAAH Mod (Custom Build Notes)

This build variant hardcodes mappings and UI behavior for a specific setup.

### Behavior Changes

- CSV dependency removed; mappings are compiled into firmware
- Single bank only (bank switching disabled)
- SW5 and SW_E are press-only (no release action); press-latch guards prevent bounce double-triggers

### MIDI Mappings (Channel 1)

- SW1: CC25 value 1, momentary
- SW2: CC25 value 2, momentary
- SW3: CC25 value 3, momentary
- SW4: CC28, toggle (on=127, off=0)
- SWA: CC79, toggle (on=127, off=0)
- SWB: CC80, toggle (on=127, off=0)
- SWC: CC81, toggle (on=127, off=0)
- SWD: CC60, toggle (on=127, off=0)
- SW5: CC27 value 0, momentary (press-only)
- SWE: CC26 value 0, momentary (press-only)

### Display & UI

- Boot screen shows centered text: "Modded by" and "YEAHWAAH" with a thin progress bar
- Bank/config text replaced by an ATC logo splash
- On switch press, display shows a label for 3 seconds, then returns to ATC logo
- Labels use the Old English font from `heraldic-shadows`, centered on screen

### Label Mapping

- SW1: SCENE 1
- SW2: SCENE 2
- SW3: SCENE 3
- SWA: EFFECTS 1
- SWB: EFFECTS 2
- SWC: DLY/RVB
- SW5: PATCH UP
- SWE: PATCH DOWN

### Notes

- Battery and expression probe code exists but is currently disabled

## Display Driver Library

The repo includes `stm32-ssd1306` in `MIDI_Commander_Custom/Middlewares/stm32-ssd1306-master/`. It supports OLEDs based on SSD1306 (plus SH1106/SH1107/SSD1309) over I2C or 4-wire SPI. See:

- `MIDI_Commander_Custom/Middlewares/stm32-ssd1306-master/ssd1306/ssd1306_conf_template.h`
- `MIDI_Commander_Custom/Middlewares/stm32-ssd1306-master/examples/`

Upstream references (in case you need them):

- `https://github.com/afiskon/stm32-ssd1306`
- `https://github.com/4ilo/ssd1306-stm32HAL`

## Acknowledgements

- harvie256: project founder
- eliericha: expansion to 10 commands per button
