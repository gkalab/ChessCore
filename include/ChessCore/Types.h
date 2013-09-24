//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Types.h: Basic data type definitions.
//

#pragma once

#include <climits>

namespace ChessCore {
typedef int error_t;

#ifdef _DEBUG
#define DEBUG_TRUE  true
#define DEBUG_FALSE false
#else // !_DEBUG
#define DEBUG_TRUE  false
#define DEBUG_FALSE true
#endif // _DEBUG

//
// Bit board representation:
//
// r|n|b|q|k|b|n|r 8
// p|p|p|p|p|p|p|p 7
// .|.|.|.|.|.|.|. 6
// .|.|.|.|.|.|.|. 5
// .|.|.|.|.|.|.|. 4
// .|.|.|.|.|.|.|. 3
// P|P|P|P|P|P|P|P 2
// R|N|B|Q|K|B|N|R 1
// a b c d e f g h
//
// uint64_t represents the position of a single piece type:
//
// bit 63|62|61|60|59|58|57|56|55|54|53|52|51|50|49|48
// sqr H8|G8|F8|E8|D8|C8|B8|A8|H7|G7|F7|E7|D7|C7|B7|A7
//
// bit 47|46|45|44|43|42|41|40|39|38|37|36|35|34|33|32
// sqr H6|G6|F6|E6|D6|C6|B6|A6|H5|G5|F5|E5|D5|C5|B5|A5
//
// bit 31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16
// sqr H4|G4|F4|E4|D4|C4|B4|A4|H3|G3|F3|E3|D3|C3|B3|A3
//
// bit 15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00
// sqr H2|G2|F2|E2|D2|C2|B2|A2|H1|G1|F1|E1|D1|C1|B1|A1
//

typedef int Square;

// Squares
enum {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1, A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3, A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5, A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7, A8, B8, C8, D8, E8, F8, G8, H8,
    MAXSQUARES
};

typedef int BoardFile;

// Files
enum {
    FILEA = 0, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH, MAXFILES
};

typedef int BoardRank;

// Ranks
enum {
    RANK1 = 0, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8, MAXRANKS
};

// Piece (without colour info)
typedef uint8_t Piece;

// Pieces
enum {
    EMPTY = 0, ALLPIECES = 0, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, MAXPIECES
};

// Colour
typedef uint8_t Colour;

// Colours
enum {
    WHITE = 0x0, BLACK = 0x1, PIECE_MASK = 0x7, COLOUR_MASK = 0x1, MAXCOLOURS = 2
};

// Combined Piece and Colour (bit-4 clear for white, set for black).
typedef uint8_t PieceColour;

// Move mask, used during move generation
struct MoveMask {
    uint64_t bitmask;
    uint64_t filemask;
    uint64_t diag1mask;
    uint64_t diag2mask;
};

// File/Rank to offset
inline Square fileRankOffset(BoardFile file, BoardRank rank) {
    return (rank << 3) + file;
}

// Offset to file
inline BoardFile offsetFile(Square offset) {
    return (BoardFile)(offset & 7);
}

// Offset to rank
inline BoardRank offsetRank(Square offset) {
    return (BoardRank)(offset >> 3);
}

// Bit of an offset
inline uint64_t offsetBit(Square offset) {
    return 1ULL << offset;
}

// Bit of a file/rank
inline uint64_t fileRankBit(BoardFile file, BoardRank rank) {
    return 1ULL << ((rank << 3) + file);
}

// Is file/rank a light square?
inline bool isLightSq(BoardFile file, BoardRank rank) {
    return (file & 1) != (rank & 1);
}

// Is file/rank a dark square?
inline bool isDarkSq(BoardFile file, BoardRank rank) {
    return (file & 1) == (rank & 1);
}

// Is offset a light square?
inline bool isLightSqOffset(Square offset) {
    return isLightSq(offsetFile(offset), offsetRank(offset));
}

// Is file/rank a dark square?
inline bool isDarkSqOffset(Square offset) {
    return isDarkSq(offsetFile(offset), offsetRank(offset));
}

// Move/Colour to Halfmove
inline unsigned toHalfMove(unsigned move, Colour colour) {
    return (move * 2) - (colour == BLACK ? 0 : 1);
}

// Halfmove to move
inline unsigned toMove(unsigned halfmove) {
    return (halfmove + 1) / 2;
}

// Halfmove to colour
inline Colour toColour(unsigned halfmove) {
    return (halfmove & 1) == 1 ? WHITE : BLACK;
}

// Halfmove to opponent colour
inline Colour toOppositeColour(unsigned halfmove) {
    return (halfmove & 1) == 1 ? BLACK : WHITE;
}

// Create PieceColour from Piece and Colour
inline PieceColour toPieceColour(Piece piece, Colour colour) {
    return piece | (colour << 7);
}

// Other colour
inline Colour flipColour(Colour colour) {
    return colour ^ COLOUR_MASK;
}

// Piece with opposite colour
inline PieceColour flipPieceColour(PieceColour piece) {
    return piece ^ (1 << 7);
}

inline Piece pieceOnly(PieceColour piece) {
    return piece & PIECE_MASK;
}

inline Colour pieceColour(PieceColour piece) {
    return (piece >> 7) & COLOUR_MASK;
}

// 0-based index of a piece in an array, in the following order:
// wp, wr, wn, wb, wq, wk, bp, br, bn, bb, bq, bk
inline unsigned pieceIndex(PieceColour piece) {
    return (pieceOnly(piece) - 1) + (pieceColour(piece) == BLACK ? 6 : 0);
}

inline unsigned pieceIndex(Piece piece, Colour colour) {
    return ((unsigned)piece - 1) + (colour == BLACK ? 6 : 0);
}

inline int16_t toInt16(int32_t value) {
    if (value < SHRT_MIN)
        return SHRT_MIN;
    else if (value > SHRT_MAX)
        return SHRT_MAX;

    return (int16_t)value;
}

inline int16_t toInt16(int64_t value) {
    if (value < SHRT_MIN)
        return SHRT_MIN;
    else if (value > SHRT_MAX)
        return SHRT_MAX;

    return (int16_t)value;
}
} // namespace ChessCore
