//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Rand64.cpp: 64-bit Random Number Generator class implementations.
//

#include <ChessCore/Rand64.h>
#include <ChessCore/Util.h>
#include <stdlib.h>

using namespace std;

namespace ChessCore {
uint64_t Rand64::randrsl[RANDSIZ], Rand64::randCount;
uint64_t Rand64::mm[RANDSIZ];
uint64_t Rand64::aa = 0, Rand64::bb = 0, Rand64::cc = 0;

#define ind(mm, x) (*(uint64_t *)((unsigned char *)(mm) + ((x) & ((RANDSIZ - 1) << 3))))
#define rngstep(mix, a, b, mm, m, m2, r, x) \
    { \
        x = *m;  \
        a = (mix) + *(m2++); \
        *(m++) = y = ind(mm, x) + a + b; \
        *(r++) = b = ind(mm, y >> RANDSIZL) + x; \
    }

void Rand64::isaac64() {
    uint64_t a, b, x, y, *m, *m2, *r, *mend;

    r = randrsl;
    a = aa;
    b = bb + (++cc);

    for (m = mm, mend = m2 = m + (RANDSIZ / 2); m < mend; ) {
        rngstep(~(a ^ (a << 21)), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 5), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a << 12), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 33), a, b, mm, m, m2, r, x);
    }

    for (m2 = mm; m2 < mend; ) {
        rngstep(~(a ^ (a << 21)), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 5), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a << 12), a, b, mm, m, m2, r, x);
        rngstep(a ^ (a >> 33), a, b, mm, m, m2, r, x);
    }

    bb = b;
    aa = a;
}

#define mix(a, b, c, d, e, f, g, h) \
    { \
        a -= e; f ^= h >> 9;  h += a; \
        b -= f; g ^= a << 9;  a += b; \
        c -= g; h ^= b >> 23; b += c; \
        d -= h; a ^= c << 15; c += d; \
        e -= a; b ^= d >> 14; d += e; \
        f -= b; c ^= e << 20; e += f; \
        g -= c; d ^= f >> 17; f += g; \
        h -= d; e ^= g << 14; g += h; \
    }

void Rand64::init() {
    unsigned i;
    uint64_t a, b, c, d, e, f, g, h;

    // Use system rand() to seed this rand().  Might not be a good idea?
    srand(Util::getTickCount());

    for (i = 0; i < RANDSIZ; ++i) {
        randrsl[i] = ::rand() & 0xff;
        mm[i] = 0;
    }

    aa = bb = cc = (uint64_t)0;
    a = b = c = d = e = f = g = h = 0x9e3779b97f4a7c13LL; /* the golden ratio */

    for (i = 0; i < 4; ++i) /* scramble it */
        mix(a, b, c, d, e, f, g, h);

    for (i = 0; i < RANDSIZ; i += 8) { /* fill in mm[] with messy stuff */
        a += randrsl[i];
        b += randrsl[i + 1];
        c += randrsl[i + 2];
        d += randrsl[i + 3];
        e += randrsl[i + 4];
        f += randrsl[i + 5];
        g += randrsl[i + 6];
        h += randrsl[i + 7];

        mix(a, b, c, d, e, f, g, h);
        mm[i] = a;
        mm[i + 1] = b;
        mm[i + 2] = c;
        mm[i + 3] = d;
        mm[i + 4] = e;
        mm[i + 5] = f;
        mm[i + 6] = g;
        mm[i + 7] = h;
    }

    for (i = 0; i < RANDSIZ; i += 8) {
        a += mm[i];
        b += mm[i + 1];
        c += mm[i + 2];
        d += mm[i + 3];
        e += mm[i + 4];
        f += mm[i + 5];
        g += mm[i + 6];
        h += mm[i + 7];
        mix(a, b, c, d, e, f, g, h);
        mm[i] = a;
        mm[i + 1] = b;
        mm[i + 2] = c;
        mm[i + 3] = d;
        mm[i + 4] = e;
        mm[i + 5] = f;
        mm[i + 6] = g;
        mm[i + 7] = h;
    }

    isaac64(); /* fill in the first set of results */
    randCount = RANDSIZ; /* prepare to use the first set of results */
}

uint64_t Rand64::rand() {
    return !randCount-- ? (isaac64(), randCount = RANDSIZ - 1, randrsl[randCount]) : randrsl[randCount];
}

}   // namespace ChessCore
