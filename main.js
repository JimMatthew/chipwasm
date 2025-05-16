import createModule from './core.js';

createModule().then((Module) => {
    const init = Module.cwrap('chip8_init_emscripten', 'void', []);
    const cycle = Module.cwrap('chip8_cycle_emscripten', 'void', []);
    const load_program = Module.cwrap('chip8_load_rom_emscripten', 'void', ['number', 'number']);
    const pressKey = Module.cwrap('chip8_key_press_emscripten', 'void', ['number']);
    const releaseKey = Module.cwrap('chip8_key_release_emscripten', 'void', ['number']);
    const getDisplayPtr = Module.cwrap('chip8_get_display_emscripten', 'number', []);
    const reset = Module.cwrap('chip8_reload_emscripten', 'void', []);
    const pauseChip = Module.cwrap('chip8_pause_emscripten', 'void', []);
    const tick = Module.cwrap('chip8_tick_emscripten', 'void', []);
    const isDisplayUpdated = Module.cwrap('chip8_is_display_updated_emscripten', 'number', []);
    const isHires = Module.cwrap('chip8_is_hires_emscripten', 'number', []);
    const setMode = Module.cwrap('chip8_set_mode_emscripten', 'void', ['number']);
    let hires = 0;
    let opsPerFrame = 10;
    init();

    const canvas = document.getElementById("screen");
    const ctx = canvas.getContext("2d");
    let scale = 10;
    const display = new Uint8Array(Module.HEAPU8.buffer, getDisplayPtr(), 128 * 64);
    let prevDisplay = new Uint8Array(128 * 64);

    function drawDisplay(forceRedraw = 0) {
        const width = isHires() ? 128 : 64;
        const height = isHires() ? 64 : 32;
    
        if (hires !== isHires()) {
            hires = isHires();
            updateCanvasSize();
            ctx.clearRect(0, 0, canvas.width, canvas.height);
        }
        if (forceRedraw) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.fillStyle = "#FFFFFF";
            for (let y = 0; y < height; y++) {
                for (let x = 0; x < width; x++) {
                    if (display[y * width + x]) {
                        ctx.fillRect(x * scale, y * scale, scale, scale);
                    }
                }
            }
            return;
        }
        if (isDisplayUpdated() === 0) return;

        for (let y = 0; y < height; y++) {
            for (let x = 0; x < width; x++) {
                const i = y * width + x;
                const current = display[i];
                const previous = prevDisplay[i];
                if (current !== previous) {
                    ctx.fillStyle = current ? "#FFFFFF" : "#000000";
                    ctx.fillRect(x * scale, y * scale, scale, scale);
                    prevDisplay[i] = current;
                }
            }
        }
    }

    let emulationStarted = false;

    function startEmulation() {
        if (emulationStarted) return;
        emulationStarted = true;

        setInterval(() => {
            for (let i = 0; i < opsPerFrame; i++) {
                cycle();
            }
            drawDisplay();
            tick();
        }, 1000 / 60);
    }
    
    const updateCanvasSize = () => {
        const width = isHires() ? 128 : 64;
        const height = isHires() ? 64 : 32;
        scale = isHires() ? 5 : 10;
        canvas.width = width * scale;
        canvas.height = height * scale;
        prevDisplay = new Uint8Array(width * height); 
    }

    document.getElementById("romLoader").onchange = (e) => {
        const file = e.target.files[0];
        const reader = new FileReader();
        reader.onload = function () {
            const bytes = new Uint8Array(reader.result);
            const buf = Module._malloc(bytes.length);
            Module.HEAPU8.set(bytes, buf);
            load_program(buf, bytes.length);
            Module._free(buf);
            startEmulation();
        };
        reader.readAsArrayBuffer(file);
    };

    const keyMap = {
        '1': 0x1, '2': 0x2, '3': 0x3, '4': 0xC,
        'q': 0x4, 'w': 0x5, 'e': 0x6, 'r': 0xD,
        'a': 0x7, 's': 0x8, 'd': 0x9, 'f': 0xE,
        'z': 0xA, 'x': 0x0, 'c': 0xB, 'v': 0xF
    };

    window.addEventListener('keydown', (e) => {
        const key = keyMap[e.key.toLowerCase()];
        if (key !== undefined) pressKey(key);
    });

    window.addEventListener('keyup', (e) => {
        const key = keyMap[e.key.toLowerCase()];
        if (key !== undefined) releaseKey(key);
    });

    document.getElementById("resetButton").addEventListener("click", () => {
        reset();
        prevDisplay.fill(0);
        drawDisplay(1);
    });

    document.getElementById("pauseButton").addEventListener("click", () => {
        pauseChip();
    });

    document.getElementById("romSelect").onchange = (e) => {
        const url = e.target.value;
        if (!url) return;

        fetch(url)
            .then(res => res.arrayBuffer())
            .then(buffer => {
                const bytes = new Uint8Array(buffer);
                const buf = Module._malloc(bytes.length);
                Module.HEAPU8.set(bytes, buf);
                load_program(buf, bytes.length);
                startEmulation();
            });
    };

    document.getElementById('opsSlider').addEventListener('input', (e) => {
        opsPerFrame = parseInt(e.target.value, 10);
    });

    document.getElementById('schipToggle').addEventListener('change', (e) => {
        const enabled = e.target.checked ? 1 : 0;
        setMode(enabled);
        updateCanvasSize();     
        drawDisplay(1);         
    });

});