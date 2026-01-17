void panic(const char *fmt, ...) {
    //pass
}

unsigned char V[16];
unsigned char memory[4096];
unsigned short I;
unsigned short pc;

unsigned short stack[16];
unsigned short sp;

unsigned char delay_timer;

unsigned char fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80 //F
};

unsigned char gfx[64 * 32];
unsigned char key[16];
unsigned char drawFlag;

unsigned char random_byte() {
    static unsigned int seed = 0xCAFEBEBE;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed & 0xFF;
}

unsigned short fetch() {
    return memory[pc] << 8 | memory[pc + 1];
}

void run_cycle() {
    const unsigned short opcode = fetch();

    switch ((opcode & 0xF000) >> 12) {
        case 0x0: {
            switch (opcode & 0xFF) {
                case 0xE0:
                    for (int i = 0; i < 2048; ++i) {
                        gfx[i] = 0;
                    }
                    drawFlag = true;
                    pc += 2;
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
            pc = opcode & 0xFFF;
            break;
        }
        case 0x2: {
            stack[sp++] = pc;;
            pc = opcode & 0xFFF;
            break;
        }
        case 0x3: {
            if (V[(opcode >> 8) & 0xF] == (opcode & 0xFF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x4: {
            if (V[(opcode >> 8) & 0xF] != (opcode & 0xFF)) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x5: {
            if (V[(opcode >> 8) & 0xF] == V[(opcode >> 4) & 0xF]) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x6: {
            V[(opcode >> 8) & 0xF] = (opcode & 0xFF);
            pc += 2;
            break;
        }
        case 0x7: {
            V[(opcode >> 8) & 0xF] += opcode & 0xFF;
            pc += 2;
            break;
        }
        case 0x8: {
            unsigned char x = (opcode >> 8) & 0xF;
            unsigned char y = (opcode >> 4) & 0xF;

            switch (opcode & 0xF) {
                case 0x0:
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x1:
                    V[x] |= V[y];
                    pc += 2;
                    break;
                case 0x2:
                    V[x] &= V[y];
                    pc += 2;
                    break;
                case 0x3:
                    V[x] ^= V[y];
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
                    V[0xF] = (V[x] & 0x80) >> 7;
                    V[x] <<= 1;
                    pc += 2;
                    break;
                }
                default: panic("Unknown opcode: %x", opcode);
            }
            break;
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
            unsigned short x = V[(opcode >> 8) & 0xF];
            unsigned short y = V[(opcode >> 4) & 0xF];
            unsigned short height = opcode & 0xF;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                unsigned short pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & 0x80 >> xline) != 0) {
                        if (gfx[(x + xline + (y + yline) * 64)] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[x + xline + (y + yline) * 64] ^= 1;
                    }
                }
            }

            drawFlag = true;
            pc += 2;
            break;
        }
        case 0xE: {
            switch (opcode & 0xFF) {
                case 0x9E:
                    if (key[V[opcode >> 8 & 0xF]] != 0) {
                        pc += 4;
                        key[V[opcode >> 8 & 0xF]] = 0;
                    } else {
                        pc += 2;
                    }
                    break;
                case 0xA1:
                    if (key[V[(opcode >> 8) & 0xF]] == 0) {
                        pc += 4;
                    } else {
                        key[V[(opcode >> 8) & 0xF]] = 0;
                        pc += 2;
                    }
                    break;
                default: panic("invalid opcode %x", opcode);
            }
            break;
        }
        case 0xF: {
            switch (opcode & 0xFF) {
                case 0x7:
                    V[(opcode >> 8) & 0xF] = delay_timer;
                    pc += 2;
                    break;
                case 0xA:
                    bool key_pressed = false;

                    for (int i = 0; i < 16; ++i) {
                        if (key[i] != 0) {
                            V[(opcode >> 8) & 0xF] = i;
                            key_pressed = true;
                            key[i] = 0;
                        }
                    }

                    if (!key_pressed)
                        return;

                    pc += 2;
                    break;
                case 0x15:
                    delay_timer = V[(opcode >> 8) & 0xF];
                    pc += 2;
                    break;
                case 0x18:
                    // Skip - no sound implemented
                    pc += 2;
                    break;
                case 0x1E:
                    unsigned short result = I + V[(opcode >> 8) & 0xF];
                    V[0xF] = result > 0xFFF;
                    I = result;
                    pc += 2;
                    break;
                case 0x29:
                    I = V[(opcode & 0xF00) >> 8] * 0x5;
                    pc += 2;
                    break;
                case 0x33:
                    memory[I] = V[opcode >> 8 & 0xF] / 100;
                    memory[I + 1] = V[opcode >> 8 & 0xF] / 10 % 10;
                    memory[I + 2] = V[opcode >> 8 & 0xF] % 10;
                    pc += 2;
                    break;
                case 0x55:
                    for (int i = 0; i <= (opcode >> 8 & 0xF); ++i) {
                        memory[I + i] = V[i];
                    }

                    I += ((opcode >> 8) & 0xF) + 1;
                    pc += 2;
                    break;
                case 0x65:
                    for (int i = 0; i <= (opcode >> 8 & 0xF); ++i) {
                        V[i] = memory[I + i];
                    }

                    I += (opcode >> 8 & 0xF) + 1;
                    pc += 2;
                    break;
                default: panic("invalid opcode %x", opcode);
            }
            break;
        }
        default: panic("invalid opcode %x", opcode);
    }

    if (delay_timer > 0)
        --delay_timer;
}

void init() {
    pc = 0x200;
    I = 0;
    sp = 0;

    for (int i = 0; i < 32 * 64; ++i) {
        gfx[i] = 0;
    }

    for (int i = 0; i < 16; ++i) {
        stack[i] = 0;
        key[i] = 0;
        V[i] = 0;
    }

    for (int i = 0; i < 4096; ++i) {
        memory[i] = 0;
    }

    for (int i = 0; i < 80; ++i) {
        memory[i] = fontset[i];
    }

    delay_timer = 0;
}

void load(unsigned char *rom, unsigned int size) {
    init();

    for (int i = 0; i < size; ++i) {
        memory[i + 0x200] = rom[i];
    }
}
