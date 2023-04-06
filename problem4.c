#include <stdint.h>
#include <stdio.h>

int ceil_log2(uint32_t x)
{
    uint32_t r, shift;
    int zero = (x == 0);

    x--;
    r = (x > 0xFFFF) << 4;
    x >>= r;
    shift = (x > 0xFF) << 3;
    x >>= shift;
    r |= shift;
    shift = (x > 0xF) << 2;
    x >>= shift;
    r |= shift;
    shift = (x > 0x3) << 1;
    x >>= shift;
    return ((r | shift | x >> 1) + 1) * !zero;
}

int main(int argc, char *argv[])
{
    printf("%u\n", ceil_log2(0));
    return 0;
}
