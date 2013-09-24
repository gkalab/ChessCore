//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Data.h: static data declarations.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {
extern uint64_t magicRookMult[64];
extern int magicRookShift[64];
extern uint64_t magicRookMask[64];
extern int magicRookIndex[64];
extern uint64_t magicRookAtkMasks[0x19000];
extern uint64_t magicBishopMult[64];
extern int magicBishopShift[64];
extern uint64_t magicBishopMask[64];
extern int magicBishopIndex[64];
extern uint64_t magicBishopAtkMasks[0x1480];
extern uint64_t rankAttacks[MAXRANKS][MAXSQUARES];
extern uint64_t pawnAttacks[MAXCOLOURS][MAXSQUARES];
extern uint64_t epMask[MAXCOLOURS][MAXSQUARES];
extern uint64_t knightAttacks[MAXSQUARES];
extern uint64_t kingAttacks[MAXSQUARES];
extern uint64_t fileMasks[MAXFILES];
extern uint64_t rankMasks[MAXRANKS];
extern uint64_t rankMask1to4;
extern uint64_t rankMask5to8;
extern uint64_t rankMask1and8;
extern uint64_t rankMask2and7;
extern uint64_t fileRankMasks[MAXSQUARES];
extern uint64_t diagMasks[MAXSQUARES];
extern uint64_t connectMasks[MAXSQUARES][MAXSQUARES];
extern int8_t pinnedDirs[MAXSQUARES][MAXSQUARES];
extern uint64_t originalSquares[MAXCOLOURS][MAXPIECES];
extern uint64_t rookSquares;
extern uint64_t kingSquares;
extern uint64_t notFileA;
extern uint64_t notFileH;
extern uint64_t fileAH;
extern int8_t pieceSquare[MAXCOLOURS][MAXPIECES - 1][MAXSQUARES];
extern int8_t kingSquareEndgame[MAXCOLOURS][MAXSQUARES];
extern char pieceChars[MAXPIECES];
extern const char *unicodePieces[12];

//
// Initialise data structures
//
extern CHESSCORE_EXPORT bool dataInit();

#ifdef _DEBUG
//
// Dump bits of data to log
//
extern CHESSCORE_EXPORT void testData();
#endif // _DEBUG

} // namespace ChessCore
