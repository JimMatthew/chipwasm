TARGET = core.js
SOURCE = chip8.c

all:
	emcc $(SOURCE) -o $(TARGET) \
	  -s MODULARIZE=1 \
	  -s EXPORT_ES6=1 \
	  -s EXPORT_NAME=createModule \
	  -s EXPORTED_FUNCTIONS='["_init","_cycle", "_tick", "_loadROM","_malloc","_free","_pressKey","_releaseKey","_getDisplay", "_reload", "_pauseChip", "_isDisplayUpdated", "_resetDisplayFlag"]' \
	  -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall"]' \
	  -O2

clean:
	rm -f core.js core.wasm core.wasm.map