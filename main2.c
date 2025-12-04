// BIG ENDIAN AABB -> AA, BB

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void panic(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "PANIC: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    abort();
}

void TODO() { panic("NOT IMPLEMENTED"); }

unsigned char V[16];
unsigned char memory[4096];

unsigned short I;
unsigned short pc;

unsigned short stack[16];
unsigned short sp;

unsigned char delay_timer;
unsigned char sound_timer;

unsigned char random_byte() {
    TODO();
}

unsigned char wait_for_key() {
    TODO();
}

unsigned short fetch() {
    return memory[pc] << 8 | memory[pc + 1];
}


void decode(unsigned short opcode) {
    switch (opcode & 0xF000 >> 12) {
        case 0x0: {
            switch (opcode & 0xF) {
                case 0xE0: TODO();
                    break;
                case 0xEE: {
                    pc = stack[--sp];
                    pc += 2;
                    break;
                }
                default: panic("Invalid opcode %x", opcode);
            }
            break;
        }
        case 0x1: {
            pc = opcode & 0xFFF;;
            break;
        }
        case 0x2: {
            stack[sp++] = pc;;
            pc = opcode & 0xFFF;
            break;
        }
        case 0x3: {
            if (V[opcode >> 8 & 0xF] == opcode & 0xFF) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x4: {
            if (V[opcode >> 8 & 0xF] != opcode & 0xFF) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x5: {
            if (V[opcode >> 8 & 0xF] == V[opcode >> 4 & 0xF]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x6: {
            V[opcode >> 8 & 0xF] = opcode & 0xFF;
            pc += 2;
            break;
        }
        case 0x7: {
            V[opcode >> 8 & 0xF] += opcode & 0xFF;
            pc += 2;
            break;
        }
        case 0x8: {
            unsigned char x = opcode >> 8 & 0xF;
            unsigned char y = opcode >> 4 & 0xF;

            switch (opcode & 0xF) {
                case 0x0:
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x1:
                    V[x] = V[x] | V[y];
                    pc += 2;
                    break;
                case 0x2:
                    V[x] = V[x] & V[y];
                    pc += 2;
                    break;
                case 0x3:
                    V[x] = V[x] ^ V[y];
                    pc += 2;
                    break;
                case 0x4: {
                    const unsigned short sum = V[x] + V[y];
                    V[x] = sum & 0xFF;
                    V[0xF] = sum > 0xFF;
                    pc += 2;
                    break;
                }
                case 0x5: {
                    V[0xF] = V[x] > V[y];
                    V[x] = V[x] - V[y];
                    pc += 2;
                    break;
                }
                case 0x6: {
                    V[0xF] = V[x] & 1;
                    V[x] >>= 1;
                    pc += 2;
                    break;
                }
                case 0x7: {
                    V[0xF] = V[y] > V[x];
                    V[x] = V[y] - V[x];
                    pc += 2;
                    break;
                }
                case 0xE: {
                    V[0xF] = V[x] & 0x80 >> 7;
                    V[x] <<= 1;
                    pc += 2;
                    break;
                }
                default: panic("Unknown opcode: %x", opcode);
            }
        }
        case 0x9: {
            if (V[opcode >> 8 & 0xF] != V[opcode >> 4 & 0xF]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0xA: {
            I = opcode & 0xFFF;
            pc += 2;
            break;
        }
        case 0xB: {
            pc = (opcode & 0xFFF) + V[0];
            break;
        }
        case 0xC: {
            V[opcode >> 8 & 0xF] = random_byte() & (opcode & 0xFF);
            pc += 2;
            break;
        }
        case 0xD: {
            TODO();
            break;
        }
        case 0xE: {
            switch (opcode & 0xFF) {
                case 0x9E: TODO();
                    break;
                case 0xA1: TODO();
                    break;
                default: panic("invalid opcode %x", opcode);
            }
            break;
        }
        case 0xF: {
            switch (opcode & 0xFF) {
                case 0x07:
                    V[opcode >> 8 & 0xF] = delay_timer;
                    pc += 2;
                    break;
                case 0x0A:
                    V[opcode >> 8 & 0xF] = wait_for_key();
                    pc += 2;
                    break;
                case 0x15:
                    delay_timer = V[opcode >> 8 & 0xF];
                    pc += 2;
                    break;
                case 0x18:
                    sound_timer = V[opcode >> 8 & 0xF];
                    pc += 2;
                    break;
                case 0x1E:
                    unsigned short result = I + V[opcode >> 8 & 0xF];
                    V[0xF] = result > 0xFFF;
                    I = result;
                    pc += 2;
                    break;
                case 0x29: TODO();
                    break;
                case 0x33: TODO();
                    break;
                case 0x55: TODO();
                    break;
                case 0x65: TODO();
                    break;
                default: panic("invalid opcode %x", opcode);
            }
            break;
        }
        default: panic("invalid opcode %x", opcode);
    }
}


int main() {
    return 1;
}
