TARGET = core.js
SOURCE = chip8.c opcode.c main.c

all:
	emcc $(SOURCE) -o $(TARGET) \
	  -s MODULARIZE=1 \
	  -s EXPORT_ES6=1 \
	  -s EXPORT_NAME=createModule \
	  -s EXPORTED_FUNCTIONS='["_chip8_init_emscripten","_chip8_cycle_emscripten", "_chip8_tick_emscripten", "_chip8_set_mode_emscripten","_chip8_is_hires_emscripten", "_chip8_load_rom_emscripten","_malloc","_free","_chip8_key_press_emscripten","_chip8_key_release_emscripten","_chip8_get_display_emscripten", "_chip8_reload_emscripten", "_chip8_pause_emscripten", "_chip8_is_display_updated_emscripten"]' \
	  -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
	  -O2

clean:
	rm -f core.js core.wasm core.wasm.map