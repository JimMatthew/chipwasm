Chip-8 Emulator

The core emulator is written in C. 
There are 2 compilation targets, wasm for the web, and with Raylib for a native desktop application. 

The core emulator logic is in chip8.c and opcode.c

To compile for the web, run the make file, which uses main.c to compile to webassembly. Then serve the
html, js, and wasm files. 


To compile a native desktop application, compile using raylibmain.c. 

