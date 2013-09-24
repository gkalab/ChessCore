//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Lowlevel.h: Low-level function declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {

extern "C"
{

// popcnt() function pointer
extern CHESSCORE_EXPORT uint32_t (ASMCALL *popcnt)(uint64_t bb);

// C++ popcnt() implementation
extern CHESSCORE_EXPORT uint32_t ASMCALL cppPopcnt(uint64_t bb);

#if CPU_X86

extern CHESSCORE_EXPORT bool ASMCALL x86HasPopcnt();
extern CHESSCORE_EXPORT uint32_t ASMCALL x86Popcnt(uint64_t bb);
extern CHESSCORE_EXPORT uint32_t ASMCALL x86Lsb(uint64_t bb);
extern CHESSCORE_EXPORT uint32_t ASMCALL x86Lsb2(uint64_t &bb, uint64_t &bit);
extern CHESSCORE_EXPORT uint16_t ASMCALL x86Bswap16(uint16_t value);
extern CHESSCORE_EXPORT uint32_t ASMCALL x86Bswap32(uint32_t value);
extern CHESSCORE_EXPORT uint64_t ASMCALL x86Bswap64(uint64_t value);

#define lsb(bb) x86Lsb(bb)
#define lsb2(bb, bit) x86Lsb2(bb, bit)
#define bswap16(v) x86Bswap16(v)
#define bswap32(v) x86Bswap32(v)
#define bswap64(v) x86Bswap64(v)

#elif CPU_X64

extern CHESSCORE_EXPORT bool ASMCALL x64HasPopcnt();
extern CHESSCORE_EXPORT uint32_t ASMCALL x64Popcnt(uint64_t bb);
extern CHESSCORE_EXPORT uint32_t ASMCALL x64Lsb(uint64_t bb);
extern CHESSCORE_EXPORT uint32_t ASMCALL x64Lsb2(uint64_t &bb, uint64_t &bit);
extern CHESSCORE_EXPORT uint16_t ASMCALL x64Bswap16(uint16_t value);
extern CHESSCORE_EXPORT uint32_t ASMCALL x64Bswap32(uint32_t value);
extern CHESSCORE_EXPORT uint64_t ASMCALL x64Bswap64(uint64_t value);

#define lsb(bb) x64Lsb(bb)
#define lsb2(bb, bit) x64Lsb2(bb, bit)
#define bswap16(v) x64Bswap16(v)
#define bswap32(v) x64Bswap32(v)
#define bswap64(v) x64Bswap64(v)

#else

extern CHESSCORE_EXPORT uint32_t ASMCALL cppLsb(uint64_t bb);
extern CHESSCORE_EXPORT uint32_t ASMCALL cppLsb2(uint64_t &bb, uint64_t &bit);
extern CHESSCORE_EXPORT uint16_t ASMCALL cppBswap16(uint16_t x);
extern CHESSCORE_EXPORT uint32_t ASMCALL cppBswap32(uint32_t x);
extern CHESSCORE_EXPORT uint64_t ASMCALL cppBswap64(uint64_t x);

#define lsb(bb) cppLsb(bb)
#define lsb2(bb, bit) cppLsb2(bb, bit)
#define bswap16(v) cppBswap16(v)
#define bswap32(v) cppBswap32(v)
#define bswap64(v) cppBswap64(v)

#endif // CPU_xxx

}    // extern "C"

/**
 * Initialise low-level module. Will set-up the popcnt function pointer to
 * use either the assembler or C++ implementation.
 */
extern CHESSCORE_EXPORT void lowlevelInit();

/**
 * Determine if the CPU POPCNT instruction is being used by the popcnt() function.
 *
 * @return true if the CPU POPCNT instruction is being used, else false.
 */
extern CHESSCORE_EXPORT bool usingCpuPopcnt();

} // namespace ChessCore
