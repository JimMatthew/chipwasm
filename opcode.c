#include "opcode.h"
#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void code0(uint16_t opcode);
void code8(uint8_t subcode, uint8_t X, uint8_t Y);
void codeD(uint16_t opcode, uint8_t X, uint8_t Y);
void codeE(uint16_t NN, uint8_t X);
void codeF(uint16_t opcode, uint8_t X);

int temp = 0;
int nn = 0;
int kp = -1;
/*
    Main opcode decoder
    This function decodes the opcode and calls the appropriate function
*/
void chip8_decode_and_execute(uint16_t opcode) {

    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t subcode = opcode & 0x000F;
    uint8_t N = (opcode & 0xF000) >> 12;
    uint16_t NN = opcode & 0x00FF;
    int NNN = opcode & 0x0FFF;
    uint16_t sum = 0;
    uint8_t value = 0;
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
    case 0x8: code8(subcode, X, Y); break;
    case 0x9:
        if (chip8.registers[X] != chip8.registers[Y]) chip8.programCounter += 2;
        break;
    case 0xA: chip8.indexRegister = NNN; break;
    case 0xB:
        chip8.programCounter
            = NNN + (chip8.jumpx ? chip8.registers[X] : chip8.registers[0]);
        break;
    case 0xC:
        chip8.registers[X] = (rand() % 256) & NN; // Apply mask
        break;
    case 0xD: codeD(opcode, X, Y); break;
    case 0xE: codeE(NN, X); break;
    case 0xF: codeF(opcode, X); break;
    default: printf("Unknown opcode main 0x%x\n\n", opcode); chip8.isPaused = 1;
    }
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
        width = chip8.hires ? 128 : 64;
        height = chip8.hires ? 64 : 32;
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
        width = chip8.hires ? 128 : 64;
        height = chip8.hires ? 64 : 32;
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
    case 0xFE:  //Set to low-res mode
        chip8.hires = 0;
        chip8.displayUpdate = 1;
        break;
    case 0xFF: // Set to high-res mode
        chip8.hires = 1;
        chip8.displayUpdate = 1;
        break;
    default:
        if ((opcode & 0xF000) == 0x0000 && (opcode & 0xF0F0) == 0x00C0) {
            int width = chip8.hires ? 128 : 64;
            int height = chip8.hires ? 64 : 32;
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

void code8(uint8_t subcode, uint8_t X, uint8_t Y) {
    uint16_t sum = 0;
    switch (subcode) {
    case 0x0: chip8.registers[X] = chip8.registers[Y]; break;
    case 0x1:
        chip8.registers[X] |= chip8.registers[Y];
        if (chip8.vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x2:
        chip8.registers[X] &= chip8.registers[Y];
        if (chip8.vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x3:
        chip8.registers[X] ^= chip8.registers[Y];
        if (chip8.vfReset) chip8.registers[0xF] = 0;
        break;
    case 0x4:
        sum = chip8.registers[X] + chip8.registers[Y];
        chip8.registers[X] = sum & 0xFF;
        chip8.registers[0xF] = (sum > 0xFF) ? 1 : 0;
        break;
    case 0x5:
        temp = chip8.registers[X] < chip8.registers[Y] ? 0 : 1;
        chip8.registers[X] -= chip8.registers[Y];
        chip8.registers[0xF] = temp;
        break;
    case 0x6:
        if (chip8.setXOnShift) chip8.registers[X] = chip8.registers[Y];
        temp = chip8.registers[X] & 0x1;
        chip8.registers[X] >>= 1;
        chip8.registers[0xF] = temp;
        break;
    case 0x7:
        temp = chip8.registers[X] > chip8.registers[Y] ? 0 : 1;
        chip8.registers[X] = chip8.registers[Y] - chip8.registers[X];
        chip8.registers[0xF] = temp;
        break;
    case 0xE:
        if (chip8.setXOnShift) chip8.registers[X] = chip8.registers[Y];
        temp = (chip8.registers[X] & 0x80) >> 7;
        chip8.registers[X] <<= 1;
        chip8.registers[0xF] = temp;
        break;
    }
}

void codeE(uint16_t NN, uint8_t X) {
    switch (NN) {
    case 0x9E:
        if (chip8.key[chip8.registers[X]] == 1) chip8.programCounter += 2;
        break;
    case 0xA1:
        if (chip8.key[chip8.registers[X]] == 0) chip8.programCounter += 2;
        break;
    default: printf("Unknown opcode in E 0x%x\n\n", NN); chip8.isPaused = 1;
    }
}

void codeF(uint16_t opcode, uint8_t X) {
    uint8_t value = 0;
    switch (opcode & 0x00FF) {
    case 0x07: chip8.registers[X] = chip8.delayTimer; break;
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
                chip8.registers[X] = i;
                break;
            }
        }
        if (!nn) {
            chip8.programCounter -= 2;
        }
        nn = 0;
        break;
    case 0x15: chip8.delayTimer = chip8.registers[X]; break;
    case 0x18: chip8.soundTimer = chip8.registers[X]; break;
    case 0x1E: chip8.indexRegister += chip8.registers[X]; break;
    case 0x29: chip8.indexRegister = 0x050 + (chip8.registers[X] * 5); break;
    case 0x33:
        value = chip8.registers[X];
        chip8.memory[chip8.indexRegister] = value / 100;
        chip8.memory[chip8.indexRegister + 1] = (value / 10) % 10;
        chip8.memory[chip8.indexRegister + 2] = value % 10;
        break;
    case 0x55:
        for (int i = 0; i <= X; i++) {
            chip8.memory[chip8.indexRegister + i] = chip8.registers[i];
        }
        if (chip8.memoryInc) chip8.indexRegister += X + 1;
        break;
    case 0x65:
        for (int i = 0; i <= X; i++) {
            chip8.registers[i] = chip8.memory[chip8.indexRegister + i];
        }
        if (chip8.memoryInc) chip8.indexRegister += X + 1;
        break;
    default: printf("Unknown opcode in F 0x%x\n\n", opcode); chip8.isPaused = 1;
    }
}

void codeD(uint16_t opcode, uint8_t X, uint8_t Y) {
    int height = opcode & 0x000F;
    uint16_t screenWidth = chip8.hires ? 128 : 64;
    uint16_t screenHeight = chip8.hires ? 64 : 32;

    uint8_t x = chip8.registers[X] % screenWidth;
    uint8_t y = chip8.registers[Y] % screenHeight;
    uint8_t width = 8;

    chip8.registers[0xF] = 0;

    if (height == 0) {
        // Super-CHIP mode: 16×16 sprite
        height = 16;
        width = 16;
    }

    for (int row = 0; row < height; row++) {
        if (y + row >= screenHeight) break; // Prevent drawing outside screen
        uint16_t spriteRow;

        if (width == 8) {
            spriteRow = chip8.memory[chip8.indexRegister + row];
        } else {
            spriteRow = (chip8.memory[chip8.indexRegister + row * 2] << 8)
                        | chip8.memory[chip8.indexRegister + row * 2 + 1];
        }

        for (int col = 0; col < width; col++) {
            if (x + col >= screenWidth) break; // Prevent drawing outside screen
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
