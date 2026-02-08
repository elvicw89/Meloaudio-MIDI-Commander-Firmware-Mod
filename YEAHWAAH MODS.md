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
