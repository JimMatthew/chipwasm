/*
    raylibmain.c
    This file contains the main function for the Chip8 emulator using raylib.
    It initializes the emulator, loads a ROM, and handles input and rendering.
*/

#include "chip8.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

void update_keys() {
    struct {
        int key;
        uint8_t chip8_key;
    } keymap[] = {
        {KEY_ONE, 0x1}, {KEY_TWO, 0x2}, {KEY_THREE, 0x3}, {KEY_FOUR, 0xC},
        {KEY_Q, 0x4},   {KEY_W, 0x5},   {KEY_E, 0x6},     {KEY_R, 0xD},
        {KEY_A, 0x7},   {KEY_S, 0x8},   {KEY_D, 0x9},     {KEY_F, 0xE},
        {KEY_Z, 0xA},   {KEY_X, 0x0},   {KEY_C, 0xB},     {KEY_V, 0xF},
    };

    int count = sizeof(keymap) / sizeof(keymap[0]);

    for (int i = 0; i < count; i++) {
        int key = keymap[i].key;
        uint8_t chip8_key = keymap[i].chip8_key;

        if (IsKeyDown(key)) {
            chip8_press_key(chip8_key);
        }
        if (IsKeyReleased(key)) {
            chip8_release_key(chip8_key);
        }
    }
}

void loadProgram(char *filename) {
    uint8_t program[4000];
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Error opening file");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    fread(&program, 1, size, f);
    loadROM(program, size);
    fclose(f);
}

void drawDisplay() {
    uint8_t *display = getDisplay();
    ClearBackground(BLACK);
    int height = isHiresMode() ? 64 : 32;
    int width = isHiresMode() ? 128 : 64;
    int pixelSize = isHiresMode() ? 5 : 10;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (display[y * width + x]) {
                DrawRectangle(x * pixelSize, y * pixelSize, pixelSize,
                              pixelSize, RAYWHITE);
            }
        }
    }
}

int main(int argc, char **argv) {
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 950;
    const int screenHeight = 500;
    int isPaused = 0;
    int mode = 0;
    chip8Init();
    if (argc != 2) {
        printf("Usage: %s program.ch8\n", argv[0]);
        return 1;
    } else {
        loadProgram(argv[1]);
    }

    InitWindow(screenWidth, screenHeight, "chip-8 emulator - JML");

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    while (!WindowShouldClose()) {
        update_keys();
        for (int i = 0; i < 8; i++) {
            chip8Cycle();
        }
        chip8Tick();

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);
        drawDisplay();

        int buttonWidth = 100;
        int buttonHeight = 40;
        int buttonX
            = GetScreenWidth() - buttonWidth - 20; // 20px from right edge
        int buttonY = 20;
        int pauseButtonX = GetScreenWidth() - buttonWidth - 130;
        int modeButtonY = 70;

        // --- Draw the buttons ---
        DrawRectangle(buttonX, buttonY, buttonWidth, buttonHeight, DARKGRAY);
        DrawRectangleLines(buttonX, buttonY, buttonWidth, buttonHeight,
                           RAYWHITE);
        DrawText("Reload", buttonX + 15, buttonY + 10, 20, RAYWHITE);

        DrawRectangle(pauseButtonX, buttonY, buttonWidth, buttonHeight,
                      DARKGRAY);
        DrawRectangleLines(pauseButtonX, buttonY, buttonWidth, buttonHeight,
                           RAYWHITE);
        DrawText(isPaused ? "Start" : "Pause", pauseButtonX + 15, buttonY + 10,
                 20, RAYWHITE);

        DrawRectangle(buttonX, modeButtonY, buttonWidth, buttonHeight,
                      DARKGRAY);
        DrawRectangleLines(buttonX, modeButtonY, buttonWidth, buttonHeight,
                           RAYWHITE);
        DrawText(mode ? "schip" : "chip8", buttonX + 15, modeButtonY + 10, 20,
                 RAYWHITE);
        // --- Handle button clicks ---

        if (CheckCollisionPointRec(
                GetMousePosition(),
                (Rectangle){buttonX, buttonY, buttonWidth, buttonHeight})
            && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            reload();
        }

        if (CheckCollisionPointRec(
                GetMousePosition(),
                (Rectangle){pauseButtonX, buttonY, buttonWidth, buttonHeight})
            && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            pauseChip();
            isPaused = isPaused ? 0 : 1;
        }

        if (CheckCollisionPointRec(
                GetMousePosition(),
                (Rectangle){buttonX, modeButtonY, buttonWidth, buttonHeight})
            && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mode = mode ? 0 : 1;
            setMode(mode);
        }

        EndDrawing();
    }
    CloseWindow(); 

    return 0;
}