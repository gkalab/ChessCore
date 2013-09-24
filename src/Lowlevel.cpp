/*
 * ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
 *
 * Lowlevel.cpp: Low-level function implementation.
 */

#include <ChessCore/Lowlevel.h>

namespace ChessCore {

// popcnt() function pointer
uint32_t (ASMCALL *popcnt)(uint64_t bb);
static bool isUsingCpuPopcnt = false;

// C++ popcnt implementation
uint32_t ASMCALL cppPopcnt(uint64_t bb) {
    const uint64_t C55 = 0x5555555555555555ULL;
    const uint64_t C33 = 0x3333333333333333ULL;
    const uint64_t C0F = 0x0f0f0f0f0f0f0f0fULL;
    const uint64_t C01 = 0x0101010101010101ULL;

    bb -= (bb >> 1) & C55;          // put count of each 2 bits into those 2 bits
    bb = (bb & C33) + ((bb >> 2) & C33); // put count of each 4 bits into those 4 bits
    bb = (bb + (bb >> 4)) & C0F;    // put count of each 8 bits into those 8 bits
    return (bb * C01) >> 56;        // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
}

#if !CPU_X86 && !CPU_X64

static const uint32_t mod37lsb[37] = { // map a bit value mod 37 to its position
    32, 0, 1, 26, 2, 23, 27, 0,  3,  16,  24, 30, 28, 11, 0,
    13, 4, 7, 17, 0, 25, 22, 31, 15, 29,  10, 12, 6,  0,  21,
    14, 9, 5, 20, 8, 19, 18
};

uint32_t ASMCALL cppLsb(uint64_t bb) {
    if (bb == 0ULL)
        return 64;

    uint32_t count = 0;
    int i;

    if ((bb & 0xffffffffULL) == 0) {
        bb >>= 32;
        count = 32;
    }

    i = (int)bb;
    count += mod37lsb[(-i & i) % 37];
    return count;
}

uint32_t ASMCALL cppLsb2(uint64_t &bb, uint64_t &bit) {
    uint32_t count = 0, u32;
    uint64_t tempBB = bb, tempBit;

    if (tempBB == 0ULL)
        return 64;

    if ((tempBB & 0xffffffffULL) == 0ULL) {
        tempBB >>= 32;
        count = 32;
    }

    u32 = (uint32_t)tempBB;
    count += _mod37lsb[(-u32 & u32) % 37];
    tempBit = 1ULL << count;
    bb &= ~tempBit;
    bit = tempBit;
    return count;
}

uint16_t ASMCALL cppBswap16(uint16_t x) {
    return (x >> 8) | (x << 8);
}

uint32_t ASMCALL cppBswap32(uint32_t x) {
    return
        (x >> 24) |
        ((x << 8) & 0x00FF0000) |
        ((x >> 8) & 0x0000FF00) |
        (x << 24);
}

uint64_t ASMCALL cppBswap64(uint64_t x) {
    return
        (x >> 56) |
        ((x << 40) & 0x00FF000000000000ULL) |
        ((x << 24) & 0x0000FF0000000000ULL) |
        ((x << 8) & 0x000000FF00000000ULL) |
        ((x >> 8) & 0x00000000FF000000ULL) |
        ((x >> 24) & 0x0000000000FF0000ULL) |
        ((x >> 40) & 0x000000000000FF00ULL) |
        (x << 56);
}
#endif // !CPU_X86 && !CPU_X64

void lowlevelInit() {
#if CPU_X86
    if (x86HasPopcnt()) {
        popcnt = x86Popcnt;
        isUsingCpuPopcnt = true;
    } else {
        popcnt = cppPopcnt;
        isUsingCpuPopcnt = false;
    }
#elif CPU_X64
    if (x64HasPopcnt()) {
        popcnt = x64Popcnt;
        isUsingCpuPopcnt = true;
    } else {
        popcnt = cppPopcnt;
        isUsingCpuPopcnt = false;
    }
#else
    popcnt = cppPopcnt;
    isUsingCpuPopcnt = false;
#endif
}

bool usingCpuPopcnt() {
    return isUsingCpuPopcnt;
}

}   // namespace ChessCore
