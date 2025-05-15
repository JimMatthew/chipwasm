/*
    EmScripten bindings for Chip8 emulator
    This file contains the bindings for the Chip8 emulator to be used with
    EmScripten. It provides functions to initialize the emulator, load ROMs, and
    handle input/output.
*/

#include "chip8.h"
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
void chip8_init_emscripten() { chip8Init(); }

EMSCRIPTEN_KEEPALIVE
void chip8_reload_emscripten() { reload(); }

EMSCRIPTEN_KEEPALIVE
void chip8_load_rom_emscripten(uint8_t *data, int length) {
    loadROM(data, length);
}

EMSCRIPTEN_KEEPALIVE
void chip8_tick_emscripten() { chip8Tick(); }

EMSCRIPTEN_KEEPALIVE
void chip8_cycle_emscripten() { chip8Cycle(); }

EMSCRIPTEN_KEEPALIVE
int chip8_is_display_updated_emscripten() { return isDisplayUpdated(); }

EMSCRIPTEN_KEEPALIVE
uint8_t *chip8_get_display_emscripten() { return getDisplay(); }

EMSCRIPTEN_KEEPALIVE
void chip8_key_press_emscripten(int k) { chip8_press_key(k); }

EMSCRIPTEN_KEEPALIVE
void chip8_key_release_emscripten(int k) { chip8_release_key(k); }

EMSCRIPTEN_KEEPALIVE
void chip8_set_mode_emscripten(int mode) { setMode(mode); }

EMSCRIPTEN_KEEPALIVE
int chip8_is_hires_emscripten() { return isHiresMode(); }

EMSCRIPTEN_KEEPALIVE
void chip8_pause_emscripten() { pauseChip(); }