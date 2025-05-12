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

uint8_t memory[MEM_SIZE];
uint8_t program[MEM_SIZE];
uint8_t registers[NUM_REGISTERS];
uint16_t stack[STACK_SIZE];
uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
uint8_t key[NUM_KEYS];

uint8_t sp = 0; // stack pointer
uint16_t programCounter = 0;
uint16_t indexRegister = 0;
uint8_t delayTimer = 0;
uint8_t soundTimer = 0;

int isPaused = 0;
int programSize = -1;
int displayUpdate = 0;

//Quirck settings for chip8 behavior
int setXOnShift = 1; // Shift opcode moves VY to VX
int vfReset = 1;     // AND OR and XOR reset VF to 0
int memoryInc = 1;   // increment index register after store/load
int jumpx = 0;       // Jump opcode uses Vx instead of V0
int clip = 0;

//temp variables
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
        memory[0x050 + i] = fontset[i];
    }
}

EMSCRIPTEN_KEEPALIVE
void init() {
    programCounter = 0x200;
    loadFont();
    srand(time(NULL));
}

EMSCRIPTEN_KEEPALIVE
uint8_t *getDisplay() { return display; }

EMSCRIPTEN_KEEPALIVE
void pressKey(int k) {
    if (k >= 0 && k < 16) key[k] = 1;
}

EMSCRIPTEN_KEEPALIVE
void releaseKey(int k) {
    if (k >= 0 && k < 16) key[k] = 0;
}

EMSCRIPTEN_KEEPALIVE
void reload() {
    memset(&memory, 0, MEM_SIZE);
    memset(&display, 0, DISPLAY_HEIGHT * DISPLAY_WIDTH);
    memset(registers, 0, NUM_REGISTERS);
    loadFont();
    memcpy(&memory[0x200], &program, programSize);
    programCounter = 0x200;
    indexRegister = 0;
    delayTimer = 0;
    sp = 0;
}

EMSCRIPTEN_KEEPALIVE
void loadROM(uint8_t *data, int length) {
    memcpy(&program, data, length);
    programSize = length;
    reload();
}

EMSCRIPTEN_KEEPALIVE
void pauseChip() { isPaused = isPaused ? 0 : 1; }

EMSCRIPTEN_KEEPALIVE
void tick() {
    if (delayTimer > 0) delayTimer--;
}

EMSCRIPTEN_KEEPALIVE
void cycle() {
    if (isPaused) return;
    uint16_t opcode = memory[programCounter] << 8 | memory[programCounter + 1];
    programCounter += 2;
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
            memset(display, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT);
            displayUpdate = 1;
        } else if (opcode == 0x00EE) {
            programCounter = stack[sp];
            sp--;
        } else {
            printf("Unknown opcode in 0x 0x%x\n\n", opcode);
            isPaused = 1;
        }
        break;

    case 0x1: programCounter = opcode & 0x0FFF; break;
    case 0x2:
        sp++;
        stack[sp] = programCounter;
        programCounter = opcode & 0x0FFF;
        break;
    case 0x3:
        if (registers[xx] == (opcode & 0x00FF)) {
            programCounter += 2;
        }
        break;
    case 0x4:
        if (registers[xx] != (opcode & 0x00FF)) {
            programCounter += 2;
        }
        break;
    case 0x5:
        if (registers[xx] == registers[yy]) {
            programCounter += 2;
        }
        break;
    case 0x6: registers[xx] = opcode & 0x00FF; break;
    case 0x7: registers[xx] += (opcode & 0x00FF); break;
    case 0x8:
        switch (subcode) {
        case 0x0: registers[xx] = registers[yy]; break;
        case 0x1:
            registers[xx] |= registers[yy];
            if (vfReset) registers[0xF] = 0;
            break;
        case 0x2:
            registers[xx] &= registers[yy];
            if (vfReset) registers[0xF] = 0;
            break;
        case 0x3:
            registers[xx] ^= registers[yy];
            if (vfReset) registers[0xF] = 0;
            break;
        case 0x4:
            sum = registers[xx] + registers[yy];
            registers[xx] = sum & 0xFF;
            registers[0xF] = (sum > 0xFF) ? 1 : 0;
            break;
        case 0x5:
            temp = registers[xx] < registers[yy] ? 0 : 1;
            registers[xx] -= registers[yy];
            registers[0xF] = temp;
            break;
        case 0x6:
            if (setXOnShift) registers[xx] = registers[yy];
            temp = registers[xx] & 0x1;
            registers[xx] >>= 1;
            registers[0xF] = temp;
            break;
        case 0x7:
            temp = registers[xx] > registers[yy] ? 0 : 1;
            registers[xx] = registers[yy] - registers[xx];
            registers[0xF] = temp;
            break;
        case 0xE:
            if (setXOnShift) registers[xx] = registers[yy];
            temp = (registers[xx] & 0x80) >> 7;
            registers[xx] <<= 1;
            registers[0xF] = temp;
            break;
        }
        break;
    case 0x9:
        if (registers[xx] != registers[yy]) programCounter += 2;
        break;
    case 0xA: indexRegister = opcode & 0x0FFF; break;
    case 0xB:
        if (jumpx) {
            programCounter = (opcode & 0x0FFF) + registers[xx]; // Use Vx
        } else {
            programCounter
                = (opcode & 0x0FFF) + registers[0]; // Use V0 (standard)
        }
        break;
    case 0xC:
        rnd = rand() % 256;                      // Random byte (0–255)
        registers[xx] = rnd & (opcode & 0x00FF); // Apply mask
        break;
    case 0xD: {
        int height = opcode & 0x000F;
        uint8_t x = registers[xx] & 0x3F; // VX % 64
        uint8_t y = registers[yy] & 0x1F; // VY % 32
        registers[0xF] = 0;

        for (int row = 0; row < height; row++) {
            if (y + row >= DISPLAY_HEIGHT) break; // Clip vertically

            uint8_t spriteByte = memory[indexRegister + row];

            for (int col = 0; col < 8; col++) {
                if (x + col >= DISPLAY_WIDTH) break; // Clip horizontally

                uint8_t spritePixel = (spriteByte >> (7 - col)) & 0x1;
                int index = (y + row) * DISPLAY_WIDTH + (x + col);

                if (spritePixel) {
                    if (display[index]) registers[0xF] = 1;
                    display[index] ^= 1;
                }
            }
        }

        displayUpdate = 1;
        break;
    }
    case 0xE:
        switch (opcode & 0x00FF) {
        case 0x9E:
            if (key[registers[xx]] == 1) programCounter += 2;
            break;
        case 0xA1:
            if (key[registers[xx]] == 0) programCounter += 2;
            break;
        default: printf("Unknown opcode in 0xE 0x%x\n\n", opcode); isPaused = 1;
        }
        break;
    case 0xF:
        switch (opcode & 0x00FF) {
        case 0x07: registers[xx] = delayTimer; break;
        case 0x0A:
            for (int i = 0; i < 16; i++) {
                if (kp != -1) {
                    if (key[kp] != 1) {
                        nn = 1;
                        kp = -1;
                    }
                    break;
                }
                if (key[i] == 1) {
                    kp = i;
                    registers[xx] = i;
                    break;
                }
            }
            if (!nn) {
                programCounter -= 2;
            }
            nn = 0;
            break;
        case 0x15: delayTimer = registers[xx]; break;
        case 0x18: soundTimer = registers[xx]; break;
        case 0x1E: indexRegister += registers[xx]; break;
        case 0x29: indexRegister = 0x050 + (registers[xx] * 5); break;
        case 0x33:
            value = registers[xx];
            memory[indexRegister] = value / 100;
            memory[indexRegister + 1] = (value / 10) % 10;
            memory[indexRegister + 2] = value % 10;
            break;
        case 0x55:
            for (int i = 0; i <= (xx); i++) {
                memory[indexRegister + i] = registers[i];
            }
            if (memoryInc) indexRegister += xx + 1;
            break;
        case 0x65:
            for (int i = 0; i <= (xx); i++) {
                registers[i] = memory[indexRegister + i];
            }
            if (memoryInc) indexRegister += xx + 1;
            break;
        default: printf("Unknown opcode in F 0x%x\n\n", opcode); isPaused = 1;
        }
        break;

    default: printf("Unknown opcode main 0x%x\n\n", opcode); isPaused = 1;
    }
}