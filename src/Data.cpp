//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Data.cpp: static data definitions.
//

#include <ChessCore/Data.h>
#include <ChessCore/Util.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Log.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

namespace ChessCore {
#ifdef DEBUG
static const char *m_classname = 0;
#endif // DEBUG

#define DUMP_PAWN_INIT   0
#define DUMP_KNIGHT_INIT 0
#define DUMP_KING_INIT   0
#define DUMP_FILE_INIT   0
#define DUMP_RANK_INIT   0

#define DUMP_ANY         (DUMP_PAWN_INIT + DUMP_KNIGHT_INIT + DUMP_KING_INIT + DUMP_FILE_INIT + DUMP_RANK_INIT)

uint64_t magicRookMult[64] = {
    0xa8002c000108020ULL,  0x4440200140003000ULL,    0x8080200010011880ULL,      0x380180080141000ULL,
    0x1a00060008211044ULL, 0x410001000a0c0008ULL,    0x9500060004008100ULL,      0x100024284a20700ULL,
    0x802140008000ULL,     0x80c01002a00840ULL,      0x402004282011020ULL,       0x9862000820420050ULL,
    0x1001448011100ULL,    0x6432800200800400ULL,    0x40100010002000cULL,       0x2800d0010c080ULL,
    0x90c0008000803042ULL, 0x4010004000200041ULL,    0x3010010200040ULL,         0xa40828028001000ULL,
    0x123010008000430ULL,  0x24008004020080ULL,      0x60040001104802ULL,        0x582200028400d1ULL,
    0x4000802080044000ULL, 0x408208200420308ULL,     0x610038080102000ULL,       0x3601000900100020ULL,
    0x80080040180ULL,      0xc2020080040080ULL,      0x80084400100102ULL,        0x4022408200014401ULL,
    0x40052040800082ULL,   0xb08200280804000ULL,     0x8a80a008801000ULL,        0x4000480080801000ULL,
    0x911808800801401ULL,  0x822a003002001894ULL,    0x401068091400108aULL,      0x4a10a00004cULL,
    0x2000800640008024ULL, 0x1486408102020020ULL,    0x100a000d50041ULL,         0x810050020b0020ULL,
    0x204000800808004ULL,  0x20048100a000cULL,       0x112000831020004ULL,       0x9000040810002ULL,
    0x440490200208200ULL,  0x8910401000200040ULL,    0x6404200050008480ULL,      0x4b824a2010010100ULL,
    0x4080801810c0080ULL,  0x400802a0080ULL,         0x8224080110026400ULL,      0x40002c4104088200ULL,
    0x1002100104a0282ULL,  0x1208400811048021ULL,    0x3201014a40d02001ULL,      0x5100019200501ULL,
    0x101000208001005ULL,  0x2008450080702ULL,       0x1002080301d00cULL,        0x410201ce5c030092ULL
};

int magicRookShift[64] = {
    52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52
};

uint64_t magicRookMask[64];
int magicRookIndex[64];
uint64_t magicRookAtkMasks[0x19000];

uint64_t magicBishopMult[64] = {
    0x440049104032280ULL,  0x1021023c82008040ULL,   0x404040082000048ULL,      0x48c4440084048090ULL,
    0x2801104026490000ULL, 0x4100880442040800ULL,   0x181011002e06040ULL,      0x9101004104200e00ULL,
    0x1240848848310401ULL, 0x2000142828050024ULL,   0x1004024d5000ULL,         0x102044400800200ULL,
    0x8108108820112000ULL, 0xa880818210c00046ULL,   0x4008008801082000ULL,     0x60882404049400ULL,
    0x104402004240810ULL,  0xa002084250200ULL,      0x100b0880801100ULL,       0x4080201220101ULL,
    0x44008080a00000ULL,   0x202200842000ULL,       0x5006004882d00808ULL,     0x200045080802ULL,
    0x86100020200601ULL,   0xa802080a20112c02ULL,   0x80411218080900ULL,       0x200a0880080a0ULL,
    0x9a01010000104000ULL, 0x28008003100080ULL,     0x211021004480417ULL,      0x401004188220806ULL,
    0x825051400c2006ULL,   0x140c0210943000ULL,     0x242800300080ULL,         0xc2208120080200ULL,
    0x2430008200002200ULL, 0x1010100112008040ULL,   0x8141050100020842ULL,     0x822081014405ULL,
    0x800c049e40400804ULL, 0x4a0404028a000820ULL,   0x22060201041200ULL,       0x360904200840801ULL,
    0x881a08208800400ULL,  0x60202c00400420ULL,     0x1204440086061400ULL,     0x8184042804040ULL,
    0x64040315300400ULL,   0xc01008801090a00ULL,    0x808010401140c00ULL,      0x4004830c2020040ULL,
    0x80005002020054ULL,   0x40000c14481a0490ULL,   0x10500101042048ULL,       0x1010100200424000ULL,
    0x640901901040ULL,     0xa0201014840ULL,        0x840082aa011002ULL,       0x10010840084240aULL,
    0x420400810420608ULL,  0x8d40230408102100ULL,   0x4a00200612222409ULL,     0xa08520292120600ULL
};

int magicBishopShift[64] = {
    58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58
};

uint64_t magicBishopMask[64];
int magicBishopIndex[64];
uint64_t magicBishopAtkMasks[0x1480];

uint64_t pawnAttacks[MAXCOLOURS][MAXSQUARES];
uint64_t epMask[MAXCOLOURS][MAXSQUARES];
uint64_t knightAttacks[MAXSQUARES];
uint64_t kingAttacks[MAXSQUARES];
uint64_t fileMasks[MAXFILES];
uint64_t rankMasks[MAXRANKS];
uint64_t rankMask1to4;
uint64_t rankMask5to8;
uint64_t rankMask1and8;
uint64_t rankMask2and7;
uint64_t fileRankMasks[MAXSQUARES];
uint64_t diagMasks[MAXSQUARES];
uint64_t connectMasks[MAXSQUARES][MAXSQUARES];
int8_t pinnedDirs[MAXSQUARES][MAXSQUARES];
uint64_t originalSquares[MAXCOLOURS][MAXPIECES];
uint64_t rookSquares;
uint64_t kingSquares;
uint64_t notFileA;
uint64_t notFileH;
uint64_t fileAH;

char pieceChars[MAXPIECES] = {
    'X', 'P', 'R', 'N', 'B', 'Q', 'K'
};

//
// TODO: Move unicodePieces[]
//

// Pieces in ChessFront.ttf
const char *unicodePieces[12] = {
    "\xe2\x99\x99", // White Pawn
    "\xe2\x99\x96", // White Rook
    "\xe2\x99\x98", // White Knight
    "\xe2\x99\x97", // White Bishop
    "\xe2\x99\x95", // White Queen
    "\xe2\x99\x94", // White King
    "\xe2\x99\x9f", // Black Pawn
    "\xe2\x99\x9c", // Black Rook
    "\xe2\x99\x9e", // Black Knight
    "\xe2\x99\x9d", // Black Bishop
    "\xe2\x99\x9b", // Black Queen
    "\xe2\x99\x9a"  // Black King
};

static inline bool setIfOnBoard(int file, int rank, uint64_t &bb) {
    if ((file & ~7) == 0 && (rank & ~7) == 0) {
        bb |= fileRankBit(file, rank);
        return true;
    }
    return false;
}

static uint64_t slide(int sq, uint64_t b, int deltas[], int fmin, int fmax, int rmin, int rmax) {
    uint64_t result = 0;
    int rk = sq / 8, fl = sq % 8, r, f, i, dx, dy;

    for (i = 0; i < 4; i++) {
        for (dx = ((9 + deltas[i]) & 7) - 1, dy = deltas[i] / 7, f = fl + dx, r = rk + dy;
             (!dx || (f >= fmin && f <= fmax)) && (!dy || (r >= rmin && r <= rmax));
             f += dx, r += dy) {
            result |= (1ULL << (f + r * 8));

            if (b & (1ULL << (f + r * 8)))
                break;
        }
    }
    return result;
}

static void initSlideAttacks(uint64_t attacks[], int aIndex[], uint64_t mask[], const int shift[],
                             const uint64_t mult[], int deltas[]) {
    int i, j, k, l, m, ix;
    uint64_t b, bb, scratch;

    for (i = 0, ix = 0; i < 64; i++, ix += j) {
        aIndex[i] = ix;
        mask[i] = slide(i, 0, deltas, 1, 6, 1, 6);
        j = (1 << (64 - shift[i]));

        for (k = 0; k < j; k++) {
            for (l = 0, b = 0, bb = mask[i]; bb; l++) {
                m = lsb2(bb, scratch);

                if (k & (1 << l))
                    b |= (1ULL << m);
            }

            attacks[ix + ((b * mult[i]) >> shift[i])] = slide(i, b, deltas, 0, 7, 0, 7);
        }
    }
}

//
// Initialise data structures
//
bool dataInit() {
    uint32_t i;

    int f, r, o, f2, r2, o2;
    uint64_t bb;
#if defined(DEBUG) && DUMP_ANY > 0
    char dbgbuf[256];
#endif

    int dirs[] = { 9, 7, -7, -9, 1, -1, 8, -8 };
    initSlideAttacks(magicBishopAtkMasks, magicBishopIndex, magicBishopMask, magicBishopShift,
                     magicBishopMult, dirs);
    initSlideAttacks(magicRookAtkMasks, magicRookIndex, magicRookMask, magicRookShift,
                     magicRookMult, dirs + 4);

    // Initialise pawnAttacks
    // Note: we initialise ranks 1 and 8 as well, even though no pawn will
    // ever be on those ranks.  This is because position::attacks() uses
    // these masks in reverse to determine if a pawn is attacking a square.
    memset(pawnAttacks, 0, sizeof(pawnAttacks));

    for (i = 0; i < MAXSQUARES; i++) {
        bb = 0;
        f = offsetFile(i);
        r = offsetRank(i);
        setIfOnBoard(f - 1, r + 1, bb);
        setIfOnBoard(f + 1, r + 1, bb);
        pawnAttacks[WHITE][i] = bb;

#if defined(DEBUG) && DUMP_PAWN_INIT > 0
        dumpBitboard(bb, dbgbuf);
        logdbg("pawnAttacks[WHITE][%u]=\n%s", i, dbgbuf);
#endif

        bb = 0;
        setIfOnBoard(f - 1, r - 1, bb);
        setIfOnBoard(f + 1, r - 1, bb);
        pawnAttacks[BLACK][i] = bb;

#if defined(DEBUG) && DUMP_PAWN_INIT > 0
        dumpBitboard(bb, dbgbuf);
        logdbg("pawnAttacks[BLACK][%u]=\n%s", i, dbgbuf);
#endif
    }

    // Initialise en-passant masks
    memset(epMask, 0, sizeof(epMask));

    for (f = FILEA; f <= FILEH; f++) {
        bb = 0ULL;
        setIfOnBoard(f - 1, RANK5, bb);
        setIfOnBoard(f + 1, RANK5, bb);
        epMask[WHITE][fileRankOffset(f, RANK5)] = bb;

        bb = 0ULL;
        setIfOnBoard(f - 1, RANK4, bb);
        setIfOnBoard(f + 1, RANK4, bb);
        epMask[BLACK][fileRankOffset(f, RANK4)] = bb;
    }

    // Initialise knightAttacks
    memset(knightAttacks, 0, sizeof(knightAttacks));

    for (i = 0; i < MAXSQUARES; i++) {
        bb = 0;
        f = offsetFile(i);
        r = offsetRank(i);
        setIfOnBoard(f - 2, r + 1, bb);
        setIfOnBoard(f - 1, r + 2, bb);
        setIfOnBoard(f + 1, r + 2, bb);
        setIfOnBoard(f + 2, r + 1, bb);
        setIfOnBoard(f + 2, r - 1, bb);
        setIfOnBoard(f + 1, r - 2, bb);
        setIfOnBoard(f - 1, r - 2, bb);
        setIfOnBoard(f - 2, r - 1, bb);
        knightAttacks[i] = bb;

#if defined(DEBUG) && DUMP_KNIGHT_INIT > 0
        dumpBitboard(bb, dbgbuf);
        logdbg("knightAttacks[%u]=\n%s", i, dbgbuf);
#endif
    }

    // Initialise kingAttacks
    memset(kingAttacks, 0, sizeof(kingAttacks));

    for (i = 0; i < MAXSQUARES; i++) {
        bb = 0;
        f = offsetFile(i);
        r = offsetRank(i);
        setIfOnBoard(f - 1, r, bb);
        setIfOnBoard(f - 1, r + 1, bb);
        setIfOnBoard(f, r + 1, bb);
        setIfOnBoard(f + 1, r + 1, bb);
        setIfOnBoard(f + 1, r, bb);
        setIfOnBoard(f + 1, r - 1, bb);
        setIfOnBoard(f, r - 1, bb);
        setIfOnBoard(f - 1, r - 1, bb);
        kingAttacks[i] = bb;

#if defined(DEBUG) && DUMP_KING_INIT > 0
        dumpBitboard(bb, dbgbuf);
        logdbg("kingAttacks[%u]=\n%s", i, dbgbuf);
#endif
    }

    // Initialise file_masks
    memset(fileMasks, 0, sizeof(fileMasks));
    bb = 0x0101010101010101ULL;

    for (i = 0; i < MAXFILES; i++) {
        fileMasks[i] = bb;
        bb <<= 1;

#if defined(DEBUG) && DUMP_FILE_INIT
        dumpBitboard(fileMasks[i], dbgbuf);
        logdbg("fileMasks[%u]=\n%s", i, dbgbuf);
#endif
    }

    notFileA = ~fileMasks[FILEA];
    notFileH = ~fileMasks[FILEH];
    fileAH = fileMasks[FILEA] | fileMasks[FILEH];

    // Initialise rankMasks
    memset(rankMasks, 0, sizeof(rankMasks));
    bb = 0x00000000000000ffULL;

    for (i = 0; i < MAXRANKS; i++) {
        rankMasks[i] = bb;
        bb <<= 8;

#if defined(DEBUG) && DUMP_RANK_INIT
        dumpBitboard(rankMasks[i], dbgbuf);
        logdbg("rankMasks[%u]=\n%s", i, dbgbuf);
#endif
    }

    // Initialise other file/rank/square bitmasks
    rankMask1to4 = rankMasks[RANK1] | rankMasks[RANK2] | rankMasks[RANK3] | rankMasks[RANK4];
    rankMask5to8 = rankMasks[RANK5] | rankMasks[RANK6] | rankMasks[RANK7] | rankMasks[RANK8];
    rankMask1and8 = rankMasks[RANK1] | rankMasks[RANK8];
    rankMask2and7 = rankMasks[RANK2] | rankMasks[RANK7];

    // Initialise file_rank_masks
    for (o = 0; o < MAXSQUARES; o++) {
        bb = 0ULL;

        // left
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(--f, r, bb))
            ;

        // right
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(++f, r, bb))
            ;

        // up
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(f, ++r, bb))
            ;

        // down
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(f, --r, bb))
            ;

        fileRankMasks[o] = bb;
    }

    // Initialise diagMasks
    for (o = 0; o < MAXSQUARES; o++) {
        bb = 0ULL;

        // left/up
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(--f, ++r, bb))
            ;

        // right/up
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(++f, ++r, bb))
            ;

        // left/down
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(--f, --r, bb))
            ;

        // right/down
        f = offsetFile(o);
        r = offsetRank(o);
        while (setIfOnBoard(++f, --r, bb))
            ;

        diagMasks[o] = bb;
    }

    // Initialise connectMasks
    for (o = 0; o < MAXSQUARES; o++) {
        f = offsetFile(o);
        r = offsetRank(o);

        for (o2 = 0; o2 < MAXSQUARES; o2++) {
            bb = 0ULL;
            f2 = offsetFile(o2);
            r2 = offsetRank(o2);

            if (f == f2) {
                if (r < r2 - 1) {
                    // down
                    r2--;

                    while (r < r2)
                        bb |= fileRankBit(f2, r2--);
                } else if (r > r2 + 1) {
                    // up
                    r2++;

                    while (r > r2)
                        bb |= fileRankBit(f2, r2++);
                }
            } else if (r == r2) {
                if (f < f2 - 1) {
                    // left
                    f2--;

                    while (f < f2)
                        bb |= fileRankBit(f2--, r2);
                } else if (f > f2 + 1) {
                    // right
                    f2++;

                    while (f > f2)
                        bb |= fileRankBit(f2++, r2);
                }
            } else if (abs(f - f2) == abs(r - r2)) {
                if (f < f2 - 1 && r > r2 + 1) {
                    // left/up
                    f2--;
                    r2++;

                    while (f < f2 &&r > r2)
                        bb |= fileRankBit(f2--, r2++);
                } else if (f > f2 + 1 && r > r2 + 1) {
                    // right/up
                    f2++;
                    r2++;

                    while (f > f2 && r > r2)
                        bb |= fileRankBit(f2++, r2++);
                } else if (f < f2 - 1 && r < r2 - 1) {
                    // left/down
                    f2--;
                    r2--;

                    while (f < f2 && r < r2)
                        bb |= fileRankBit(f2--, r2--);
                } else if (f > f2 + 1 && r < r2 - 1) {
                    // right/down
                    f2++;
                    r2--;

                    while (f > f2 && r < r2)
                        bb |= fileRankBit(f2++, r2--);
                }
            }

            connectMasks[o][o2] = bb;
        }
    }

    // Initialise pinnedDirs
    for (o = 0; o < MAXSQUARES; o++) {
        f = offsetFile(o);
        r = offsetRank(o);

        for (o2 = 0; o2 < MAXSQUARES; o2++) {
            f2 = offsetFile(o2);
            r2 = offsetRank(o2);

            if (f == f2) {
                // Same file
                if (r < r2)
                    pinnedDirs[o][o2] = +8;
                else if (r > r2)
                    pinnedDirs[o][o2] = -8;
            } else if (r == r2) {
                // Same rank
                if (f < f2)
                    pinnedDirs[o][o2] = +1;
                else if (f > f2)
                    pinnedDirs[o][o2] = -1;
            } else if (abs(f - f2) == abs(r - r2)) {
                // Same diagonal
                if (f > f2 && r < r2)
                    pinnedDirs[o][o2] = +7;
                else if (f < f2 && r < r2)
                    pinnedDirs[o][o2] = +9;
                else if (f < f2 && r > r2)
                    pinnedDirs[o][o2] = -7;
                else if (f > f2 && r > r2)
                    pinnedDirs[o][o2] = -9;
            }
        }
    }

    // Initialise original squares
    memset(originalSquares, 0, sizeof(originalSquares));
    originalSquares[WHITE][PAWN] = offsetBit(A2) | offsetBit(B2) | offsetBit(C2) | offsetBit(D2)
                                   | offsetBit(E2) | offsetBit(F2) | offsetBit(G2) | offsetBit(H2);
    originalSquares[WHITE][ROOK] = offsetBit(A1) | offsetBit(H1);
    originalSquares[WHITE][KNIGHT] = offsetBit(B1) | offsetBit(G1);
    originalSquares[WHITE][BISHOP] = offsetBit(C1) | offsetBit(F1);
    originalSquares[WHITE][QUEEN] = offsetBit(D1);
    originalSquares[WHITE][KING] = offsetBit(E1);
    originalSquares[BLACK][PAWN] = offsetBit(A7) | offsetBit(B7) | offsetBit(C7) | offsetBit(D7)
                                   | offsetBit(E7) | offsetBit(F7) | offsetBit(G7) | offsetBit(H7);
    originalSquares[BLACK][ROOK] = offsetBit(A8) | offsetBit(H8);
    originalSquares[BLACK][KNIGHT] = offsetBit(B8) | offsetBit(G8);
    originalSquares[BLACK][BISHOP] = offsetBit(C8) | offsetBit(F8);
    originalSquares[BLACK][QUEEN] = offsetBit(D8);
    originalSquares[BLACK][KING] = offsetBit(E8);
    rookSquares = offsetBit(A1) | offsetBit(H1) | offsetBit(A8) | offsetBit(H8);
    kingSquares = offsetBit(E1) | offsetBit(E8);

    return true;
}

#ifdef DEBUG
//
// Dump bits of data to log
//
void testData() {
    int i, j;

    for (i = 0; i < MAXSQUARES; i++)
        for (j = 0; j < MAXSQUARES; j++) {
            string s = Util::formatBB(connectMasks[i][j]);
            LOGDBG << "connectMasks[" <<
                char(offsetFile(i) + 'a') << char(offsetRank(i) + '1') << "][" <<
                char(offsetFile(j) + 'a') <<  char(offsetRank(j) + '1') << "]=" << s;
        }
}
#endif // DEBUG
}   // namespace ChessCore
