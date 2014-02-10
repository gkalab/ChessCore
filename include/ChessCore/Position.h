//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Position.h: Position class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Move.h>
#include <ChessCore/Blob.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Util.h>

namespace ChessCore {
// UnmakeMoveInfo holds the information from Position::makeMove()
// used to unmake the move in Position::unmakeMove()
struct UnmakeMoveInfo {
    uint64_t hashKey;   // Position::m_hashKey
    Move lastMove;      // Position::m_lastMove
    uint16_t hmclock;   // Position::m_hmclock
    uint8_t flags;      // Position::m_flags
    Piece captured;     // Piece captured by last move (if capture)
    uint8_t ep;         // Position::m_ep
};

class CHESSCORE_EXPORT Position {
private:
    static const char *m_classname;

public:
    // Position flags
    enum {
        FL_NONE         = 0x00, // None
        FL_WCASTLE_KS   = 0x01, // White can castle kingside
        FL_WCASTLE_QS   = 0x02, // White can castle queenside
        FL_BCASTLE_KS   = 0x04, // Black can castle kingside
        FL_BCASTLE_QS   = 0x08, // Black can castle queenside
        FL_EP_MOVE      = 0x10, // m_ep is valid
        FL_INCHECK      = 0x20, // Side to move is in check
        FL_INDBLCHECK   = 0x40, // Side to move is in double check

        FL_WCASTLE = FL_WCASTLE_KS | FL_WCASTLE_QS,
        FL_BCASTLE = FL_BCASTLE_KS | FL_BCASTLE_QS,
        FL_CASTLE = FL_WCASTLE | FL_BCASTLE,

        // When duplicating a position, some flags should be preserved, while
        // others reset
        FL_PRESERVE = FL_CASTLE
    };

    // Used to compare positions to see what differs
    enum Difference {
        DIFF_NONE, // No difference
        DIFF_PIECES,
        DIFF_BOARD,
        DIFF_HASH,
        DIFF_PLY,
        DIFF_FLAGS,
        DIFF_EP,
        DIFF_HMCLOCK,
        DIFF_LAST_MOVE
    };

    // Used to detect illegal positions
    enum Legal {
        LEGAL,                          // Position/FEN is legal
        ILLPOS_WHITE_ONE_KING,          // White must have exactly one king
        ILLPOS_BLACK_ONE_KING,          // Black must have exactly one king
        ILLPOS_WHITE_TOO_MANY_PIECES,   // White has too many pieces
        ILLPOS_BLACK_TOO_MANY_PIECES,   // Black has too many pieces
        ILLPOS_WHITE_CASTLE_KING_MOVED, // White cannot castle as the king has moved
        ILLPOS_BLACK_CASTLE_KING_MOVED, // Black cannot castle as the king has moved
        ILLPOS_WHITE_CASTLE_KS_ROOK_MOVED, // White cannot castle kingside as the rook has moved
        ILLPOS_WHITE_CASTLE_QS_ROOK_MOVED, // White cannot castle queenside as the rook has moved
        ILLPOS_BLACK_CASTLE_KS_ROOK_MOVED, // Black cannot castle kingside as the rook has moved
        ILLPOS_BLACK_CASTLE_QS_ROOK_MOVED, // Black cannot castle queenside as the rook has moved
        ILLPOS_EP_NO_PAWN,              // Enpassant is invalid as the pawn is not present
        ILLPOS_EP_NOT_EMPTY_BEHIND_PAWN, // Enpassant is invalid as squares behind pawn are not empty
        ILLPOS_SIDE_TO_MOVE_GIVING_CHECK, // The side to move is giving check
        ILLFEN_WRONG_NUMBER_OF_FIELDS,  // FEN must have four or six fields
        ILLFEN_PIECE_DIGIT_INVALID,     // Piece placement digit is invalid
        ILLFEN_PIECE_CHARACTER_INVALID, // Piece placement character is invalid
        ILLFEN_ACTIVE_COLOUR_INVALID,   // Active colour is invalid
        ILLFEN_INVALID_CASTLING_CHARACTER, // Invalid castling availability character
        ILLFEN_INVALID_ENPASSANT_FILE,  // Invalid en-passant file
        ILLFEN_INVALID_ENPASSANT_RANK,  // Invalid en-passant rank
        ILLFEN_INVALID_HALFMOVE_CLOCK,  // Invalid halfmove clock
        ILLFEN_INVALID_FULLMOVE_NUMBER, // Invalid fullmove number
        ILLBLOB_WRONG_SIZE,             // Invalid binary object size
        ILLBLOB_DECODE_FAIL             // Error decoding binary object
    };

protected:
    // Index into m_hashCastle
    enum {
        HASH_WCASTLE_KS,
        HASH_WCASTLE_QS,
        HASH_BCASTLE_KS,
        HASH_BCASTLE_QS,
    };

    // Random numbers used to generate position hash
    static uint64_t m_hashPiece[768];
    static uint64_t m_hashCastle[4];
    static uint64_t m_hashEnPassant[8];
    static uint64_t m_hashTurn;

    // Normal game starting position (initialised on demand)
    static Position m_starting;
    static bool m_startingInit;
    static void initStarting();

    // Piece bitboards.  ALLPIECES offset (0) holds the bitboard for all pieces of that colour
    uint64_t m_pieces[MAXCOLOURS][MAXPIECES];

    /*
     * Board layout:
     * +--+--+--+--+--+--+--+--+
     * |56|57|58|59|60|61|62|63|
     * +--+--+--+--+--+--+--+--+
     * |48|49|50|51|52|53|54|55|
     * +--+--+--+--+--+--+--+--+          +7  +8  +9
     * |40|41|42|43|44|45|46|47|            \  |  /
     * +--+--+--+--+--+--+--+--+             \ | /
     * |32|33|34|35|36|37|38|39|         -1---   ---+1
     * +--+--+--+--+--+--+--+--+             / | \
     * |24|25|26|27|28|29|30|31|            /  |  \
     * +--+--+--+--+--+--+--+--+          -9  -8  -7
     * |16|17|18|19|20|21|22|23|
     * +--+--+--+--+--+--+--+--+
     * |08|09|10|11|12|13|14|15|
     * +--+--+--+--+--+--+--+--+
     * |00|01|02|03|04|05|06|07|
     * +--+--+--+--+--+--+--+--+
     */
    PieceColour m_board[MAXSQUARES];

    uint64_t m_hashKey;         // Zobrist key
    uint16_t m_ply;             // Halfmoves into game (0 = start)
    uint8_t m_flags;            // FL_xxx
    uint8_t m_ep;               // Only valid if flag FL_EP_MOVE set
    uint16_t m_hmclock;         // Halfmoves since pawn move or piece capture
    Move m_lastMove;            // Last move made

public:
    //
    // Constructors
    //
    Position() {
        // Don't initialise the object; if the user wants it initialised then
        // they can call init() themselves.
    }

    inline Position(const Position &other) {
        set(other);
    }

    inline Position(const Position *other) {
        set(other);
    }

    //
    // Initialisation
    //
    void init();
    void set(const Position &other);
    void set(const Position *other);

    bool equals(const Position &other, bool includeLastMove = false) const;

    Difference whatDiffers(const Position &other) const;

    inline uint16_t ply() const {
        return m_ply;
    }

    inline void setPly(uint16_t ply) {
        m_ply = ply;
    }

    inline bool wtm() const {
        return (m_ply & 1) == 0;
    }

    inline uint8_t flags() const {
        return m_flags;
    }

    inline void setFlags(uint8_t flags) {
        m_flags |= flags;
    }

    inline void clearFlags(uint8_t flags) {
        m_flags &= ~flags;
    }

    inline uint8_t ep() const {
        return m_ep;
    }

    inline void setEp(uint8_t ep) {
        m_ep = ep;
    }

    inline uint16_t hmclock() const {
        return m_hmclock;
    }

    inline void setHmclock(uint16_t hmclock) {
        m_hmclock = hmclock;
    }

    const Move lastMove() const {
        return m_lastMove;
    }

    /**
     * Initialise a position to its default stating position.
     */
    void setStarting();

    /**
     * Test if a position is at the default starting position.
     */
    bool isStarting() const;

    /**
     * Make a move in the position.
     *
     * @param move the move to make.
     * @param umi where unmake move info is stored, which will be used
     * by unmakeMove().
     *
     * @return true if the move was successfully made, else false.
     */
    bool makeMove(Move move, UnmakeMoveInfo &umi);

    /**
     * Make a null move in the position.
     *
     * @param umi where unmake move info is stored, which will be used
     * by unmakeMove().
     *
     * @return true if the move was successfully made, else false.
     */
    bool makeNullMove(UnmakeMoveInfo &umi);

    /**
     * Unmake a move; reverse the effect of makeMove() and makeNullMove().
     *
     * @param umi the information created by makeMove() or makeNullMove()
     * used to restore the position to it's previous state.
     *
     * @return true if the move was successfully made.
     */
    bool unmakeMove(const UnmakeMoveInfo &umi);

    /**
     * Return the formatted move number for the position.
     */
    std::string moveNumber() const;

    /**
     * Get a list of pinned pieces in the current position.
     *
     * @param pinned where to store pinned information:
     *   pinned.piece = pinned piece type.
     *   pinned.from = pinned piece square (offset).
     *   pinned.to = pinning piece square (offset).
     * @param epCapPinned Where to store a bitboard of pawns which are not
     *   technically pinned, however they are pinned in as much as they cannot
     *   capture en-passant (for example if a rook is attacking the king along
     *   that rank). These ep-pinned pawns won't appear in the 'pinned' array.
     * @param stm if true then find pinned pieces for the side that is to
     * move, else if false for the side that just moved.
     *
     * @return the number of pinned pieces found.
     */
    unsigned findPinned(Move *pinned, uint64_t &epCapPinned, bool stm) const;

    /**
     * Get a bitboard of pinned pieces in the current position.
     *
     * @param pinned where to store a bitboard of pinned pieces.
     * @param epCapPinned Where to store a bitboard of pawns which are not
     *   technically pinned, however they are pinned in as much as they cannot
     *   capture en-passant (for example if a rook is attacking the king along
     *   that rank). These ep-pinned pawns won't appear in the 'pinned' bitboard.
     * @param stm if true then find pinned pieces for the side that is to
     * move, else if false for the side that just moved.
     *
     * @return the number of pinned pieces found.
     */
    unsigned findPinned(uint64_t &pinned, uint64_t &epCapPinned, bool stm) const;

    /**
     * Get a list of the pieces attacking the specified square.
     *
     * @param sq The offset of the square.
     * @param moves Where to store the list of moves that attack the square.
     * This may be null.
     * @param stm if true then find the attacking moves for the side that is
     * to move, else if false for the side that just moved.
     *
     * @return the number of pieces attacking the square.
     */
    unsigned attacks(unsigned sq, Move *moves, bool stm) const;

    /**
     * Determine if any pieces attacks the specified square.
     *
     * @param sq The offset of the square.
     * @param stm if true then find the attacking moves for the side that is
     * to move, else if false for the side that just moved.
     * @param removePiece pieces that are removed from the board (temporarily)
     * during the tests.
     *
     * @return true if the square is being attacked, else false.
     */
    bool attacks(unsigned sq, bool stm, uint64_t removePiece = 0ULL) const;

    /**
     * Complete a move by setting the correct flags (including check and mate)
     * and generate the SAN.
     *
     * @param move Move to complete (updated).
     * @param includeMoveNum if true then the move number is added to the returned
     * formatted move, else if false the move is not added.
     *
     * @return The formatted move.
     */
    std::string completeMove(Move &move, bool includeMoveNum);

    /**
     * Set the position from a Forsythe-Edwards Notation string.
     *
     * @param fen string containing FEN.
     *
     * @return LEGAL if the position is legal, else a ILLFEN_xxx value that describes why
     * the FEN is illegal.
     */
    Legal setFromFen(const char *fen);

    /**
     * Set the position from Forsythe-Edwards Notation string fields.
     *
     * @param piecePlacement the piece-placement specifier.
     * @param activeColour the active colour specifier.
     * @param castling the castling specifier.
     * @param epTarget the en-passant specifier.
     * @param halfmoveClock the half-move clock specifier.
     * @param fullmoveNumber the full move number specifier.
     *
     * @return LEGAL if the position is legal, else a ILLFEN_xxx value that describes why
     * the FEN is illegal.
     */
    Legal setFromFen(const char *piecePlacement, const char *activeColour, const char *castling, const char *epTarget,
                     const char *halfmoveClock, const char *fullmoveNumber);

    /**
     * Get the FEN string for the position.
     *
     * @param epd if true then generate an EPD-compatible position (without trailing
     * halfmove clock and side to move).
     *
     * @return FEN string of the position
     */
    std::string fen(bool epd = false) const;

    /**
     * Set the position from a binary object.
     *
     * @param blob The Blob object containing the position representation.
     *
     * @return LEGAL if the position is legal, else a ILLFEN_xxx value that describes why
     * the FEN is illegal.
     */
    Legal setFromBlob(const Blob &blob);

    /**
     * Get the binary representation of the position.
     *
     * @param blob The Blob object to store the binary representation in.
     *
     * @return true if the position was stored successfully, else false.
     */
    bool blob(Blob &blob) const;

    /**
     * Set to a random position.  This is used for testing.
     */
    void setRandom();

    inline void setPieceAll(Colour col, Piece pce, Square sq) {
        setPieceBB(col, pce, offsetBit(sq));
        m_board[sq] = toPieceColour(pce, col);
    }

    inline void clearPieceAll(Square sq) {
        clearPieceBB(pieceColour(m_board[sq]), pieceOnly(m_board[sq]), ~offsetBit(sq));
        m_board[sq] = EMPTY;
    }

private:
    inline void setPieceBB(Colour col, Piece pce, Square sq) {
        setPieceBB(col, pce, offsetBit(sq));
    }

    inline void setPieceBB(Colour col, Piece pce, uint64_t sqBit) {
        m_pieces[col][pce] |= sqBit;
        m_pieces[col][ALLPIECES] |= sqBit;
    }


    inline void clearPieceBB(Square sq) {
        clearPieceBB(pieceColour(m_board[sq]), pieceOnly(m_board[sq]), ~offsetBit(sq));
    }

    inline void clearPieceBB(Colour col, Piece pce, Square sq) {
        clearPieceBB(col, pce, ~offsetBit(sq));
    }

    inline void clearPieceBB(Colour col, Piece pce, uint64_t notSqBit) {
        m_pieces[col][pce] &= notSqBit;
        m_pieces[col][ALLPIECES] &= notSqBit;
    }

public:
    inline PieceColour piece(Square sq) const {
        return m_board[sq];
    }

    inline void piece(Square sq, Piece &piece, Colour &colour) const {
        piece = pieceOnly(m_board[sq]);
        colour = pieceColour(m_board[sq]);
    }

    /**
     * Count the number of pieces for a specified colour.
     *
     * @param col the colour to count.
     * @param pce the piece to count (if ALLPIECES then all pieces are counted).
     *
     * @return the number of pieces of the specified colour.
     */
    inline uint32_t pieceCount(Colour col, Piece pce) const {
        return popcnt(m_pieces[col][pce]);
    }

    /**
     * Get the number of light/dark squared bishops.
     *
     * @param col the colour of the bishops to count.
     * @param lightCount where to store the number of light-squares bishops.
     * @param darkCount where to store the number of dark-squared bishops.
     */
    void bishopSquares(Colour col, uint32_t &lightCount, uint32_t &darkCount) const;

    /**
     * Determine if the position is illegal.
     *
     * @return LEGAL if the position is legal, else a ILLPOS_xxx value that describes why
     * the position is illegal.
     */
    Legal isLegal() const;

    /**
     * @return The position's hash key.  This can be zero.
     */
    inline uint64_t hashKey() const {
        return m_hashKey;
    }

    /**
     * Forcably regenerate the hash key.
     */
    void regenerateHashKey() {
        m_hashKey = generateHashKey();
    }

    uint64_t generateHashKey() const;

    static inline uint64_t pieceHash(Colour colour, Piece piece, Square square) {
        return m_hashPiece[(64 * ((colour * 6) + (piece - 1))) + square];
    }

    static inline uint64_t pieceHash(PieceColour piece, Square square) {
        return pieceHash(pieceColour(piece), pieceOnly(piece), square);
    }

    /**
     * Dump the position to astd::string     */
    std::string dump(bool lowlevel = false) const;

#ifdef DEBUG
    /**
     * Sanity check the position to ensure it's valid.
     */
    bool sanityCheck() const;
#endif // DEBUG

    /**
     * Generate moves/captures in the position.
     *
     * @param moves where to store the generated moves.
     *
     * @return the number of moves generated.
     */
    inline unsigned genMoves(Move *moves) const {
        if ((m_flags & FL_INCHECK) == 0)
            return genNonEvasions(moves);
        else
            return genEvasions(moves);
    }

    /**
     * Generate non-check-evasion moves.
     *
     * @param moves where to store the generated moves.
     *
     * @return the number of moves generated.
     */
    unsigned genNonEvasions(Move *moves) const;

    /**
     * Generate check evasion moves.
     *
     * @param moves where to store the generated moves.
     *
     * @return the number of moves generated.
     */
    unsigned genEvasions(Move *moves) const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Position &pos);
};

} // namespace ChessCore
