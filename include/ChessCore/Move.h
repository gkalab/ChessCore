//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Move.h: Move class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <vector>

namespace ChessCore {
class Position;                 // Forward

struct CHESSCORE_EXPORT Move {
private:
    static const char *m_classname;

public:
    // Move flags
    enum {
        FL_NONE             = 0x0000,   // No flag
        FL_CASTLE_KS        = 0x0001,   // Castled kingside
        FL_CASTLE_QS        = 0x0002,   // Castled queenside
        FL_EP_MOVE          = 0x0004,   // Double pawn move (en-passant capture possible)
        FL_EP_CAP           = 0x0008,   // En-passant capture
        FL_PROMOTION        = 0x0010,   // Pawn promotion
        FL_CAPTURE          = 0x0020,   // Capture
        FL_CHECK            = 0x0040,   // Move gives check
        FL_DOUBLE_CHECK     = 0x0080,   // Move gives double check
        FL_MATE             = 0x0100,   // Move gives mate
        FL_DRAW             = 0x0200,   // Move cause draw
        FL_ILLEGAL          = 0x0400,   // Move is illegal
        FL_CAN_MOVE         = 0x0800    // Pinned piece can move
    };

protected:
    // +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
    // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |           flags (FL_xxx)                | prom   | piece  |   from offset   |    to offset    |
    // |                 (14 bits)               |(3 bits)|(3 bits)|     (6 bits)    |     (6 bits)    |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    uint32_t m_flags  : 14;
    uint32_t m_prom   : 3;
    uint32_t m_piece  : 3;
    uint32_t m_from   : 6;
    uint32_t m_to     : 6;

public:
    Move() = default;

    Move(const Move &other) = default;

    inline Move(const Move *other) :
		m_flags(other->m_flags),
		m_prom(other->m_prom),
		m_piece(other->m_piece),
		m_from(other->m_from),
		m_to(other->m_to)
	{
    }

    inline Move(Piece piece, Square from, Square to) :
		m_flags(0),
		m_prom(0),
		m_piece(piece),
		m_from(from),
		m_to(to)
	{
    }

    inline Move(unsigned flags, Piece piece, Square from, Square to) :
		m_flags(flags),
		m_prom(0),
		m_piece(piece),
		m_from(from),
		m_to(to)
	{
    }

    inline Move(uint32_t intValue) {
        set(intValue);
    }

    inline void init() {
        m_flags = 0;
        m_prom = 0;
        m_piece = 0;
        m_from = 0;
        m_to = 0;
    }

    inline void set(Piece piece, Square from, Square to) {
        m_flags = 0;
        m_prom = 0;
        m_piece = piece;
        m_from = from;
        m_to = to;
    }

    inline void set(unsigned flags, Piece piece, Square from, Square to) {
        m_flags = flags;
        m_prom = 0;
        m_piece = piece;
        m_from = from;
        m_to = to;
    }

    inline void set(unsigned flags, Piece prom, Piece piece, Square from, Square to) {
        m_flags = flags;
        m_prom = prom;
        m_piece = piece;
        m_from = from;
        m_to = to;
    }

    inline void set(const Move &other) {
        m_flags = other.m_flags;
        m_prom = other.m_prom;
        m_piece = other.m_piece;
        m_from = other.m_from;
        m_to = other.m_to;
    }

    inline void set(const Move *other) {
        m_flags = other->m_flags;
        m_prom = other->m_prom;
        m_piece = other->m_piece;
        m_from = other->m_from;
        m_to = other->m_to;
    }

    inline uint32_t intValue() const {
        return *((uint32_t *)this);
    }

    inline void set(uint32_t intValue) {
        *((uint32_t *)this) = intValue;
    }

    inline Square to() const {
        return m_to;
    }

    inline void setTo(Square to) {
        m_to = to;
    }

    inline Square from() const {
        return m_from;
    }

    inline void setFrom(Square from) {
        m_from = from;
    }

    inline Piece piece() const {
        return m_piece;
    }

    inline void setPiece(Piece piece) {
        m_piece = piece;
    }

    inline Piece prom() const {
        return m_prom;
    }

    inline void setProm(Piece prom) {
        m_prom = prom;
    }

    inline unsigned flags() const {
        return m_flags;
    }

    inline void setFlags(unsigned flags) {
        m_flags |= flags;
    }

    inline void clearFlags(unsigned flags) {
        m_flags &= ~flags;
    }

    // Individual flag tests
    inline bool isCastleKS() const {
        return (m_flags & FL_CASTLE_KS) != 0;
    }

    inline bool isCastleQS() const {
        return (m_flags & FL_CASTLE_QS) != 0;
    }

    inline bool isCastle() const {
        return (m_flags & (FL_CASTLE_KS | FL_CASTLE_QS)) != 0;
    }

    inline bool isEpMove() const {
        return (m_flags & FL_EP_MOVE) != 0;
    }

    inline bool isEpCap() const {
        return (m_flags & FL_EP_CAP) != 0;
    }

    inline bool isPromotion() const {
        return (m_flags & FL_PROMOTION) != 0;
    }

    inline bool isCapture() const {
        return (m_flags & FL_CAPTURE) != 0;
    }

    inline bool isCheck() const {
        return (m_flags & FL_CHECK) != 0;
    }

    inline bool isDoubleCheck() const {
        return (m_flags & FL_DOUBLE_CHECK) != 0;
    }

    inline bool isMate() const {
        return (m_flags & FL_MATE) != 0;
    }

    inline bool isDraw() const {
        return (m_flags & FL_DRAW) != 0;
    }

    inline bool isIllegal() const {
        return (m_flags & FL_ILLEGAL) != 0;
    }

    inline bool isQuiescent() const {
        return (m_flags & (FL_CAPTURE | FL_PROMOTION)) != 0;
    }

    inline bool isCheckOrPromotion() const {
        return (m_flags & (FL_CHECK | FL_PROMOTION)) != 0;
    }

    inline bool canMove() const {
        return (m_flags & FL_CAN_MOVE) != 0;
    }

    inline static bool isSlidingPiece(Piece piece) {
        return piece == ROOK || piece == BISHOP || piece == QUEEN;
    }

    inline bool isSlidingPiece() const {
        return isSlidingPiece(piece());
    }

	/**
	 * Test if two moves are equal; ignoring flags.
	 */
    inline bool equals(const Move &other) const {
        return
            m_from == other.m_from &&
            m_to == other.m_to &&
            (!isPromotion() || m_prom == other.m_prom);
    }

    inline void setNull() {
        m_flags = 0;
        m_prom = 0;
        m_piece = 0;
        m_from = 0;
        m_to = 0;
    }

    inline bool isNull() const {
        return m_from == 0 && m_to == 0;
    }

    static inline Move nullMove() {
        return Move();
    }

    /**
     * Reverse the move's from and to squares.
     */
    void reverseFromTo();

    /**
     * Generate the Short Algebraic Notation (SAN) for the move.
     *
     * @param pos the position in which this move is being made.
     * @param pieceMap the list of chess pieces to use in the output.  If this
     * is 0, then the default built-in set is used.
     *
     * @return Formatted move string.
     */
    std::string san(const Position &pos, const char *pieceMap = 0) const;

    /**
     * Generate the co-ordinate notation for a move.
     *
     * @param uciCompliant if true then generate in a form suitable for
     * a UCI chess engine.
     *
     * @return Formatted move string.
     */
    std::string coord(bool uciCompliant = false) const;

    /*
     * Parse the move from SAN format.
     *
     * @param position the position in which this move is being made.
     * @param str the move text to parse.
     *
     * @return true if the move text was parsed successfully, else false.
     */
    bool parse(const Position &pos, const std::string &str);

    /**
     * Complete the move by setting the correct flags from the list of generated
     * moves in the position.  If the move is a pawn promotion then the piece is
     * always set as queen.
     *
     * NOTE: Check and Mate flags are not set.
     *
     * @param pos The position in which the move is being made.
     * @param suppressError If true then no error logging is generated if the
     * move fails to complete (i.e. is illegal).  If false then error/debug logs
     * are generated.
     *
     * @return true if the move was completed (is legal), else false if the move
     * is illegal.
     */
    bool complete(const Position &pos, bool suppressError = false);

    //
    // Get a piece from its text representation
    //
    static Piece pieceFromText(char text);

    /**
     * Dump to a string.
     *
     * @return Dump of the move.
     */
    std::string dump(bool includeFlags = true) const;

    /**
     * Dump a vector of Move objects to a string.
     *
     * @param moves The list of moves to dump.
     *
     * @return Dump of the move list.
     */
    static std::string dump(const std::vector<Move> &moves);

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Move &move);
};

class ChessCoreMoveException : public ChessCoreException {
public:
    ChessCoreMoveException(Move move) {
        m_reason = "Illegal move " + move.dump();
    }
};

} // namespace ChessCore
