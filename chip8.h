/*
    Chip8 Emulator implementation
    This file contains the implementation of the Chip8 emulator. It includes
    functions to initialize the emulator, load ROMs, and handle input/output.
    To use this emulator, include this header file in your project and link
    against the implementation file.
    This emulator is based on the Chip8 specification and is compatible with
    most Chip8 ROMs. It also includes support for Super-CHIP mode, which adds
    additional opcodes and features to the original Chip8 specification.

    To use this emulator, you must implement the event loop and rendering
    logic in your application. You must call the `chip8Tick` and `chip8Cycle` functions
    at the appropriate intervals to update the emulator state and process input.
    The `chip8Tick` function should be called 60 times per second to update the
    timers, while the `chip8Cycle` function should be called N number of times
    per tick. The number of cycles per tick can be adjusted based on the desired
    speed of the emulator. The `chip8Cycle` function will decode and execute
    the next opcode in the program counter. The `chip8Tick` function will update
    the delay and sound timers, and handle any other necessary updates to the
    emulator state.
    The emulator also includes a display buffer that can be accessed using the
    `getDisplay` function. This buffer contains the pixel data for the current
    frame, and can be used to render the display in your application. The
    `isDisplayUpdated` function can be used to check if the display has been    
    updated since the last frame. If the display has been updated, you should
    render the display using the data in the display buffer. The `chip8_press_key`
    and `chip8_release_key` functions can be used to handle input from the
    keyboard. These functions should be called when a key is pressed or released,
    and will update the internal state of the emulator accordingly. The
    `setMode` function can be used to switch between standard Chip8 mode and
    Super-CHIP mode. The emulator will automatically adjust its behavior based
    on the selected mode. The `isHiresMode` function can be used to check if
    the emulator is currently in high-resolution mode. The `pauseChip` function
    can be used to pause or unpause the emulator. When the emulator is paused,
    the `chip8Cycle` function will not execute any opcodes, and the emulator
    state will not be updated. 
*/

#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

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
    int setXOnShift; // Shift opcode moves VY to VX
    int vfReset;     // AND OR and XOR reset VF to 0
    int memoryInc;   // increment index register after store/load
    int jumpx;       // Jump opcode uses Vx instead of V0
    int clip;
    int hires;
};

extern struct chip8 chip8;

// Initialize the Chip8 emulator
// This function should be called before any other functions
void chip8Init();

//Load a ROM into the emulator
void loadROM(uint8_t *data, int length);

// Reset the emulator state
// This will reload the last loaded ROM
void reload();

// Call tick 60 times per second
void chip8Tick();

// Call cycle N times per tick
void chip8Cycle();

/*
    Set mode for the chip8 emulator.
    0: Standard mode
    1: Super-CHIP mode
*/
void setMode(int mode);

void chip8_press_key(int key);

void chip8_release_key(int key);

// is the display in high-res mode
// 0: low-res mode
// 1: high-res mode
int isHiresMode();

// returns a pointer to the display buffer
//The display buffer is of size DISPLAY_WIDTH * DISPLAY_HEIGHT
//you must check isHiresMode() to know the size of the display
//if the display is in low-res mode, the size is 64 * 32 and
//only the first 64 * 32 bytes of the display buffer are used
uint8_t *getDisplay();

//returns 1 if the display has been updated
// returns 0 if the display has not been updated
int isDisplayUpdated();

// pause the emulator or unpause it if paused
void pauseChip();

#endif