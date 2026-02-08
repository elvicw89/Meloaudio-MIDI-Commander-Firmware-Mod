# YEAHWAAH MODS

This document lists the custom modifications made to the MeloAudio MIDI Commander firmware for this build.

## Credits
- Based on the open-source project: `https://github.com/harvie256/midi-commander-custom#`

## Firmware Behavior Changes
- Removed CSV dependency by hardcoding MIDI mappings in firmware.
- Forced a single bank only (bank switching disabled).
- SW5 and SW_E are press-only (no action on release); added press-latch guards to prevent double-triggering from switch bounce.

## Switch Mappings (MIDI Channel 1)
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

## Display & UI
- Boot screen shows centered text: "Modded by" and "YEAHWAAH" with a thin progress bar.
- Replaced default bank/config text with an ATC logo splash.
- On switch press, screen shows a label for 3 seconds, then returns to ATC logo.
- Labels use the Old English font from `heraldic-shadows` and are centered.

## Label Mapping
- SW1: SCENE 1
- SW2: SCENE 2
- SW3: SCENE 3
- SWA: EFFECTS 1
- SWB: EFFECTS 2
- SWC: DLY/RVB
- SW5: PATCH UP
- SWE: PATCH DOWN

## Notes
- Battery and expression probe code was explored but is currently disabled.

  # Expression Pedal Mods (CC11 + Stability)
  ## Date: 2026-02-09

  # Summary
  - Expression pedal now sends CC11 (Expression) on the global MIDI channel.
  - Added filtering, deadband, stability gating, and minimum send interval to prevent jitter.

  Firmware Changes
  1) New CC sender helper
  - File: MIDI_Commander_Custom/Core/Inc/midi_cmds.h
    - Added: int8_t midiCmd_send_cc_value(uint8_t channel, uint8_t cc_number, uint8_t value);
  - File: MIDI_Commander_Custom/Core/Src/midi_cmds.c
    - Added midiCmd_send_cc_value(): sends single CC message on specified channel/CC/value.

  2) Expression pedal input + CC11 mapping + stability
  - File: MIDI_Commander_Custom/Core/Src/main.c
    - Added expression subsystem:
      - EXPRESSION_ENABLED = 1
      - EXPRESSION_ADC_CHANNEL = 8 (PB0)
      - EXPRESSION_CC_NUMBER = 11
      - EXPRESSION_POLL_MS = 12
      - EXPRESSION_DEADBAND = 4
      - EXPRESSION_STABLE_REQUIRED = 5
      - EXPRESSION_MIN_SEND_MS = 30
      - EXPRESSION_MIN_RAW = 100
      - EXPRESSION_MAX_RAW = 4000
      - EXPRESSION_SCALE_NUM/DEN = 105/100
    - Added expression_gpio_init() to configure selected ADC pin as analog input.
    - Added expression_apply_calibration(): clamps and scales raw ADC.
    - Added expression_update():
      - EMA filter (3/4 previous + 1/4 new)
      - Converts 12-bit to 7-bit via >> 5
      - Deadband and stability gating
      - Minimum send interval
      - Sends CC11 via midiCmd_send_cc_value().
    - Hooked expression_update() into main loop.
    - ADC init runs when expression mode is enabled.

  Behavior Notes
  - Uses global MIDI channel from pGlobalSettings[GLOBAL_SETTINGS_CHANNEL] (defaults to 1).
  - Value range is 0..127.
  - Deadband/stability settings were tuned to eliminate idle jitter.

  Artifacts
  - Built firmware: MIDI_Commander_Custom/DFU Release/MIDI_Commander_Custom.bin
