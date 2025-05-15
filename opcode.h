#ifndef CHIP8_OPCODE_H
#define CHIP8_OPCODE_H

#include <stdint.h>

void chip8_decode_and_execute(uint16_t opcode);

#endif