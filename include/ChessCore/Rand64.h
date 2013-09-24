//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Rand64.h: 64-bit Random Number Generator class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {
class CHESSCORE_EXPORT Rand64 {
protected:
    static const unsigned RANDSIZL = 8;
    static const unsigned RANDSIZ = 1 << RANDSIZL;
    static uint64_t randrsl[RANDSIZ];
    static uint64_t randCount;
    static uint64_t mm[RANDSIZ];
    static uint64_t aa;
    static uint64_t bb;
    static uint64_t cc;

    static void isaac64();
public:
    static void init();
    static uint64_t rand();
};
} // namespace ChessCore
