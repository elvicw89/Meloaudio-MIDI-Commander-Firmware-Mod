# YEAHWAAH MODS

Custom firmware modifications for the **MeloAudio MIDI Commander**, coded specifically for use with the **Hotone Ampero 2 Stomp**.

All MIDI mappings, labels, and the expression pedal behavior in this build are tailored to control the Ampero 2 Stomp (scene switching, effect toggles, delay/reverb, and patch up/down).

## Credits
- Based on the open-source project: [harvie256/midi-commander-custom](https://github.com/harvie256/midi-commander-custom)

## Firmware Behavior Changes
- Removed CSV dependency by hardcoding MIDI mappings in firmware.
- Forced a single bank only (bank switching disabled).
- SW5 and SWE are press-only (no action on release). Press-latch guards prevent double-triggering from switch bounce.

## Switch Mappings (MIDI Channel 1)

| Switch | Message | Mode | Label |
|--------|---------|------|-------|
| SW1 | CC25, value 1 | Momentary | SCENE 1 |
| SW2 | CC25, value 2 | Momentary | SCENE 2 |
| SW3 | CC25, value 3 | Momentary | SCENE 3 |
| SW4 | CC28 | Toggle (on=127, off=0) | n/a |
| SWA | CC79 | Toggle (on=127, off=0) | EFFECTS 1 |
| SWB | CC80 | Toggle (on=127, off=0) | EFFECTS 2 |
| SWC | CC81 | Toggle (on=127, off=0) | DLY/RVB |
| SWD | CC60 | Toggle (on=127, off=0) | n/a |
| SW5 | CC27, value 0 | Momentary (press-only) | PATCH UP |
| SWE | CC26, value 0 | Momentary (press-only) | PATCH DOWN |

## Display & UI
- Boot screen shows centered text, "Modded by" and "YEAHWAAH", with a thin progress bar.
- Default bank/config text replaced with an ATC logo splash.
- On switch press, the screen shows a label for 3 seconds, then returns to the ATC logo.
- Labels are centered and use the Old English font from `heraldic-shadows`.

## Expression Pedal (CC11)
*Added 2026-02-09*

The expression pedal sends **CC11 (Expression)** on the global MIDI channel (defaults to 1), with a value range of 0–127. Filtering, deadband, stability gating, and a minimum send interval eliminate idle jitter.

### Firmware Changes

**1. New CC sender helper**
- `MIDI_Commander_Custom/Core/Inc/midi_cmds.h`
  - Added `int8_t midiCmd_send_cc_value(uint8_t channel, uint8_t cc_number, uint8_t value);`
- `MIDI_Commander_Custom/Core/Src/midi_cmds.c`
  - Added `midiCmd_send_cc_value()`, which sends a single CC message on the specified channel, CC, and value.

**2. Expression pedal input, CC11 mapping, and stability**
- `MIDI_Commander_Custom/Core/Src/main.c`
  - Added expression subsystem with the following settings:

| Setting | Value |
|---------|-------|
| `EXPRESSION_ENABLED` | 1 |
| `EXPRESSION_ADC_CHANNEL` | 8 (PB0) |
| `EXPRESSION_CC_NUMBER` | 11 |
| `EXPRESSION_POLL_MS` | 12 |
| `EXPRESSION_DEADBAND` | 4 |
| `EXPRESSION_STABLE_REQUIRED` | 5 |
| `EXPRESSION_MIN_SEND_MS` | 30 |
| `EXPRESSION_MIN_RAW` | 100 |
| `EXPRESSION_MAX_RAW` | 4000 |
| `EXPRESSION_SCALE_NUM/DEN` | 105/100 |

  - Added `expression_gpio_init()` to configure the selected ADC pin as an analog input.
  - Added `expression_apply_calibration()` to clamp and scale the raw ADC value.
  - Added `expression_update()`, which:
    - Applies an EMA filter (3/4 previous + 1/4 new)
    - Converts 12-bit to 7-bit via `>> 5`
    - Applies deadband and stability gating
    - Enforces the minimum send interval
    - Sends CC11 via `midiCmd_send_cc_value()`
  - Hooked `expression_update()` into the main loop.
  - ADC init runs when expression mode is enabled.

## Build Artifacts
- Built firmware: `MIDI_Commander_Custom/DFU Release/MIDI_Commander_Custom.bin`

## Flashing

```bash
dfu-util -a 0 -s 0x08003000:leave -D "MIDI_Commander_Custom/DFU Release/MIDI_Commander_Custom.bin"
```

## Roadmap
- [ ] Mini UI displaying expression pedal values
- [ ] Battery usage display
- [ ] Battery charging indicator
- Note: Battery and expression probe code was explored earlier but is currently disabled.
