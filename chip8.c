#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "chip8.h"
#include "opcode.h"

struct chip8 chip8;

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

//Set quirks based on mode
// 0: Standard mode
// 1: Super-CHIP mode
void setMode(int mode) {
    if (mode == 0) {
        chip8.hires = 0;
        chip8.setXOnShift = 1;
        chip8.vfReset = 1;
        chip8.memoryInc = 1;
        chip8.jumpx = 0;
        chip8.displayUpdate = 1;
    } else if (mode == 1) {
        chip8.setXOnShift = 0;
        chip8.vfReset = 0;
        chip8.memoryInc = 0;
        chip8.jumpx = 1;
        chip8.displayUpdate = 1;
    } else {
        printf("Unknown mode %d\n", mode);
    }
}

void chip8Init() {
    chip8.programCounter = 0x200;
    loadFont();
    srand(time(NULL));
    setMode(0);
}

uint8_t *getDisplay() { return chip8.display; }

int isDisplayUpdated() {
    if (chip8.displayUpdate) {
        chip8.displayUpdate = 0;
        return 1;
    }
    return 0;
}

void resetDisplayFlag() { chip8.displayUpdate = 0; }

void chip8_press_key(int key) {
    if (key >= 0 && key < KEY_SIZE)
        chip8.key[key] = 1;
}

void chip8_release_key(int key) {
    if (key >= 0 && key < KEY_SIZE)
        chip8.key[key] = 0;
}

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

void loadROM(uint8_t *data, int length) {
    memcpy(&chip8.program, data, length);
    chip8.programSize = length;
    reload();
}

void pauseChip() { chip8.isPaused = chip8.isPaused ? 0 : 1; }

void chip8Tick() {
    if (chip8.delayTimer > 0) chip8.delayTimer--;
}

int isHiresMode() { return chip8.hires; }

void chip8Cycle() {
    if (chip8.isPaused) return;
    uint16_t opcode = chip8.memory[chip8.programCounter] << 8
                      | chip8.memory[chip8.programCounter + 1];
    chip8.programCounter += 2;
    chip8_decode_and_execute(opcode);
}