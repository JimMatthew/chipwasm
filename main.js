import createModule from './core.js';

createModule().then((Module) => {
    const init = Module.cwrap('init', 'void', []);
    const cycle = Module.cwrap('cycle', 'void', []);
    const load_program = Module.cwrap('loadROM', 'void', ['number', 'number']);
    const pressKey = Module.cwrap('pressKey', 'void', ['number']);
    const releaseKey = Module.cwrap('releaseKey', 'void', ['number']);
    const getDisplayPtr = Module.cwrap('getDisplay', 'number', []);
    const reset = Module.cwrap('reload', 'void', []);
    const pauseChip = Module.cwrap('pauseChip', 'void', []);
    const tick = Module.cwrap('tick', 'void', []);
    const isDisplayUpdated = Module.cwrap('isDisplayUpdated', 'number', []);
    const resetDisplayFlag = Module.cwrap('resetDisplayFlag', 'void', []);
    let opsPerFrame = 10;
    init();

    const canvas = document.getElementById("screen");
    const ctx = canvas.getContext("2d");
    const scale = 10;
    const display = new Uint8Array(Module.HEAPU8.buffer, getDisplayPtr(), 64 * 32);
    const prevDisplay = new Uint8Array(64 * 32);

    function drawDisplay(forceRedraw = 0) {
        if (forceRedraw) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.fillStyle = "#FFFFFF";
            for (let y = 0; y < 32; y++) {
                for (let x = 0; x < 64; x++) {
                    if (display[y * 64 + x]) {
                        ctx.fillRect(x * scale, y * scale, scale, scale);
                    }
                }
            }
            resetDisplayFlag();
            return;
        }
        if (isDisplayUpdated() === 0) return;

        for (let y = 0; y < 32; y++) {
            for (let x = 0; x < 64; x++) {
                const i = y * 64 + x;
                const current = display[i];
                const previous = prevDisplay[i];

                if (current !== previous) {
                    if (current) {
                        ctx.fillStyle = "#FFFFFF";
                        ctx.fillRect(x * scale, y * scale, scale, scale);
                    } else {
                        ctx.fillStyle = "#000000";
                        ctx.fillRect(x * scale, y * scale, scale, scale);
                    }
                    prevDisplay[i] = current;
                }
            }
        }
        resetDisplayFlag();
    }
    function drawDisplay5() {
        if (isDisplayUpdated() === 0) return;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = "#FFFFFF";

        for (let y = 0; y < 32; y++) {
            for (let x = 0; x < 64; x++) {
                if (display[y * 64 + x]) {
                    ctx.fillRect(x * scale, y * scale, scale, scale);
                }
            }
        }
        resetDisplayFlag();
    }

    let emulationStarted = false;

    function startEmulation() {
        if (emulationStarted) return;
        emulationStarted = true;

        setInterval(() => {
            for (let i = 0; i < opsPerFrame; i++) {
                console.log("Cycle: " + i);
                cycle();
            }
            drawDisplay();
            tick();
        }, 1000 / 60);
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

    
});