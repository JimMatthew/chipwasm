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
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
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

void loadFont() {
    for (int i = 0; i < 80; ++i) {
        chip8.memory[0x050 + i] = fontset[i];
    }
}

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
void resetDisplayFlag() {
    chip8.displayUpdate = 0;
}

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
void cycle() {
    if (chip8.isPaused) return;
    uint16_t opcode = chip8.memory[chip8.programCounter] << 8 | chip8.memory[chip8.programCounter + 1];
    chip8.programCounter += 2;
    uint8_t xx = (opcode & 0x0F00) >> 8;
    uint8_t yy = (opcode & 0x00F0) >> 4;
    uint8_t subcode = opcode & 0x000F;
    uint16_t sum = 0;
    uint8_t value = 0;
    uint8_t rnd = 0;
    // switch on first nibble
    switch ((opcode & 0xF000) >> 12) {
    case 0x0:
        if (opcode == 0x00E0) {
            memset(chip8.display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            chip8.displayUpdate = 1;
        } else if (opcode == 0x00EE) {
            chip8.programCounter = chip8.stack[chip8.sp];
            chip8.sp--;
        } else {
            printf("Unknown opcode in 0x 0x%x\n\n", opcode);
            chip8.isPaused = 1;
        }
        break;

    case 0x1: chip8.programCounter = opcode & 0x0FFF; break;
    case 0x2:
        chip8.sp++;
        chip8.stack[chip8.sp] = chip8.programCounter;
        chip8.programCounter = opcode & 0x0FFF;
        break;
    case 0x3:
        if (chip8.registers[xx] == (opcode & 0x00FF)) {
            chip8.programCounter += 2;
        }
        break;
    case 0x4:
        if (chip8.registers[xx] != (opcode & 0x00FF)) {
            chip8.programCounter += 2;
        }
        break;
    case 0x5:
        if (chip8.registers[xx] == chip8.registers[yy]) {
            chip8.programCounter += 2;
        }
        break;
    case 0x6: chip8.registers[xx] = opcode & 0x00FF; break;
    case 0x7: chip8.registers[xx] += (opcode & 0x00FF); break;
    case 0x8:
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
        break;
    case 0x9:
        if (chip8.registers[xx] != chip8.registers[yy]) chip8.programCounter += 2;
        break;
    case 0xA: chip8.indexRegister = opcode & 0x0FFF; break;
    case 0xB:
        if (jumpx) {
            chip8.programCounter = (opcode & 0x0FFF) + chip8.registers[xx]; // Use Vx
        } else {
            chip8.programCounter
                = (opcode & 0x0FFF) + chip8.registers[0]; // Use V0 (standard)
        }
        break;
    case 0xC:
        rnd = rand() % 256;                      // Random byte (0–255)
        chip8.registers[xx] = rnd & (opcode & 0x00FF); // Apply mask
        break;
    case 0xD: {
        int height = opcode & 0x000F;
        uint8_t x = chip8.registers[xx] & 0x3F; // VX % 64
        uint8_t y = chip8.registers[yy] & 0x1F; // VY % 32
        chip8.registers[0xF] = 0;
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
        break;
    }
    case 0xE:
        switch (opcode & 0x00FF) {
        case 0x9E:
            if (chip8.key[chip8.registers[xx]] == 1) chip8.programCounter += 2;
            break;
        case 0xA1:
            if (chip8.key[chip8.registers[xx]] == 0) chip8.programCounter += 2;
            break;
        default: printf("Unknown opcode in 0xE 0x%x\n\n", opcode); chip8.isPaused = 1;
        }
        break;
    case 0xF:
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
        break;

    default: printf("Unknown opcode main 0x%x\n\n", opcode); chip8.isPaused = 1;
    }
}