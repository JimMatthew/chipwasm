#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define MEM_SIZE 4096
#define KEY_SIZE 16
#define NUM_REGISTERS 16
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define STACK_SIZE 16
#define NUM_KEYS 16
#define PIXEL_SIZE 10

struct chip8 {
    uint8_t memory[MEM_SIZE];
    uint8_t program[MEM_SIZE];
    uint8_t registers[NUM_REGISTERS];
    uint16_t stack[STACK_SIZE];
    uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    uint8_t key[NUM_KEYS];
    uint8_t sp; // stack pointer
    uint16_t programCounter;
    uint16_t indexRegister;
    uint8_t delayTimer;
    uint8_t soundTimer;

    int isPaused;
    int programSize;
    int displayUpdate;
};

struct chip8 chip8;
int hires = 0;

// Quirk settings for chip8 behavior
int setXOnShift = 1; // Shift opcode moves VY to VX
int vfReset = 1;     // AND OR and XOR reset VF to 0
int memoryInc = 1;   // increment index register after store/load
int jumpx = 0;       // Jump opcode uses Vx instead of V0
int clip = 0;

// temp variables
int temp = 0;
int nn = 0;
int kp = -1;

uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void loadFont() { memcpy(&chip8.memory[0x050], fontset, 80); }

EMSCRIPTEN_KEEPALIVE
void init() {
    chip8.programCounter = 0x200;
    loadFont();
    srand(time(NULL));
}

EMSCRIPTEN_KEEPALIVE
uint8_t *getDisplay() { return chip8.display; }

EMSCRIPTEN_KEEPALIVE
int isDisplayUpdated() {
    if (chip8.displayUpdate) {
        chip8.displayUpdate = 0;
        return 1;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void resetDisplayFlag() { chip8.displayUpdate = 0; }

EMSCRIPTEN_KEEPALIVE
void pressKey(int k) {
    if (k >= 0 && k < 16) chip8.key[k] = 1;
}

EMSCRIPTEN_KEEPALIVE
void releaseKey(int k) {
    if (k >= 0 && k < 16) chip8.key[k] = 0;
}

EMSCRIPTEN_KEEPALIVE
void reload() {
    memset(chip8.memory, 0, sizeof(chip8.memory));
    memset(chip8.display, 0, sizeof(chip8.display));
    memset(chip8.registers, 0, sizeof(chip8.registers));
    memset(chip8.stack, 0, sizeof(chip8.stack));
    loadFont();
    if (chip8.programSize > 0 && chip8.programSize <= (MEM_SIZE - 0x200)) {
        memcpy(&chip8.memory[0x200], chip8.program, chip8.programSize);
    }
    chip8.programCounter = 0x200;
    chip8.indexRegister = 0;
    chip8.delayTimer = 0;
    chip8.soundTimer = 0;
    chip8.sp = 0;
    chip8.displayUpdate = 1;
}

EMSCRIPTEN_KEEPALIVE
void loadROM(uint8_t *data, int length) {
    memcpy(&chip8.program, data, length);
    chip8.programSize = length;
    reload();
}

EMSCRIPTEN_KEEPALIVE
void pauseChip() { chip8.isPaused = chip8.isPaused ? 0 : 1; }

EMSCRIPTEN_KEEPALIVE
void tick() {
    if (chip8.delayTimer > 0) chip8.delayTimer--;
}
EMSCRIPTEN_KEEPALIVE
int isHiresMode() {
    return hires;
}

void arithmetic(uint8_t subcode, uint8_t xx, uint8_t yy) {
    uint16_t sum = 0;
    switch (subcode) {
    case 0x0: chip8.registers[xx] = chip8.registers[yy]; break;
    case 0x1:
        chip8.registers[xx] |= chip8.registers[yy];
        if (vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x2:
        chip8.registers[xx] &= chip8.registers[yy];
        if (vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x3:
        chip8.registers[xx] ^= chip8.registers[yy];
        if (vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x4:
        sum = chip8.registers[xx] + chip8.registers[yy];
        chip8.registers[xx] = sum & 0xFF;
        chip8.registers[0xF] = (sum > 0xFF) ? 1 : 0;
        break;
    case 0x5:
        temp = chip8.registers[xx] < chip8.registers[yy] ? 0 : 1;
        chip8.registers[xx] -= chip8.registers[yy];
        chip8.registers[0xF] = temp;
        break;
    case 0x6:
        if (setXOnShift) chip8.registers[xx] = chip8.registers[yy];
        temp = chip8.registers[xx] & 0x1;
        chip8.registers[xx] >>= 1;
        chip8.registers[0xF] = temp;
        break;
    case 0x7:
        temp = chip8.registers[xx] > chip8.registers[yy] ? 0 : 1;
        chip8.registers[xx] = chip8.registers[yy] - chip8.registers[xx];
        chip8.registers[0xF] = temp;
        break;
    case 0xE:
        if (setXOnShift) chip8.registers[xx] = chip8.registers[yy];
        temp = (chip8.registers[xx] & 0x80) >> 7;
        chip8.registers[xx] <<= 1;
        chip8.registers[0xF] = temp;
        break;
    }
}

void Fcode(uint16_t opcode, uint8_t xx) {
    uint8_t value = 0;
    switch (opcode & 0x00FF) {
    case 0x07: chip8.registers[xx] = chip8.delayTimer; break;
    case 0x0A:
        for (int i = 0; i < 16; i++) {
            if (kp != -1) {
                if (chip8.key[kp] != 1) {
                    nn = 1;
                    kp = -1;
                }
                break;
            }
            if (chip8.key[i] == 1) {
                kp = i;
                chip8.registers[xx] = i;
                break;
            }
        }
        if (!nn) {
            chip8.programCounter -= 2;
        }
        nn = 0;
        break;
    case 0x15: chip8.delayTimer = chip8.registers[xx]; break;
    case 0x18: chip8.soundTimer = chip8.registers[xx]; break;
    case 0x1E: chip8.indexRegister += chip8.registers[xx]; break;
    case 0x29: chip8.indexRegister = 0x050 + (chip8.registers[xx] * 5); break;
    case 0x33:
        value = chip8.registers[xx];
        chip8.memory[chip8.indexRegister] = value / 100;
        chip8.memory[chip8.indexRegister + 1] = (value / 10) % 10;
        chip8.memory[chip8.indexRegister + 2] = value % 10;
        break;
    case 0x55:
        for (int i = 0; i <= (xx); i++) {
            chip8.memory[chip8.indexRegister + i] = chip8.registers[i];
        }
        if (memoryInc) chip8.indexRegister += xx + 1;
        break;
    case 0x65:
        for (int i = 0; i <= (xx); i++) {
            chip8.registers[i] = chip8.memory[chip8.indexRegister + i];
        }
        if (memoryInc) chip8.indexRegister += xx + 1;
        break;
    default: printf("Unknown opcode in F 0x%x\n\n", opcode); chip8.isPaused = 1;
    }
}

void Ecode(uint16_t NN, uint8_t xx) {
    switch (NN) {
    case 0x9E:
        if (chip8.key[chip8.registers[xx]] == 1) chip8.programCounter += 2;
        break;
    case 0xA1:
        if (chip8.key[chip8.registers[xx]] == 0) chip8.programCounter += 2;
        break;
    default: printf("Unknown opcode in E 0x%x\n\n", NN); chip8.isPaused = 1;
    }
}

EMSCRIPTEN_KEEPALIVE
void setMode(int mode) {
    if (mode == 0) {
        hires = 0;
        setXOnShift = 1; 
        vfReset = 1;     
        memoryInc = 1;   
        jumpx = 0;       
        chip8.displayUpdate = 1;
    } else if (mode == 1) {
        setXOnShift = 0;
        vfReset = 0;
        memoryInc = 0;
        jumpx = 1;
        chip8.displayUpdate = 1;
    } else {
        printf("Unknown mode %d\n", mode);
    }
}

void displayCode(uint16_t opcode, uint8_t xx, uint8_t yy) {
    int height = opcode & 0x000F;
    uint8_t x = chip8.registers[xx];
    uint8_t y = chip8.registers[yy];
    uint8_t width = 8;

    chip8.registers[0xF] = 0;

    if (height == 0) {
        // Super-CHIP mode: 16×16 sprite
        height = 16;
        width = 16;
    }

    // Use current display resolution
    uint16_t screenWidth = hires ? 128 : 64;
    uint16_t screenHeight = hires ? 64 : 32;

    for (int row = 0; row < height; row++) {
        uint16_t spriteRow;

        if (width == 8) {
            spriteRow = chip8.memory[chip8.indexRegister + row];
        } else {
            spriteRow = (chip8.memory[chip8.indexRegister + row * 2] << 8) |
                         chip8.memory[chip8.indexRegister + row * 2 + 1];
        }

        for (int col = 0; col < width; col++) {
            uint8_t spritePixel = (spriteRow >> (width - 1 - col)) & 0x1;
            uint8_t screenX = (x + col) % screenWidth;
            uint8_t screenY = (y + row) % screenHeight;
            int index = screenY * screenWidth + screenX;

            if (spritePixel) {
                if (chip8.display[index]) chip8.registers[0xF] = 1;
                chip8.display[index] ^= 1;
            }
        }
    }

    chip8.displayUpdate = 1;
}

void displayCode2(uint16_t opcode, uint8_t xx, uint8_t yy) {
    int height = opcode & 0x000F;
    uint8_t x = chip8.registers[xx] & 0x3F; // VX % 64
    uint8_t y = chip8.registers[yy] & 0x1F; // VY % 32
    uint8_t width = 8;
    chip8.registers[0xF] = 0;
    if (height == 0) {
        height = 16;
        width = 16;
    }
    for (int row = 0; row < height; row++) {
        if (y + row >= DISPLAY_HEIGHT) break; // Clip vertically
        uint8_t spriteByte = chip8.memory[chip8.indexRegister + row];
        for (int col = 0; col < 8; col++) {
            if (x + col >= DISPLAY_WIDTH) break; // Clip horizontally
            uint8_t spritePixel = (spriteByte >> (7 - col)) & 0x1;
            int index = (y + row) * DISPLAY_WIDTH + (x + col);
            if (spritePixel) {
                if (chip8.display[index]) chip8.registers[0xF] = 1;
                chip8.display[index] ^= 1;
            }
        }
    }
    chip8.displayUpdate = 1;
}

void code0(uint16_t opcode) {
    int width = 0;
    int height = 0;

    switch (opcode & 0x00FF) {
        case 0xE0: // Clear the display
        memset(chip8.display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
        chip8.displayUpdate = 1;
        break;
    case 0xEE: // Return from subroutine
        chip8.programCounter = chip8.stack[chip8.sp];
        chip8.sp--;
        break;
    case 0xFB:
        width = hires ? 128 : 64;
        height = hires ? 64 : 32;

        for (int y = 0; y < height; y++) {
            for (int x = width - 1; x >= 4; x--) {
                chip8.display[y * width + x]
                    = chip8.display[y * width + (x - 4)];
            }
            // Clear leftmost 4 pixels
            for (int x = 0; x < 4; x++) {
                chip8.display[y * width + x] = 0;
            }
        }
        chip8.displayUpdate = 1;
        break;
    case 0xFC:
        width = hires ? 128 : 64;
        height = hires ? 64 : 32;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width - 4; x++) {
                chip8.display[y * width + x]
                    = chip8.display[y * width + (x + 4)];
            }
            // Clear rightmost 4 pixels
            for (int x = width - 4; x < width; x++) {
                chip8.display[y * width + x] = 0;
            }
        }
        chip8.displayUpdate = 1;
        break;
    case 0xFD: // Pause the emulator
        chip8.isPaused = 1;
        break;
    case 0xFE: // Resume the emulator
        hires = 0;
        chip8.displayUpdate = 1;
        break;
    case 0xFF: // Set the display to low resolution
        hires = 1;
        chip8.displayUpdate = 1;
        break;
    default:
        if ((opcode & 0xF000) == 0x0000 && (opcode & 0xF0F0) == 0x00C0) {
            int width = hires ? 128 : 64;
            int height = hires ? 64 : 32;

            for (int y = height - 1; y >= (opcode & 0x000F); y--) {
                for (int x = 0; x < width; x++) {
                    chip8.display[y * width + x]
                        = chip8.display[(y - (opcode & 0x000F)) * width + x];
                }
            }

            // Clear new lines at top
            for (int y = 0; y < (opcode & 0x000F); y++) {
                for (int x = 0; x < width; x++) {
                    chip8.display[y * width + x] = 0;
                }
            }

            chip8.displayUpdate = 1;
        }
        break;
        printf("Unknown opcode in 0x0 0x%x\n\n", opcode);
        chip8.isPaused = 1;
    }
}

EMSCRIPTEN_KEEPALIVE
void cycle() {
    if (chip8.isPaused) return;
    uint16_t opcode = chip8.memory[chip8.programCounter] << 8
                      | chip8.memory[chip8.programCounter + 1];
    chip8.programCounter += 2;
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t subcode = opcode & 0x000F;
    uint16_t sum = 0;
    uint8_t value = 0;
    uint8_t N = (opcode & 0xF000) >> 12;
    uint16_t NN = opcode & 0x00FF;
    int NNN = opcode & 0x0FFF;

    // switch on first nibble
    switch (N) {
    case 0x0: code0(opcode); break;
    case 0x1: chip8.programCounter = NNN; break;
    case 0x2:
        chip8.sp++;
        chip8.stack[chip8.sp] = chip8.programCounter;
        chip8.programCounter = NNN;
        break;
    case 0x3:
        if (chip8.registers[X] == NN) chip8.programCounter += 2;
        break;
    case 0x4:
        if (chip8.registers[X] != NN) chip8.programCounter += 2;
        break;
    case 0x5:
        if (chip8.registers[X] == chip8.registers[Y]) chip8.programCounter += 2;
        break;
    case 0x6: chip8.registers[X] = NN; break;
    case 0x7: chip8.registers[X] += NN; break;
    case 0x8: arithmetic(subcode, X, Y); break;
    case 0x9:
        if (chip8.registers[X] != chip8.registers[Y]) chip8.programCounter += 2;
        break;
    case 0xA: chip8.indexRegister = NNN; break;
    case 0xB:
           if (jumpx) {
            chip8.programCounter = (opcode & 0x0FFF) + chip8.registers[X]; // Use Vx
        } else {
            chip8.programCounter
                = (opcode & 0x0FFF) + chip8.registers[0]; // Use V0 (standard)
        }
        break;
        break;
    case 0xC:
        chip8.registers[X] = (rand() % 256) & NN; // Apply mask
        break;
    case 0xD: displayCode(opcode, X, Y); break;
    case 0xE: Ecode(NN, X); break;
    case 0xF: Fcode(opcode, X); break;
    default: printf("Unknown opcode main 0x%x\n\n", opcode); chip8.isPaused = 1;
    }
}