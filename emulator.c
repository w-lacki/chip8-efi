
#define MEM 4096 // 4K

struct {
    char V[16];
    short index;
    short pc;
    short sp;
    short stack[16];
} chip8;

void next_cycle() {

}