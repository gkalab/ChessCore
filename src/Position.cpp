//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Position.cpp: Position class implementation.
//

#include <ChessCore/Position.h>
#include <ChessCore/Bitstream.h>
#include <ChessCore/Rand64.h>
#include <ChessCore/Log.h>

#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <sstream>
#include <memory>

#ifdef DEBUG
#define CHECK_PIECE(col, pce, sq, sqBit) \
    ASSERT(m_pieces[col][pce] & sqBit); \
    ASSERT(m_pieces[col][ALLPIECES] & sqBit); \
    ASSERT(m_board[sq] == toPieceColour(pce, col))

#define CHECK_EMPTY(col, pce, sq, sqBit) \
    ASSERT((m_pieces[col][pce] & sqBit) == 0ULL); \
    ASSERT((m_pieces[col][ALLPIECES] & sqBit) == 0ULL); \
    ASSERT(m_board[sq] == EMPTY)

#else // !DEBUG
#define CHECK_PIECE(col, pce, sq, sqBit) /* empty */
#define CHECK_EMPTY(col, pce, sq, sqBit) /* empty */
#endif // DEBUG

using namespace std;

namespace ChessCore {
const char *Position::m_classname = "Position";

Position Position::m_starting;
bool Position::m_startingInit = false;

void Position::initStarting() {
    m_starting.init();

    for (BoardFile file = FILEA; file <= FILEH; file++) {
        m_starting.setPieceAll(WHITE, PAWN, fileRankOffset(file, RANK2));
        m_starting.setPieceAll(BLACK, PAWN, fileRankOffset(file, RANK7));
    }

    m_starting.setPieceAll(WHITE, ROOK, A1);
    m_starting.setPieceAll(WHITE, KNIGHT, B1);
    m_starting.setPieceAll(WHITE, BISHOP, C1);
    m_starting.setPieceAll(WHITE, QUEEN, D1);
    m_starting.setPieceAll(WHITE, KING, E1);
    m_starting.setPieceAll(WHITE, BISHOP, F1);
    m_starting.setPieceAll(WHITE, KNIGHT, G1);
    m_starting.setPieceAll(WHITE, ROOK, H1);
    m_starting.setPieceAll(BLACK, ROOK, A8);
    m_starting.setPieceAll(BLACK, KNIGHT, B8);
    m_starting.setPieceAll(BLACK, BISHOP, C8);
    m_starting.setPieceAll(BLACK, QUEEN, D8);
    m_starting.setPieceAll(BLACK, KING, E8);
    m_starting.setPieceAll(BLACK, BISHOP, F8);
    m_starting.setPieceAll(BLACK, KNIGHT, G8);
    m_starting.setPieceAll(BLACK, ROOK, H8);

    m_starting.m_ply = 0;
    m_starting.m_flags = FL_WCASTLE_KS | FL_WCASTLE_QS | FL_BCASTLE_KS | FL_BCASTLE_QS;
    m_starting.m_ep = 0;
    m_starting.m_hmclock = 0;
    m_starting.m_lastMove.setNull();
    m_starting.m_hashKey = m_starting.generateHashKey();

    m_startingInit = true;
}

//
// Initialisation
//
void Position::init() {
    memset(m_pieces, 0, sizeof(m_pieces));
    memset(m_board, 0, sizeof(m_board));
    m_hashKey = 0ULL;
    m_ply = 0;
    m_flags = FL_NONE;
    m_ep = 0;
    m_hmclock = 0;
    m_lastMove.setNull();
}

void Position::set(const Position &other) {
    memcpy(m_pieces, other.m_pieces, sizeof(m_pieces));
    memcpy(m_board, other.m_board, sizeof(m_board));
    m_hashKey = other.m_hashKey;
    m_ply = other.m_ply;
    m_flags = other.m_flags;
    m_ep = other.m_ep;
    m_hmclock = other.m_hmclock;
    m_lastMove = other.m_lastMove;
}

void Position::set(const Position *other) {
    memcpy(m_pieces, other->m_pieces, sizeof(m_pieces));
    memcpy(m_board, other->m_board, sizeof(m_board));
    m_hashKey = other->m_hashKey;
    m_ply = other->m_ply;
    m_flags = other->m_flags;
    m_ep = other->m_ep;
    m_hmclock = other->m_hmclock;
    m_lastMove = other->m_lastMove;
}

bool Position::equals(const Position &other, bool includeLastMove /*=false*/) const {
    return
        memcmp(m_pieces, other.m_pieces, sizeof(m_pieces)) == 0 &&
        memcmp(m_board, other.m_board, sizeof(m_board)) == 0 &&
        m_hashKey == other.m_hashKey &&
        m_ply == other.m_ply &&
        m_flags == other.m_flags &&
        m_ep == other.m_ep &&
        m_hmclock == other.m_hmclock &&
        (!includeLastMove || m_lastMove.equals(other.m_lastMove));
}

Position::Difference Position::whatDiffers(const Position &other) const {
    if (memcmp(m_pieces, other.m_pieces, sizeof(m_pieces)) != 0)
        return DIFF_PIECES;

    if (memcmp(m_board, other.m_board, sizeof(m_board)) != 0)
        return DIFF_BOARD;

    if (m_hashKey != other.m_hashKey)
        return DIFF_HASH;

    if (m_ply != other.m_ply)
        return DIFF_PLY;

    if (m_flags != other.m_flags)
        return DIFF_FLAGS;

    if (m_ep != other.m_ep)
        return DIFF_EP;

    if (m_hmclock != other.m_hmclock)
        return DIFF_HMCLOCK;

    if (!m_lastMove.equals(other.m_lastMove))
        return DIFF_LAST_MOVE;

    return DIFF_NONE;
}

void Position::setStarting() {
    if (!m_startingInit)
        initStarting();

    set(m_starting);
}

bool Position::isStarting() const {
    if (!m_startingInit)
        initStarting();

    return equals(m_starting);
}

bool Position::makeMove(Move move, UnmakeMoveInfo &umi) {
    uint64_t fromBit, toBit;
    int count;
    Square from, to;
    Colour moveSide, oppSide;
    Piece pce, capPce;
    bool resetHmclock;

    if (move.isNull()) {
        LOGWRN << "Null move supplied";
        return false;
    }

    umi.hashKey = m_hashKey;
    umi.lastMove = m_lastMove;
    umi.hmclock = m_hmclock;
    umi.flags = m_flags;
    // umi.captured done later
    umi.ep = m_ep;

    pce = move.piece();
    from = move.from();
    to = move.to();
    fromBit = offsetBit(from);
    toBit = offsetBit(to);

    m_hashKey ^= m_hashTurn;

    if (m_flags & FL_EP_MOVE)
        m_hashKey ^= m_hashEnPassant[m_ep];

    m_ply++;
    m_flags &= FL_PRESERVE;
    m_ep = 0;
    m_lastMove.setNull();

    moveSide = toColour(m_ply);
    oppSide = flipColour(moveSide);
    capPce = m_board[to] & PIECE_MASK;

    CHECK_PIECE(moveSide, pce, from, fromBit);

    if (move.isCapture() && !move.isEpCap()) {
        CHECK_PIECE(oppSide, capPce, to, toBit);
        if (capPce == EMPTY) {
            LOGWRN << "Capture square is empty";
            return false;
        }
    } else {
        CHECK_EMPTY(moveSide, pce, to, toBit);
        if (capPce != EMPTY) {
            LOGWRN << "Move-to square is not empty";
            return false;
        }
    }

    setPieceBB(moveSide, pce, toBit);
    m_board[to] = toPieceColour(pce, moveSide);
    clearPieceBB(moveSide, pce, ~fromBit);
    m_board[from] = EMPTY;

    m_hashKey ^= pieceHash(moveSide, pce, from);
    m_hashKey ^= pieceHash(moveSide, pce, to);

    resetHmclock = (pce == PAWN);

    if (move.isCapture()) {
        if (move.isEpCap()) {
            capPce = PAWN;

            if (moveSide == WHITE) {
                to -= 8;
                toBit >>= 8;
            } else {
                to += 8;
                toBit <<= 8;
            }

            CHECK_PIECE(oppSide, PAWN, to, toBit);
            clearPieceBB(oppSide, PAWN, ~toBit);
            m_board[to] = EMPTY;
        } else { // Normal capture
            clearPieceBB(oppSide, capPce, ~toBit);

            // Cancel castling rights if a rook is captured
            if (capPce == ROOK && (toBit & rookSquares) && (m_flags & FL_CASTLE)) {
                if (moveSide == WHITE && (m_flags & FL_BCASTLE)) {
                    if (to == H8 && (m_flags & FL_BCASTLE_KS)) {
                        m_flags &= ~FL_BCASTLE_KS;
                        m_hashKey ^= m_hashCastle[HASH_BCASTLE_KS];
                    } else if (to == A8 && (m_flags & FL_BCASTLE_QS)) {
                        m_flags &= ~FL_BCASTLE_QS;
                        m_hashKey ^= m_hashCastle[HASH_BCASTLE_QS];
                    }
                } else if (moveSide == BLACK && (m_flags & FL_WCASTLE)) {
                    if (to == H1 && (m_flags & FL_WCASTLE_KS)) {
                        m_flags &= ~FL_WCASTLE_KS;
                        m_hashKey ^= m_hashCastle[HASH_WCASTLE_KS];
                    } else if (to == A1 && (m_flags & FL_WCASTLE_QS)) {
                        m_flags &= ~FL_WCASTLE_QS;
                        m_hashKey ^= m_hashCastle[HASH_WCASTLE_QS];
                    }
                }
            }
        }

        m_hashKey ^= pieceHash(oppSide, capPce, to);

        resetHmclock = true;
    } else if (move.isCastleKS()) {
        if (moveSide == WHITE) {
            ASSERT(pce == KING);
            ASSERT(from == E1);
            ASSERT(to == G1);
            CHECK_PIECE(WHITE, ROOK, H1, offsetBit(H1));

            setPieceBB(WHITE, ROOK, offsetBit(F1));
            m_board[F1] = toPieceColour(ROOK, WHITE);
            clearPieceBB(WHITE, ROOK, ~offsetBit(H1));
            m_board[H1] = EMPTY;

            m_hashKey ^= pieceHash(WHITE, ROOK, H1);
            m_hashKey ^= pieceHash(WHITE, ROOK, F1);

            if (m_flags & FL_WCASTLE_KS) {
                m_flags &= ~FL_WCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_KS];
            }

            if (m_flags & FL_WCASTLE_QS) {
                m_flags &= ~FL_WCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_QS];
            }
        } else {
            ASSERT(pce == KING);
            ASSERT(from == E8);
            ASSERT(to == G8);
            CHECK_PIECE(BLACK, ROOK, H8, offsetBit(H8));

            setPieceBB(BLACK, ROOK, offsetBit(F8));
            m_board[F8] = toPieceColour(ROOK, BLACK);
            clearPieceBB(BLACK, ROOK, ~offsetBit(H8));
            m_board[H8] = EMPTY;

            m_hashKey ^= pieceHash(BLACK, ROOK, H8);
            m_hashKey ^= pieceHash(BLACK, ROOK, F8);

            if (m_flags & FL_BCASTLE_KS) {
                m_flags &= ~FL_BCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_KS];
            }

            if (m_flags & FL_BCASTLE_QS) {
                m_flags &= ~FL_BCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_QS];
            }
        }
    } else if (move.isCastleQS()) {
        if (moveSide == WHITE) {
            ASSERT(pce == KING);
            ASSERT(from == E1);
            ASSERT(to == C1);
            CHECK_PIECE(WHITE, ROOK, A1, offsetBit(A1));

            setPieceBB(WHITE, ROOK, offsetBit(D1));
            m_board[D1] = toPieceColour(ROOK, WHITE);
            clearPieceBB(WHITE, ROOK, ~offsetBit(A1));
            m_board[A1] = EMPTY;

            m_hashKey ^= pieceHash(WHITE, ROOK, A1);
            m_hashKey ^= pieceHash(WHITE, ROOK, D1);

            if (m_flags & FL_WCASTLE_KS) {
                m_flags &= ~FL_WCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_KS];
            }

            if (m_flags & FL_WCASTLE_QS) {
                m_flags &= ~FL_WCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_QS];
            }
        } else {
            ASSERT(pce == KING);
            ASSERT(from == E8);
            ASSERT(to == C8);
            CHECK_PIECE(BLACK, ROOK, A8, offsetBit(A8));

            setPieceBB(BLACK, ROOK, offsetBit(D8));
            m_board[D8] = toPieceColour(ROOK, BLACK);
            clearPieceBB(BLACK, ROOK, ~offsetBit(A8));
            m_board[A8] = EMPTY;

            m_hashKey ^= pieceHash(BLACK, ROOK, A8);
            m_hashKey ^= pieceHash(BLACK, ROOK, D8);

            if (m_flags & FL_BCASTLE_KS) {
                m_flags &= ~FL_BCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_KS];
            }

            if (m_flags & FL_BCASTLE_QS) {
                m_flags &= ~FL_BCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_QS];
            }
        }
    } else if (move.isEpMove()) {
        m_ep = offsetFile(to);
        m_flags |= FL_EP_MOVE;
        m_hashKey ^= m_hashEnPassant[m_ep];
    }

    if (move.isPromotion()) {
        // Process promotion
        CHECK_PIECE(moveSide, PAWN, to, toBit);
        ASSERT(move.prom() == QUEEN || move.prom() == ROOK || move.prom() == KNIGHT ||
               move.prom() == BISHOP);

        clearPieceBB(moveSide, PAWN, ~toBit);
        setPieceBB(moveSide, move.prom(), toBit);
        m_board[to] = toPieceColour(move.prom(), moveSide);

        m_hashKey ^= pieceHash(moveSide, PAWN, to);
        m_hashKey ^= pieceHash(moveSide, move.prom(), to);
    } else if (pce == ROOK && (m_flags & FL_CASTLE) && (fromBit & rookSquares)) {
        // Cancel castling rights as rook has moved
        if (moveSide == WHITE && (m_flags & FL_WCASTLE)) {
            if (from == H1 && (m_flags & FL_WCASTLE_KS)) {
                m_flags &= ~FL_WCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_KS];
            } else if (from == A1 && (m_flags & FL_WCASTLE_QS)) {
                m_flags &= ~FL_WCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_QS];
            }
        } else if (moveSide == BLACK && (m_flags & FL_BCASTLE)) {
            if (from == H8 && (m_flags & FL_BCASTLE_KS)) {
                m_flags &= ~FL_BCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_KS];
            } else if (from == A8 && (m_flags & FL_BCASTLE_QS)) {
                m_flags &= ~FL_BCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_QS];
            }
        }
    } else if (pce == KING && !move.isCastle() && (fromBit & kingSquares) != 0ULL) {
        // Cancel castling rights if king moved
        if (moveSide == WHITE) {
            if (m_flags & FL_WCASTLE_KS) {
                m_flags &= ~FL_WCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_KS];
            }

            if (m_flags & FL_WCASTLE_QS) {
                m_flags &= ~FL_WCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_WCASTLE_QS];
            }
        } else {
            if (m_flags & FL_BCASTLE_KS) {
                m_flags &= ~FL_BCASTLE_KS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_KS];
            }

            if (m_flags & FL_BCASTLE_QS) {
                m_flags &= ~FL_BCASTLE_QS;
                m_hashKey ^= m_hashCastle[HASH_BCASTLE_QS];
            }
        }
    }

    // Save capture pce
    umi.captured = capPce;

    // Adjust hmclock
    if (resetHmclock)
        m_hmclock = 0;
    else
        m_hmclock++;

    m_lastMove = move;

    // stm is reversed as we are still making the move...
    count = attacks(lsb(m_pieces[oppSide][KING]), 0, false);
    if (count == 1) {
        m_flags |= FL_INCHECK;
        m_lastMove.setFlags(Move::FL_CHECK);
    } else if (count == 2) {
        m_flags |= FL_INCHECK | FL_INDBLCHECK;
        m_lastMove.setFlags(Move::FL_DOUBLE_CHECK);
    }

#ifdef DEBUG
    ASSERT(attacks(lsb(m_pieces[moveSide][KING]), 0, true) == 0);

    if (!sanityCheck())
        return false;
#endif // DEBUG

    return true;
}

bool Position::makeNullMove(UnmakeMoveInfo &umi) {
    Colour moveSide, oppSide;
    int count;

    umi.hashKey = m_hashKey;
    umi.lastMove = m_lastMove;
    umi.hmclock = m_hmclock;
    umi.flags = m_flags;
    umi.captured = EMPTY;
    umi.ep = m_ep;

    m_hashKey ^= m_hashTurn;

    m_ply++;
    m_flags &= FL_PRESERVE;
    m_ep = 0;
    m_lastMove.setNull();

    moveSide = toColour(m_ply);
    oppSide = flipColour(moveSide);

    // stm is reversed as we are still making the move...
    count = attacks(lsb(m_pieces[oppSide][KING]), 0, false);
    if (count == 1)
        m_flags |= FL_INCHECK;
    else if (count == 2)
        m_flags |= FL_INCHECK | FL_INDBLCHECK;

#ifdef DEBUG
    ASSERT(attacks(lsb(m_pieces[moveSide][KING]), 0, true));

    if (!sanityCheck())
        return false;
#endif // DEBUG

    return true;
}

bool Position::unmakeMove(const UnmakeMoveInfo &umi) {
    uint64_t fromBit, toBit;
    Square from, to;
    Colour moveSide, oppSide;
    Piece pce;

    moveSide = toColour(m_ply);
    oppSide = flipColour(moveSide);

    // If this wasn't a null move (made using makeNullMove()) then restore
    // the position
    if (!m_lastMove.isNull()) {
        pce = m_lastMove.piece();
        from = m_lastMove.from();
        to = m_lastMove.to();
        fromBit = offsetBit(from);
        toBit = offsetBit(to);

        setPieceBB(moveSide, pce, fromBit);
        m_board[from] = toPieceColour(pce, moveSide);
        clearPieceBB(moveSide, pce, ~toBit);
        m_board[to] = EMPTY;

        if (m_lastMove.isCapture()) {
            ASSERT(umi.captured != EMPTY);

            if (m_lastMove.isEpCap()) {
                if (moveSide == WHITE) {
                    to -= 8;
                    toBit >>= 8;
                } else {
                    to += 8;
                    toBit <<= 8;
                }
            }

            setPieceBB(oppSide, umi.captured, toBit);
            m_board[to] = toPieceColour(umi.captured, oppSide);
        }

        if (m_lastMove.isCastleKS()) {
            if (moveSide == WHITE) {
                setPieceBB(WHITE, ROOK, offsetBit(H1));
                m_board[H1] = toPieceColour(ROOK, WHITE);
                clearPieceBB(WHITE, ROOK, ~offsetBit(F1));
                m_board[F1] = EMPTY;
            } else {
                setPieceBB(BLACK, ROOK, offsetBit(H8));
                m_board[H8] = toPieceColour(ROOK, BLACK);
                clearPieceBB(BLACK, ROOK, ~offsetBit(F8));
                m_board[F8] = EMPTY;
            }
        } else if (m_lastMove.isCastleQS()) {
            if (moveSide == WHITE) {
                setPieceBB(WHITE, ROOK, offsetBit(A1));
                m_board[A1] = toPieceColour(ROOK, WHITE);
                clearPieceBB(WHITE, ROOK, ~offsetBit(D1));
                m_board[D1] = EMPTY;
            } else {
                setPieceBB(BLACK, ROOK, offsetBit(A8));
                m_board[A8] = toPieceColour(ROOK, BLACK);
                clearPieceBB(BLACK, ROOK, ~offsetBit(D8));
                m_board[D8] = EMPTY;
            }
        } else if (m_lastMove.isPromotion()) {
            setPieceBB(moveSide, PAWN, fromBit);
            m_board[from] = toPieceColour(PAWN, moveSide);
            clearPieceBB(moveSide, m_lastMove.prom(), ~toBit);
        }
    }

    // Restore values saved before the move was made
    m_hashKey = umi.hashKey;
    m_lastMove = umi.lastMove;
    m_hmclock = umi.hmclock;
    m_flags = umi.flags;
    m_ep = umi.ep;

    m_ply--;

    return true;
}

string Position::moveNumber() const {
    ostringstream oss;

    oss << dec << toMove(ply() + 1) << (toColour(ply() + 1) == WHITE ? "." : "...");
    return oss.str();
}

unsigned Position::findPinned(Move *pinned, uint64_t &epCapPinned, bool stm) const {
    uint64_t bb, toBit, moveBits, oppBits;
    Square o, count, kingOffset, fromOffset, toOffset;
    int moveFile, oppFile;
    Colour moveSide, oppSide;
    Piece pinnedPiece;
    int pinnedDir, pawnMoveDir;
    bool canMove;
    static int epRank[MAXCOLOURS] = { 5, 2 };

    if (stm) {
        oppSide = toColour(m_ply);
        moveSide = flipColour(oppSide);
    } else {
        moveSide = toColour(m_ply);
        oppSide = flipColour(moveSide);
    }

    kingOffset = lsb(m_pieces[moveSide][KING]);
    count = 0;
    pawnMoveDir = (moveSide == WHITE) ? +8 : -8;
    epCapPinned = 0ULL;

    // Queen and Rook attacks
    bb = fileRankMasks[kingOffset] & (m_pieces[oppSide][QUEEN] | m_pieces[oppSide][ROOK]);
    while (bb) {
        toOffset = lsb2(bb, toBit);

        moveBits = connectMasks[kingOffset][toOffset] & m_pieces[moveSide][ALLPIECES];
        oppBits = connectMasks[kingOffset][toOffset] & m_pieces[oppSide][ALLPIECES];

        if (moveBits && oppBits) {
            // Check if this piece is en-passant capture pinned
            if ((m_flags & FL_EP_MOVE) &&
                offsetRank(toOffset) == (moveSide == WHITE ? 4 : 3) &&
                popcnt(moveBits) == 1 && popcnt(oppBits) == 1 &&
                popcnt(moveBits & m_pieces[moveSide][PAWN]) == 1 &&
                popcnt(oppBits & m_pieces[oppSide][PAWN]) == 1) {
                    moveFile = offsetFile(lsb(moveBits));
                    oppFile = offsetFile(lsb(oppBits));
                    if (oppFile == m_ep && abs(moveFile - oppFile) == 1) {
                        epCapPinned |= moveBits;
                    }
            }
            continue; // There is another attacking piece in the way
        }

        if (moveBits && popcnt(moveBits) == 1) {
            // Just one of our pieces between the attacking queen/rook
            fromOffset = lsb(moveBits);
            pinnedPiece = m_board[fromOffset] & PIECE_MASK;
            canMove = false;

            if (pinnedPiece == QUEEN || pinnedPiece == ROOK)
                // Queen and rook can always move from a rank/file attack
                canMove = true;
            else if (pinnedPiece == PAWN) {
                // Pawn can move only if they are 'facing' towards or away from the attack
                // and the square in front of the pawn is empty
                pinnedDir = pinnedDirs[fromOffset][toOffset];

                if (abs(pinnedDir) == 8 && m_board[fromOffset + pawnMoveDir] == EMPTY)
                    canMove = true;
            }

            pinned->set(canMove ? Move::FL_CAN_MOVE : 0, pinnedPiece, fromOffset, toOffset);
            pinned++;
            count++;
        }
    }

    // Queen and Bishop attacks
    bb = diagMasks[kingOffset] & (m_pieces[oppSide][QUEEN] | m_pieces[oppSide][BISHOP]);

    while (bb) {
        toOffset = lsb2(bb, toBit);

        if (connectMasks[kingOffset][toOffset] & m_pieces[oppSide][ALLPIECES])
            continue; // There is another attacking piece in the way

        moveBits = connectMasks[kingOffset][toOffset] & m_pieces[moveSide][ALLPIECES];
        if (moveBits && popcnt(moveBits) == 1) {
            // Just one of our pieces between the attacking queen/bishop
            fromOffset = lsb(moveBits);
            pinnedPiece = m_board[fromOffset] & PIECE_MASK;
            canMove = false;

            if (pinnedPiece == QUEEN || pinnedPiece == BISHOP)
                // Queen and bishop can move from a diagonal attack
                canMove = true;
            else if (pinnedPiece == PAWN) {
                // Pawn can move only if they can capture the attacker
                // or can capture en-passant along the pin line
                pinnedDir = pinnedDirs[fromOffset][toOffset];
                o = fromOffset + pinnedDir;

                if (abs(pinnedDir - pawnMoveDir) == 1 &&
                    (o == toOffset || ((m_flags & FL_EP_MOVE) && o == fileRankOffset(m_ep, epRank[moveSide]))))
                    canMove = true;
            }

            pinned->set(canMove ? Move::FL_CAN_MOVE : 0, pinnedPiece, fromOffset, toOffset);
            pinned++;
            count++;
        }
    }

    return count;
}

unsigned Position::findPinned(uint64_t &pinned, uint64_t &epCapPinned, bool stm) const {
    uint64_t bb, toBit, moveBits, oppBits;
    Colour moveSide, oppSide;
    int moveFile, oppFile;
    int kingOffset, toOffset;
    unsigned count;

    if (stm) {
        oppSide = toColour(m_ply);
        moveSide = flipColour(oppSide);
    } else {
        moveSide = toColour(m_ply);
        oppSide = flipColour(moveSide);
    }

    kingOffset = lsb(m_pieces[moveSide][KING]);
    pinned = 0ULL;
    epCapPinned = 0ULL;
    count = 0;

    // Queen and Rook attacks
    bb = fileRankMasks[kingOffset] & (m_pieces[oppSide][QUEEN] | m_pieces[oppSide][ROOK]);
    while (bb) {
        toOffset = lsb2(bb, toBit);

        moveBits = connectMasks[kingOffset][toOffset] & m_pieces[moveSide][ALLPIECES];
        oppBits = connectMasks[kingOffset][toOffset] & m_pieces[oppSide][ALLPIECES];

        if (moveBits && oppBits) {
            // Check if this piece is en-passant capture pinned
            if ((m_flags & FL_EP_MOVE) &&
                offsetRank(toOffset) == (moveSide == WHITE ? 4 : 3) &&
                popcnt(moveBits) == 1 && popcnt(oppBits) == 1 &&
                popcnt(moveBits & m_pieces[moveSide][PAWN]) == 1 &&
                popcnt(oppBits & m_pieces[oppSide][PAWN]) == 1) {
                    moveFile = offsetFile(lsb(moveBits));
                    oppFile = offsetFile(lsb(oppBits));
                    if (oppFile == m_ep && abs(moveFile - oppFile) == 1) {
                        epCapPinned |= moveBits;
                    }
            }
            continue; // There is another attacking piece in the way
        }

        if (moveBits && popcnt(moveBits) == 1) {
            pinned |= moveBits;
            count++;
        }
    }
    // Queen and Bishop attacks
    bb = diagMasks[kingOffset] & (m_pieces[oppSide][QUEEN] | m_pieces[oppSide][BISHOP]);
    while (bb) {
        toOffset = lsb2(bb, toBit);

        if ((connectMasks[kingOffset][toOffset] & m_pieces[oppSide][ALLPIECES]))
            continue; // There is another attacking piece in the way

        moveBits = connectMasks[kingOffset][toOffset] & m_pieces[moveSide][ALLPIECES];
        if (moveBits && popcnt(moveBits) == 1) {
            // Just one of our pieces between the attacking queen/bishop
            pinned |= moveBits;
            count++;
        }
    }

    return count;
}

unsigned Position::attacks(unsigned sq, Move *moves, bool stm) const {
    uint64_t bb, fromBit;
    Colour moveSide, oppSide, col;
    Piece pce;
    Square fromOffset;
    unsigned count;
    uint8_t flags;

    if (stm) {
        oppSide = toColour(m_ply);
        moveSide = flipColour(oppSide);
    } else {
        moveSide = toColour(m_ply);
        oppSide = flipColour(moveSide);
    }

    count = 0;
    flags = 0;
    pce = pieceOnly(m_board[sq]);
    col = pieceColour(m_board[sq]);

    if (pce != EMPTY && col == oppSide) {
        if (pce == KING)
            flags = Move::FL_CHECK;
        else
            flags = Move::FL_CAPTURE;
    }

    bb = pawnAttacks[oppSide][sq] & m_pieces[moveSide][PAWN];
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if (moves) {
            moves->set(flags, PAWN, fromOffset, sq);
            moves++;
        }

        count++;
    }

    bb = knightAttacks[sq] & m_pieces[moveSide][KNIGHT];
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if (moves) {
            moves->set(flags, KNIGHT, fromOffset, sq);
            moves++;
        }

        count++;
    }

    if (kingAttacks[sq] & m_pieces[moveSide][KING]) {
        fromOffset = lsb(m_pieces[moveSide][KING]);

        if (moves) {
            moves->set(flags, KING, fromOffset, sq);
            moves++;
        }

        count++;
    }

    // Queen and Rook attacks
    bb = fileRankMasks[sq] & (m_pieces[moveSide][QUEEN] | m_pieces[moveSide][ROOK]);
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if ((connectMasks[sq][fromOffset] & (m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES])) == 0ULL) {
            if (moves) {
                moves->set(flags, m_board[fromOffset] & PIECE_MASK, fromOffset, sq);
                moves++;
            }

            count++;
        }
    }

    // Queen and Bishop attacks
    bb = diagMasks[sq] & (m_pieces[moveSide][QUEEN] | m_pieces[moveSide][BISHOP]);
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if ((connectMasks[sq][fromOffset] & (m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES])) == 0ULL) {
            if (moves) {
                moves->set(flags, m_board[fromOffset] & PIECE_MASK, fromOffset, sq);
                moves++;
            }

            count++;
        }
    }

    return count;
}

bool Position::attacks(unsigned sq, bool stm, uint64_t removePiece /*= 0ULL*/) const {
    uint64_t bb, fromBit, pieceBits;
    Colour moveSide, oppSide;
    unsigned fromOffset;

    if (stm) {
        oppSide = toColour(m_ply);
        moveSide = flipColour(oppSide);
    } else {
        moveSide = toColour(m_ply);
        oppSide = flipColour(moveSide);
    }

    if (pawnAttacks[oppSide][sq] & m_pieces[moveSide][PAWN])
        return true;

    if (knightAttacks[sq] & m_pieces[moveSide][KNIGHT])
        return true;

    if (kingAttacks[sq] & m_pieces[moveSide][KING])
        return true;

    pieceBits = (m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES]) & ~removePiece;

    // Queen and Rook attacks
    bb = fileRankMasks[sq] & (m_pieces[moveSide][QUEEN] | m_pieces[moveSide][ROOK]);
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if ((connectMasks[sq][fromOffset] & pieceBits) == 0ULL)
            return true;
    }

    // Queen and Bishop attacks
    bb = diagMasks[sq] & (m_pieces[moveSide][QUEEN] | m_pieces[moveSide][BISHOP]);
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if ((connectMasks[sq][fromOffset] & pieceBits) == 0ULL)
            return true;
    }

    return false;
}

string Position::completeMove(Move &move, bool includeMoveNum) {
    Position posTemp(this);
    UnmakeMoveInfo umi;

    if (!posTemp.makeMove(move, umi)) {
        LOGERR << "Failed to make move " << move.dump();
        return "";
    }

    // Use the lastMove made in the position as check or double check flags might
    // have been set
    move = posTemp.lastMove();

    if (includeMoveNum)
        return posTemp.moveNumber() + " " + move.san(*this);

    return move.san(*this);
}

Position::Legal Position::setFromFen(const char *fen) {
    char *fields[6];
    unsigned numFields;
    shared_ptr<char> copy(strdup(fen), free);

    numFields = Util::splitLine(copy.get(), fields, 6);
    if (numFields == 4) {
        fields[4] = 0;
        fields[5] = 0;
    } else if (numFields != 6) {
        LOGERR << "Expected 4 or 6 fields in the FEN string but got " << numFields;
        return ILLFEN_WRONG_NUMBER_OF_FIELDS;
    }

    return setFromFen(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]);
}

Position::Legal Position::setFromFen(const char *piecePlacement, const char *activeColour, const char *castling,
                                     const char *epTarget, const char *halfmoveClock, const char *fullmoveNumber) {
    const char *fen;
    char *base;
    Colour colour;
    Square sq;
    BoardFile f;
    BoardRank r;
    int i;

    if (piecePlacement == 0 || activeColour == 0 || castling == 0 || epTarget == 0) {
        LOGERR << "First four fields of FEN must be specified";
        return ILLFEN_WRONG_NUMBER_OF_FIELDS;
    }

    init();

    // Field 1: Piece placement data
    fen = piecePlacement;
    f = 0;
    r = 7;

    while (*fen != '\0' && r >= 0) {
        if (*fen == '/') {
            f = 0;
            r--;
        } else if (*fen >= '0' && *fen <= '8') {
            f += (int)(*fen - '0');

            if (f > 8) {
                LOGERR << "FEN piece placement digit character is too large: '" << *fen << "'";
                return ILLFEN_PIECE_DIGIT_INVALID;
            }
        } else {
            for (i = 0; i < MAXPIECES; i++)
                if (pieceChars[i] == toupper(*fen))
                    break;

            if (i == MAXPIECES) {
                LOGERR << "Invalid FEN piece character '" << *fen << "' in FEN";
                return ILLFEN_PIECE_CHARACTER_INVALID;
            }

            colour = isupper(*fen) ? WHITE : BLACK;
            sq = fileRankOffset(f, r);
            setPieceAll(colour, i, sq);
            f++;
        }

        fen++;
    }

    // Field 2: Active colour
    fen = activeColour;

    switch (*fen) {
    case 'w':
    case 'W':
        colour = WHITE;
        break;

    case 'b':
    case 'B':
        colour = BLACK;
        break;

    default:
        LOGERR << "Invalid FEN active colour '" << *fen << "' in FEN";
        return ILLFEN_ACTIVE_COLOUR_INVALID;
    }

    // Field 3: Castling Availability
    fen = castling;

    if (*fen != '-')
        while (*fen != '\0') {
            switch (*fen) {
            case 'K':
                m_flags |= FL_WCASTLE_KS;
                break;

            case 'Q':
                m_flags |= FL_WCASTLE_QS;
                break;

            case 'k':
                m_flags |= FL_BCASTLE_KS;
                break;

            case 'q':
                m_flags |= FL_BCASTLE_QS;
                break;

            default:
                LOGERR << "Invalid FEN castling availability character '" << *fen << "' in FEN";
                return ILLFEN_INVALID_CASTLING_CHARACTER;
            }

            fen++;
        }

    // Field 4: En-passant Target Square
    fen = epTarget;

    if (*fen != '-') {
        if (*fen >= 'a' && *fen <= 'h')
            f = *fen - 'a';
        else if (*fen >= 'A' && *fen <= 'H')
            f = *fen - 'A';
        else {
            LOGERR << "Invalid FEN en-passant file '" << *fen << "' in FEN";
            return ILLFEN_INVALID_ENPASSANT_FILE;
        }

        fen++;

        if ((colour == WHITE && *fen != '6') || (colour == BLACK && *fen != '3')) {
            LOGERR << "Invalid FEN en-passant rank '" << *fen << "' in FEN";
            return ILLFEN_INVALID_ENPASSANT_RANK;
        }

        m_ep = (uint8_t)f;
        m_flags |= FL_EP_MOVE;
    }

    // Field 5: Halfmove Clock
    if (halfmoveClock && fullmoveNumber) {
        fen = halfmoveClock;
        i = (int)strtol(fen, &base, 10);

        if (*base != '\0' || i < 0) {
            LOGERR << "Invalid FEN Halfmove Clock value '" << fen << "' in FEN";
            return ILLFEN_INVALID_HALFMOVE_CLOCK;
        }

        m_hmclock = (uint16_t)i;

        // Field 6: Fullmove Number
        fen = fullmoveNumber;
        i = (int)strtol(fen, &base, 10);

        if (*base != '\0' || i < 0) {
            LOGERR << "Invalid FEN Fullmove Number value '" << fen << "' in FEN";
            return ILLFEN_INVALID_FULLMOVE_NUMBER;
        }

        if (i == 0)
            i = 1; // Common error is have the fullmove number of 0

        // '- 1' as our position.ply means "move number in this position", not
        // "the next move will be..." (as with FEN)...
        m_ply = toHalfMove(i, colour) - 1;
    } else {
        m_ply = toHalfMove(1, colour) - 1;
    }

    Legal legal = isLegal();
    if (legal != LEGAL) {
        LOGERR << "Position is illegal (" << legal << ")";
        return legal;
    }

    unsigned count = attacks(lsb(m_pieces[toOppositeColour(m_ply)][KING]), 0, false);
    if (count == 1)
        m_flags |= FL_INCHECK;
    else if (count == 2)
        m_flags |= FL_INCHECK | FL_INDBLCHECK;

#ifdef DEBUG
    else
        ASSERT(count == 0);
#endif // DEBUG

    m_hashKey = generateHashKey();

    return LEGAL;
}

string Position::fen(bool epd /*=false*/) const {
    Square sq;
    int empty;
    ostringstream oss;

    for (BoardRank rank = RANK8; rank >= RANK1; rank--) {
        empty = 0;

        for (BoardFile file = FILEA; file <= FILEH; file++) {
            sq = fileRankOffset(file, rank);
            Piece pce = piece(sq);

            if (pce == EMPTY)
                empty++;
            else {
                if (empty > 0) {
                    oss << dec << empty;
                    empty = 0;
                }

                Colour col = (pce >> 7);
                pce &= PIECE_MASK;

                if (col == WHITE)
                    oss << pieceChars[pce];
                else
                    oss << (char)tolower(pieceChars[pce]);
            }
        }

        if (empty > 0)
            oss << dec << empty;

        if (rank > 0)
            oss << '/';
    }

    oss << " " << (toColour(m_ply) == WHITE ? 'b' : 'w') << " ";

    if (m_flags & FL_CASTLE) {
        if (m_flags & FL_WCASTLE_KS)
            oss << 'K';

        if (m_flags & FL_WCASTLE_QS)
            oss << 'Q';

        if (m_flags & FL_BCASTLE_KS)
            oss << 'k';

        if (m_flags & FL_BCASTLE_QS)
            oss << 'q';
    } else {
        oss << '-';
    }

    oss << ' ';

    if (m_flags & FL_EP_MOVE) {
        if (toColour(m_ply) == WHITE)
            oss << char(m_ep + 'a') << '3';
        else
            oss << char(m_ep + 'a') << '6';
    } else {
        oss << '-';
    }

    if (!epd)
        oss << " " << dec << m_hmclock << " " << toMove(m_ply + 1);

    return oss.str();
}

Position::Legal Position::setFromBlob(const Blob &blob) {
    //
    // Blob layout:
    //
    // position:        32 bytes (4-bits per piece).
    // to move:         1-bit. 1=white, 0=black.
    // castling rights: 4-bits.  0x8=wks, 0x4=wqs, 0x2=bks, 0x1=bqs.
    // ep-file:         4-bits. 0=none, 1=a-file ... 8=h-file.
    // halfmove clock:  16-bits.
    // fullmove number: 16-bits.
    //
    // Total size: 37-bytes + 1-bit.
    //

    if (blob.length() < 38) {
        LOGERR << "Blob is too small (" << blob.length() << ") to contain position";
        return ILLBLOB_WRONG_SIZE;
    }

    init();

    Bitstream stream(blob);
    uint32_t b;

    // Board
    for (Square sq = 0; sq < 64; sq++) {
        if (!stream.read(b, 4)) {
            LOGERR << "Failed to read position from bitstream";
            return ILLBLOB_DECODE_FAIL;
        }

        if (b == 0)
            continue;

        Colour colour = (b & 0x8) ? BLACK : WHITE;
        Piece piece = b & PIECE_MASK;
        setPieceAll(colour, piece, sq);
    }

    // To move
    if (!stream.read(b, 1)) {
        LOGERR << "Failed to read side-to-move from bitstream";
        return ILLBLOB_DECODE_FAIL;
    }

    bool wtm = (b == 0);

    // Castling rights
    if (!stream.read(b, 4)) {
        LOGERR << "Failed to read castling rights from bitstream";
        return ILLBLOB_DECODE_FAIL;
    }

    if (b & 0x8)
        m_flags |= FL_WCASTLE_KS;

    if (b & 0x4)
        m_flags |= FL_WCASTLE_QS;

    if (b & 0x2)
        m_flags |= FL_BCASTLE_KS;

    if (b & 0x1)
        m_flags |= FL_BCASTLE_QS;

    // En-passant file
    if (!stream.read(b, 4)) {
        LOGERR << "Failed to read en-passant file from bitstream";
        return ILLBLOB_DECODE_FAIL;
    }

    m_ep = (uint8_t)b;

    if (m_ep)
        m_flags |= FL_EP_MOVE;

    // Halfmove clock
    if (!stream.read(b, 16)) {
        LOGERR << "Failed to read halfmove clock from bitstream";
        return ILLBLOB_DECODE_FAIL;
    }

    m_hmclock = (uint16_t)b;

    // Fullmove number
    if (!stream.read(b, 16)) {
        LOGERR << "Failed to read fullmove number from bitstream";
        return ILLBLOB_DECODE_FAIL;
    }

    // '- 1' as our position.ply means "move number in this position", not
    // "the next move will be..." (as with FEN)...
    m_ply = toHalfMove(b, wtm) - 1;

    Legal legal = isLegal();

    if (legal != LEGAL) {
        LOGERR << "Position is illegal (" << legal << ")";
        return legal;
    }

    unsigned count = attacks(lsb(m_pieces[toOppositeColour(m_ply)][KING]), 0, false);

    if (count == 1)
        m_flags |= FL_INCHECK;
    else if (count == 2)
        m_flags |= FL_INCHECK | FL_INDBLCHECK;

#ifdef DEBUG
    else
        ASSERT(count == 0);
#endif // DEBUG

    m_hashKey = generateHashKey();

    return LEGAL;
}

bool Position::blob(Blob &blob) const {
    //
    // See Position::setFromBlob() for blob layout.
    //

    blob.free();

    if (!blob.reserve(38)) {
        LOGERR << "Failed to reserve space for position in blob";
        return false;
    }

    Bitstream stream(blob);
    uint32_t b;

    // Position
    for (Square sq = 0; sq < 64; sq++) {
        PieceColour piece = m_board[sq];
        b = (piece & PIECE_MASK) | (pieceColour(piece) == BLACK ? 0x8 : 0x0);

        if (!stream.write(b, 4)) {
            LOGERR << "Failed to write position to bitstream";
            return false;
        }
    }

    // To move
    b = (toColour(m_ply) == WHITE) ? 0 : 1;

    if (!stream.write(b, 1)) {
        LOGERR << "Failed to write side-to-move to bitstream";
        return false;
    }

    // Castling rights
    b = 0;

    if (m_flags & FL_WCASTLE_KS)
        b |= 0x8;

    if (m_flags & FL_WCASTLE_QS)
        b |= 0x4;

    if (m_flags & FL_BCASTLE_KS)
        b |= 0x2;

    if (m_flags & FL_BCASTLE_QS)
        b |= 0x1;

    if (!stream.write(b, 4)) {
        LOGERR << "Failed to write castling rights to bitstream";
        return false;
    }

    // En-passant file
    b = m_ep;

    if (!stream.write(b, 4)) {
        LOGERR << "Failed to write en-passant file to bitstream";
        return false;
    }

    // Halfmove clock
    b = m_hmclock;

    if (!stream.write(b, 16)) {
        LOGERR << "Failed to write halfmove clock to bitstream";
        return false;
    }

    // Fullmove number
    b = toMove(m_ply + 1);

    if (!stream.write(b, 16)) {
        LOGERR << "Failed to write fullmove number to bitstream";
        return false;
    }

    return true;
}

void Position::setRandom() {
    unsigned attempts = 0;
    bool valid;
    do {
        LOGDBG << "Attempt " << ++attempts;

        init();
        setStarting();
        unsigned maxMoves = (Rand64::rand() % 200) + 30;
        valid = true;
        for (unsigned i = 0; i < maxMoves && valid; i++) {
            Move moves[256];
            UnmakeMoveInfo umi;
            unsigned numMoves = genMoves(moves);
            if (numMoves > 0) {
                unsigned moveNum = Rand64::rand() % numMoves;
                if (!makeMove(moves[moveNum], umi)) {
                    LOGERR << "Failed to make move " << moves[moveNum].dump() << " in position:\n" << dump();
                    valid = false;
                }
            } else {
                valid = false;
            }
        }
    } while (!valid || isLegal() != LEGAL);
}

void Position::bishopSquares(Colour col, uint32_t &lightCount, uint32_t &darkCount) const {
    uint64_t bb, bit;
    uint32_t offset;

    lightCount = 0;
    darkCount = 0;

    bb = m_pieces[col][BISHOP];

    while (bb) {
        offset = lsb2(bb, bit);

        if (isLightSqOffset(offset))
            lightCount++;
        else
            darkCount++;
    }
}

Position::Legal Position::isLegal() const {
    unsigned count;

    count = popcnt(m_pieces[WHITE][KING]);

    if (count != 1) {
        LOGWRN << "Position is invalid; white has " << count << " kings";
        return ILLPOS_WHITE_ONE_KING;
    }

    count = popcnt(m_pieces[BLACK][KING]);

    if (count != 1) {
        LOGWRN << "Position is invalid; black has " << count << " kings";
        return ILLPOS_BLACK_ONE_KING;
    }

    count = popcnt(m_pieces[WHITE][ALLPIECES]);

    if (count > 16) {
        LOGWRN << "Position is invalid; white has " << count << " pieces";

        LOGDBG << "WHITE ALLPIECES=" << OUT_UINT64(m_pieces[WHITE][ALLPIECES]);
        LOGDBG << "WHITE PAWN     =" << OUT_UINT64(m_pieces[WHITE][PAWN]);
        LOGDBG << "WHITE ROOK     =" << OUT_UINT64(m_pieces[WHITE][ROOK]);
        LOGDBG << "WHITE KNIGHT   =" << OUT_UINT64(m_pieces[WHITE][KNIGHT]);
        LOGDBG << "WHITE BISHOP   =" << OUT_UINT64(m_pieces[WHITE][BISHOP]);
        LOGDBG << "WHITE QUEEN    =" << OUT_UINT64(m_pieces[WHITE][QUEEN]);
        LOGDBG << "WHITE KING     =" << OUT_UINT64(m_pieces[WHITE][KING]);

        return ILLPOS_WHITE_TOO_MANY_PIECES;
    }

    count = popcnt(m_pieces[BLACK][ALLPIECES]);

    if (count > 16) {
        LOGWRN << "Position is invalid; black has " << count << " pieces";
        return ILLPOS_BLACK_TOO_MANY_PIECES;
    }

    if (flags() & FL_WCASTLE) {
        if (m_pieces[WHITE][KING] != offsetBit(E1)) {
            LOGWRN << "Position is invalid; white cannot castle as no king on E1";
            return ILLPOS_WHITE_CASTLE_KING_MOVED;
        }

        if ((flags() & FL_WCASTLE_KS) && (m_pieces[WHITE][ROOK] & offsetBit(H1)) == 0) {
            LOGWRN << "Position is invalid; white cannot castle kingside as no rook on H1";
            return ILLPOS_WHITE_CASTLE_KS_ROOK_MOVED;
        }

        if ((flags() & FL_WCASTLE_QS) && (m_pieces[WHITE][ROOK] & offsetBit(A1)) == 0) {
            LOGWRN << "Position is invalid; white cannot castle queenside as no rook on A1";
            return ILLPOS_WHITE_CASTLE_QS_ROOK_MOVED;
        }
    }

    if (flags() & FL_BCASTLE) {
        if (m_pieces[BLACK][KING] != offsetBit(E8)) {
            LOGWRN << "Position is invalid; black cannot castle as no king on E8";
            return ILLPOS_BLACK_CASTLE_KING_MOVED;
        }

        if ((flags() & FL_BCASTLE_KS) && (m_pieces[BLACK][ROOK] & offsetBit(H8)) == 0) {
            LOGWRN << "Position is invalid; black cannot castle kingside as no rook on H8";
            return ILLPOS_BLACK_CASTLE_KS_ROOK_MOVED;
        }

        if ((flags() & FL_BCASTLE_QS) && (m_pieces[BLACK][ROOK] & offsetBit(A8)) == 0) {
            LOGWRN << "Position is invalid; black cannot castle queenside as no rook on A8";
            return ILLPOS_BLACK_CASTLE_QS_ROOK_MOVED;
        }
    }

    if (m_flags & FL_EP_MOVE) {
        Colour moveSide = toColour(m_ply);

        if (moveSide == WHITE) {
            if (m_board[fileRankOffset(m_ep, 3)] != toPieceColour(PAWN, WHITE)) {
                LOGWRN << "Position is invalid; en-passant file is wrong as no pawn on " <<
                    (char)('a' + m_ep) << "3";
                return ILLPOS_EP_NO_PAWN;
            }

            if (m_board[fileRankOffset(m_ep, 2)] != EMPTY ||
                m_board[fileRankOffset(m_ep, 1)] != EMPTY) {
                LOGWRN << "Position is invalid; en-passant file is wrong as " <<
                    (char)('a' + m_ep) << "2 or " << (char)('a' + m_ep) << "1 are not empty";
                return ILLPOS_EP_NOT_EMPTY_BEHIND_PAWN;
            }
        } else { // moveSide == BLACK
            if (m_board[fileRankOffset(m_ep, 4)] != toPieceColour(PAWN, BLACK)) {
                LOGWRN << "Position is invalid; en-passant file is wrong as no pawn on " <<
                    (char)('a' + m_ep) << "4";
                return ILLPOS_EP_NO_PAWN;
            }

            if (m_board[fileRankOffset(m_ep, 5)] != EMPTY ||
                m_board[fileRankOffset(m_ep, 6)] != EMPTY) {
                LOGWRN << "Position is invalid; en-passant file is wrong as " <<
                    (char)('a' + m_ep) << "5 or " << (char)('a' + m_ep) << "6 are not empty";
                return ILLPOS_EP_NOT_EMPTY_BEHIND_PAWN;
            }
        }
    }

    if (attacks(lsb(m_pieces[toColour(m_ply)][KING]), 0, true) > 0)
        return ILLPOS_SIDE_TO_MOVE_GIVING_CHECK;

    return LEGAL;
}

uint64_t Position::generateHashKey() const {
    uint64_t bb, bit, key;
    uint32_t offset;

    key = 0ULL;

    for (Colour colour = WHITE; colour <= BLACK; colour++)
        for (Piece piece  = PAWN; piece <= KING; piece++) {
            bb = m_pieces[colour][piece];

            while (bb) {
                offset = lsb2(bb, bit);
                key ^= pieceHash(colour, piece, offset);
            }
        }

    if (m_flags & FL_WCASTLE_KS)
        key ^= m_hashCastle[HASH_WCASTLE_KS];

    if (m_flags & FL_WCASTLE_QS)
        key ^= m_hashCastle[HASH_WCASTLE_QS];

    if (m_flags & FL_BCASTLE_KS)
        key ^= m_hashCastle[HASH_BCASTLE_KS];

    if (m_flags & FL_BCASTLE_QS)
        key ^= m_hashCastle[HASH_BCASTLE_QS];

    // En-passant hash
    if (m_flags & FL_EP_MOVE)
        key ^= m_hashEnPassant[m_ep];

    // Turn hash
    if ((m_ply & 1) == 1)
        key ^= m_hashTurn;

    return key;
}

string Position::dump(bool lowlevel /*=false*/) const {
    Colour col;
    Piece pce;
    ostringstream oss;

    oss << "+---------------+" << endl;

    for (BoardRank rank = RANK8; rank >= RANK1; rank--) {
        oss << '|';

        for (BoardFile file = FILEA; file <= FILEH; file++) {
            pce = m_board[fileRankOffset(file, rank)];
            col = pce >> 7;
            pce &= PIECE_MASK;

            if (pce >= PAWN && pce <= KING) {
                if (col == WHITE)
                    oss << pieceChars[pce];
                else
                    oss << char(tolower(pieceChars[pce]));
            } else if (pce != EMPTY)
                oss << '?';
            else
                oss << (isLightSq(file, rank) ? '-' : '.');

            oss << '|';
        }

        // Dump variables to the right of the board; one per line
        if (rank == RANK8)
            oss << " ply=" << m_ply << " (" << (toColour(m_ply) == WHITE ? "btm" : "wtm") << ")";
        else if (rank == RANK7) {
            oss << " flags=";

            if (m_flags & FL_WCASTLE_KS)
                oss << "WCASTLE_KS ";

            if (m_flags & FL_WCASTLE_QS)
                oss << "WCASTLE_QS ";

            if (m_flags & FL_BCASTLE_KS)
                oss << "BCASTLE_KS ";

            if (m_flags & FL_BCASTLE_QS)
                oss << "BCASTLE_QS ";

            if (m_flags & FL_EP_MOVE)
                oss << "EP_MOVE ";

            if (m_flags & FL_INDBLCHECK)
                oss << "DBLCHKECK ";
            else if (m_flags & FL_INCHECK)
                oss << "INCHECK ";
        } else if (rank == RANK6)
            oss << " ep=" << int(m_ep);
        else if (rank == RANK5)
            oss << " hmclock=" << m_hmclock;
        else if (rank == RANK4)
            oss << " lastMove=" << m_lastMove;

        oss << endl;
    }

    oss << "+---------------+" << endl;

    if (lowlevel) {
        for (col = WHITE; col <= BLACK; col++)
            for (pce = ALLPIECES; pce < MAXPIECES; pce++) {
                oss << "m_pieces[" << (col == WHITE ? "W" : "B");

                if (pce == ALLPIECES)
                    oss << "A";
                else
                    oss << pieceChars[pce];

                oss << "]=0x" << setw(16) << setfill('0') << hex << m_pieces[col][pce];
                oss << endl;
            }

    }

    return oss.str();
}

#ifdef DEBUG
bool Position::sanityCheck() const {
    bool retval = true;

    for (uint32_t sq = 0; sq < 64 && retval; sq++) {
        Piece pce;
        Colour col;
        piece(sq, pce, col);

        if (pce != EMPTY) {
            if ((m_pieces[col][pce] & offsetBit(sq)) == 0) {
                LOGERR << "Position invalid as " << (col == WHITE ? "White" : "Black") <<
                    " piece '" << pieceChars[pce] << "' on square " <<
                    ('A' + offsetFile(sq)) << ('1' + offsetRank(sq)) << " is missing from bitboard";
                retval = false;
            }

            if ((m_pieces[col][ALLPIECES] & offsetBit(sq)) == 0) {
                LOGERR << "Position invalid as " << (col == WHITE ? "White" : "Black") <<
                    " piece '" << pieceChars[pce] << "' on square " <<
                    ('A' + offsetFile(sq)) << ('1' + offsetRank(sq)) <<
                    " is missing from ALLPIECES bitboard";
                retval = false;
            }
        } else {
            if (m_pieces[col][pce] & offsetBit(sq)) {
                LOGERR << "Position invalid as " << (col == WHITE ? "White" : "Black") <<
                    " piece '" << pieceChars[pce] << "' on square " <<
                    ('A' + offsetFile(sq)) << ('1' + offsetRank(sq)) <<
                    " is set on bitboard";
                retval = false;
            }

            if (m_pieces[WHITE][ALLPIECES] & offsetBit(sq)) {
                LOGERR << "Position invalid as White ALLPIECES square " <<
                    ('A' + offsetFile(sq)) << ('1' + offsetRank(sq)) <<
                    " should be empty";
                retval = false;
            }

            if (m_pieces[BLACK][ALLPIECES] & offsetBit(sq)) {
                LOGERR << "Position invalid: Black ALLPIECES square " <<
                    ('A' + offsetFile(sq)) << ('1' + offsetRank(sq)) <<
                    " should be empty";
                retval = false;
            }
        }
    }

    // Check hash
    uint64_t key = generateHashKey();

    if (key != m_hashKey) {
        LOGERR << "Position hash is incorrect (generated=" << OUT_UINT64(key) <<
            ", current=" << OUT_UINT64(m_hashKey) << ")";
        retval = false;
    }

    if (!retval)
        LOGERR << "position:" << endl << dump(true);

    return retval;
}
#endif // DEBUG

unsigned Position::genNonEvasions(Move *moves) const {
    uint64_t bb, att, fromBit, toBit, pinnedBits, epCapPinned, occupy;
    Colour moveSide, oppSide;
    Piece pce;
    Square fromOffset, toOffset, o;
    unsigned numPins, i;
    int pinnedDir, pawnMoveDir;
    Move pinned[16], *movesStart;

    oppSide = toColour(m_ply);
    moveSide = flipColour(oppSide);
    pinnedBits = 0ULL;
    pawnMoveDir = (moveSide == WHITE) ? +8 : -8;
    movesStart = moves;

    // For each pinned piece, generate moves along the pin line
    numPins = findPinned(pinned, epCapPinned, true);
    for (i = 0; i < numPins; i++) {
        // Remember this piece as being pinned and ignore it in future
        fromOffset = pinned[i].from();
        fromBit = offsetBit(fromOffset);
        pinnedBits |= fromBit;

        if (!pinned[i].canMove())
            continue; // Piece cannot move due to the pin

        pce = pinned[i].piece();
        toOffset = pinned[i].to();
        pinnedDir = pinnedDirs[fromOffset][toOffset];
        ASSERT(pinnedDir);

        if (Move::isSlidingPiece(pce)) {
            // Move towards pinner ending with capture of pinner
            o = fromOffset;

            do {
                o += pinnedDir;
                moves->set((o == toOffset) ? Move::FL_CAPTURE : 0, pce, fromOffset, o);
                moves++;
            } while (o != toOffset);

            // Moves towards king
            toOffset = lsb(m_pieces[moveSide][KING]);
            pinnedDir = -pinnedDir;
            o = fromOffset + pinnedDir;

            while (o != toOffset) {
                moves->set(pce, fromOffset, o);
                moves++;
                o += pinnedDir;
            }
        } else {
            // A pinned pawn that can move towards attacker or capture it
            ASSERT(pce == PAWN);

            if (abs(pinnedDir) == 8) {
                // Move
                toOffset = fromOffset + pawnMoveDir;
                toBit = offsetBit(toOffset);
                moves->set(0, PAWN, fromOffset, toOffset);
                moves++;

                // Is promotion possible?
                if (toBit & rankMask1and8) {
                    // Turn the current move into a queen promotion and set the
                    // other possible promotion moves
                    moves--;
                    moves->set(Move::FL_PROMOTION, QUEEN, PAWN, fromOffset, toOffset);
                    moves++;
                    moves->set(Move::FL_PROMOTION, ROOK, PAWN, fromOffset, toOffset);
                    moves++;
                    moves->set(Move::FL_PROMOTION, KNIGHT, PAWN, fromOffset, toOffset);
                    moves++;
                    moves->set(Move::FL_PROMOTION, BISHOP, PAWN, fromOffset, toOffset);
                    moves++;
                }
                else if (fromBit & rankMask2and7) {
                    toOffset += pawnMoveDir;
                    if (m_board[toOffset] == EMPTY) {
                        moves->set(Move::FL_EP_MOVE, PAWN, fromOffset, toOffset);
                        moves++;
                    }
                }
            } else {
                // Capture
                toOffset = fromOffset + pinnedDir;
                toBit = offsetBit(toOffset);

                if (m_board[toOffset] == EMPTY)
                    moves->set(Move::FL_CAPTURE | Move::FL_EP_CAP, PAWN, fromOffset,
                               fromOffset + pinnedDir);
                else
                    moves->set(Move::FL_CAPTURE, PAWN, fromOffset, fromOffset + pinnedDir);

                moves++;

                // Is promotion possible?
                if (toBit & rankMask1and8) {
                    // Turn the current move into a queen promotion and set the
                    // other possible promotion moves
                    moves--;
                    moves->set(Move::FL_CAPTURE | Move::FL_PROMOTION, QUEEN, PAWN, fromOffset,
                               toOffset);
                    moves++;
                    moves->set(Move::FL_CAPTURE | Move::FL_PROMOTION, ROOK, PAWN, fromOffset,
                               toOffset);
                    moves++;
                    moves->set(Move::FL_CAPTURE | Move::FL_PROMOTION, KNIGHT, PAWN, fromOffset,
                               toOffset);
                    moves++;
                    moves->set(Move::FL_CAPTURE | Move::FL_PROMOTION, BISHOP, PAWN, fromOffset,
                               toOffset);
                    moves++;
                }
            }
        }
    }

    pinnedBits = ~pinnedBits; // To exclude pinned pieces
    occupy = m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES];

    //
    // Pawns
    //
    bb = m_pieces[moveSide][PAWN] & pinnedBits;
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if (moveSide == WHITE) {
            toOffset = fromOffset + 8;
            toBit = fromBit << 8;
        } else {
            toOffset = fromOffset - 8;
            toBit = fromBit >> 8;
        }

        if ((toBit & (m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES])) == 0ULL) {
            moves->set(0, PAWN, fromOffset, toOffset);
            moves++;

            // Is promotion possible?
            if (toBit & rankMask1and8) {
                // Turn the current move into a queen promotion and set the
                // other possible promotion moves
                moves--;
                moves->set(Move::FL_PROMOTION, QUEEN, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION, ROOK, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION, KNIGHT, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION, BISHOP, PAWN, fromOffset, toOffset);
                moves++;
            }
            // *ELSE* (important!): If we are on the initial rank, then attempt a two-square move
            else if (fromBit & rankMask2and7) {
                if (moveSide == WHITE) {
                    toOffset = fromOffset + 16;
                    toBit = fromBit << 16;
                } else {
                    toOffset = fromOffset - 16;
                    toBit = fromBit >> 16;
                }

                if (((m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES]) & toBit) == 0ULL) {
                    moves->set(Move::FL_EP_MOVE, PAWN, fromOffset, toOffset);
                    moves++;
                }
            }
        }

        //
        // Pawn captures
        //
        att = pawnAttacks[moveSide][fromOffset] & m_pieces[oppSide][ALLPIECES];
        while (att) {
            toOffset = lsb2(att, toBit);

            moves->set(Move::FL_CAPTURE, PAWN, fromOffset, toOffset);
            moves++;

            // Is capture + promotion possible?
            if (toBit & rankMask1and8) {
                // Turn the current move into a queen promotion and set the
                // other possible promotion moves
                moves--;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, QUEEN, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, ROOK, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, KNIGHT, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, BISHOP, PAWN, fromOffset, toOffset);
                moves++;
            }
        }

        //
        // Pawn en-passant captures
        //
        if ((m_flags & FL_EP_MOVE) && (fromBit & epCapPinned) == 0) {
            att = epMask[moveSide][fromOffset] & m_pieces[oppSide][PAWN] & fileMasks[m_ep];
            if (att) {
                toOffset = lsb(att);

                if (moveSide == WHITE)
                    toOffset += 8;
                else
                    toOffset -= 8;

                moves->set(Move::FL_EP_CAP | Move::FL_CAPTURE, PAWN, fromOffset, toOffset);
                moves++;
            }
        }
    }

    //
    // Knights
    //
    bb = m_pieces[moveSide][KNIGHT] & pinnedBits;
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        att = knightAttacks[fromOffset] & ~m_pieces[moveSide][ALLPIECES];
        while (att) {
            toOffset = lsb2(att, toBit);

            if (m_pieces[oppSide][ALLPIECES] & toBit)
                moves->set(Move::FL_CAPTURE, KNIGHT, fromOffset, toOffset);
            else
                moves->set(KNIGHT, fromOffset, toOffset);

            moves++;
        }
    }

    //
    // Rooks and Queens
    //
    bb = (m_pieces[moveSide][ROOK] | m_pieces[moveSide][QUEEN]) & pinnedBits;
    while (bb) {
        fromOffset = lsb2(bb, fromBit);
        att = Util::magicRookAttacks(fromOffset, occupy);
        att &= ~m_pieces[moveSide][ALLPIECES];

        while (att) {
            toOffset = lsb2(att, toBit);

            if (m_pieces[oppSide][ALLPIECES] & toBit)
                moves->set(Move::FL_CAPTURE, m_board[fromOffset] & PIECE_MASK, fromOffset,
                           toOffset);
            else
                moves->set(m_board[fromOffset] & PIECE_MASK, fromOffset, toOffset);

            moves++;
        }
    }

    //
    // Bishops and Queens
    //
    bb = (m_pieces[moveSide][BISHOP] | m_pieces[moveSide][QUEEN]) & pinnedBits;

    while (bb) {
        fromOffset = lsb2(bb, fromBit);
        att = Util::magicBishopAttacks(fromOffset, occupy);
        att &= ~m_pieces[moveSide][ALLPIECES];

        while (att) {
            toOffset = lsb2(att, toBit);

            if (m_pieces[oppSide][ALLPIECES] & toBit)
                moves->set(Move::FL_CAPTURE, m_board[fromOffset] & PIECE_MASK, fromOffset,
                           toOffset);
            else
                moves->set(m_board[fromOffset] & PIECE_MASK, fromOffset, toOffset);

            moves++;
        }
    }

    //
    // King
    //
    fromOffset = lsb(m_pieces[moveSide][KING]);
    fromBit = offsetBit(fromOffset);
    bb = kingAttacks[fromOffset] & ~m_pieces[moveSide][ALLPIECES];

    while (bb) {
        toOffset = lsb2(bb, toBit);

        if (attacks(toOffset, false, fromBit))
            continue;

        if (m_pieces[oppSide][ALLPIECES] & toBit)
            moves->set(Move::FL_CAPTURE, KING, fromOffset, toOffset);
        else
            moves->set(KING, fromOffset, toOffset);

        moves++;
    }

    //
    // Castling
    //
    ASSERT((m_flags & FL_INCHECK) == 0);
    if (moveSide == WHITE) {
        if ((m_flags & FL_WCASTLE_KS) &&
            (occupy & (offsetBit(F1) | offsetBit(G1))) == 0ULL &&
            !attacks(E1, false, fromBit) && !attacks(F1, false, fromBit) && !attacks(G1, false, fromBit)) {
            moves->set(Move::FL_CASTLE_KS, KING, E1, G1);
            moves++;
        }

        if ((m_flags & FL_WCASTLE_QS) &&
            (occupy & (offsetBit(B1) | offsetBit(C1) | offsetBit(D1))) == 0ULL &&
            !attacks(E1, false, fromBit) && !attacks(D1, false, fromBit) && !attacks(C1, false, fromBit)) {
            moves->set(Move::FL_CASTLE_QS, KING, E1, C1);
            moves++;
        }
    } else { // move_side == BLACK
        if ((m_flags & FL_BCASTLE_KS) &&
            (occupy & (offsetBit(F8) | offsetBit(G8))) == 0ULL &&
            !attacks(E8, false, fromBit) && !attacks(F8, false, fromBit) && !attacks(G8, false, fromBit)) {
            moves->set(Move::FL_CASTLE_KS, KING, E8, G8);
            moves++;
        }

        if ((m_flags & FL_BCASTLE_QS) &&
            (occupy & (offsetBit(B8) | offsetBit(C8) | offsetBit(D8))) == 0ULL &&
            !attacks(E8, false, fromBit) && !attacks(D8, false, fromBit) && !attacks(C8, false, fromBit)) {
            moves->set(Move::FL_CASTLE_QS, KING, E8, C8);
            moves++;
        }
    }

    return (unsigned)(moves - movesStart);
}

unsigned Position::genEvasions(Move *moves) const {
    uint64_t bb, att, fromBit, toBit, pinnedBits, epCapPinned, occupy, attackLine, attackerBit;
    Colour moveSide, oppSide;
    unsigned numAttackers;
    int fromOffset, toOffset;
    Move attackers[2], *movesStart;
    bool mustCapture;

    ASSERT(m_flags & FL_INCHECK);

    oppSide = toColour(m_ply);
    moveSide = flipColour(oppSide);
    movesStart = moves;

    //
    // King moves out of the way
    //
    fromOffset = lsb(m_pieces[moveSide][KING]);
    fromBit = offsetBit(fromOffset);
    bb = kingAttacks[fromOffset] & ~m_pieces[moveSide][ALLPIECES];

    while (bb) {
        toOffset = lsb2(bb, toBit);

        if (attacks(toOffset, false, fromBit))
            continue;

        if ((m_pieces[oppSide][ALLPIECES] & toBit))
            moves->set(Move::FL_CAPTURE, KING, fromOffset, toOffset);
        else
            moves->set(KING, fromOffset, toOffset);

        moves++;
    }

    if (m_flags & FL_INDBLCHECK)
        // If the king is double-checked then moving out of the way
        // is all it can do to evade check
        return (unsigned)(moves - movesStart);

    //
    // Move a piece between the attacker and the king or take
    // the attacking piece
    //
    findPinned(pinnedBits, epCapPinned, true);
    pinnedBits = ~pinnedBits; // To exclude pinned pieces
    numAttackers = attacks(fromOffset, attackers, false);
    if (numAttackers != 1)
        throw ChessCoreException("Number of attackers was not 1!");

    // Find the squares we need to move a piece to, or the square of the
    // attacker to capture
    if (attackers[0].isSlidingPiece()) {
        attackLine = connectMasks[fromOffset][attackers[0].from()];
        mustCapture = false;
    } else {
        attackLine = 0ULL;
        mustCapture = true;
    }

    // attackerBit will be added to attackLine after pawn moves have been generated
    attackerBit = offsetBit(attackers[0].from());
    occupy = m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES];

    // Pawns
    bb = m_pieces[moveSide][PAWN] & pinnedBits;
    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        if (!mustCapture) {
            if (moveSide == WHITE) {
                toOffset = fromOffset + 8;
                toBit = fromBit << 8;
            } else {
                toOffset = fromOffset - 8;
                toBit = fromBit >> 8;
            }

            if (((m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES]) & toBit) == 0ULL) {
                if (toBit & attackLine) {
                    moves->set(0, PAWN, fromOffset, toOffset);
                    moves++;

                    // Is promotion possible?
                    if (toBit & rankMask1and8) {
                        // Turn the current move into a queen promotion and set the
                        // other possible promotion moves
                        moves--;
                        moves->set(Move::FL_PROMOTION, QUEEN, PAWN, fromOffset, toOffset);
                        moves++;
                        moves->set(Move::FL_PROMOTION, ROOK, PAWN, fromOffset, toOffset);
                        moves++;
                        moves->set(Move::FL_PROMOTION, KNIGHT, PAWN, fromOffset, toOffset);
                        moves++;
                        moves->set(Move::FL_PROMOTION, BISHOP, PAWN, fromOffset, toOffset);
                        moves++;
                    }
                } else if (fromBit & rankMask2and7) {
                    if (moveSide == WHITE) {
                        toOffset = fromOffset + 16;
                        toBit = fromBit << 16;
                    } else {
                        toOffset = fromOffset - 16;
                        toBit = fromBit >> 16;
                    }

                    if (((m_pieces[WHITE][ALLPIECES] | m_pieces[BLACK][ALLPIECES]) & toBit) == 0ULL &&
                        (toBit & attackLine)) {
                        moves->set(Move::FL_EP_MOVE, PAWN, fromOffset, toOffset);
                        moves++;
                    }
                }
            }
        }

        //
        // Pawn captures
        //
        toBit = pawnAttacks[moveSide][fromOffset] & attackerBit;

        if (toBit) {
            toOffset = lsb(toBit);

            moves->set(Move::FL_CAPTURE, PAWN, fromOffset, toOffset);
            moves++;

            // Is capture + promotion possible?
            if (toBit & rankMask1and8) {
                // Turn the current move into a queen promotion and set the
                // other possible promotion moves
                moves--;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, QUEEN, PAWN, fromOffset,
                           toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, ROOK, PAWN, fromOffset, toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, KNIGHT, PAWN, fromOffset,
                           toOffset);
                moves++;
                moves->set(Move::FL_PROMOTION | Move::FL_CAPTURE, BISHOP, PAWN, fromOffset,
                           toOffset);
                moves++;
            }
        }

        //
        // Pawn en-passant captures
        //
        if ((m_flags & FL_EP_MOVE) && (fromBit & epCapPinned) == 0) {
            toBit = epMask[moveSide][fromOffset] & attackerBit & fileMasks[m_ep];

            if (toBit) {
                toOffset = lsb(toBit);

                if (moveSide == WHITE)
                    toOffset += 8;
                else
                    toOffset -= 8;

                moves->set(Move::FL_EP_CAP | Move::FL_CAPTURE, PAWN, fromOffset, toOffset);
                moves++;
            }
        }
    }

    // All other pieces move and capture the same way so attacker_bit
    // can be part of attack_line
    attackLine |= attackerBit;

    //
    // Knights
    //
    bb = m_pieces[moveSide][KNIGHT] & pinnedBits;

    while (bb) {
        fromOffset = lsb2(bb, fromBit);

        att = knightAttacks[fromOffset] & attackLine;

        while (att) {
            toOffset = lsb2(att, toBit);

            if (toBit & attackerBit)
                moves->set(Move::FL_CAPTURE, KNIGHT, fromOffset, toOffset);
            else if (!mustCapture)
                moves->set(KNIGHT, fromOffset, toOffset);

            moves++;
        }
    }

    //
    // Rooks and Queens
    //
    bb = (m_pieces[moveSide][ROOK] | m_pieces[moveSide][QUEEN]) & pinnedBits;

    while (bb) {
        fromOffset = lsb2(bb, fromBit);
        att = Util::magicRookAttacks(fromOffset, occupy);
        att &= attackLine;

        while (att) {
            toOffset = lsb2(att, toBit);

            if (toBit & attackerBit) {
                moves->set(Move::FL_CAPTURE, m_board[fromOffset] & PIECE_MASK, fromOffset,
                           toOffset);
            } else if (!mustCapture) {
                moves->set(m_board[fromOffset] & PIECE_MASK, fromOffset, toOffset);
            }

            moves++;
        }
    }

    //
    // Bishops and Queens
    //
    bb = (m_pieces[moveSide][BISHOP] | m_pieces[moveSide][QUEEN]) & pinnedBits;

    while (bb) {
        fromOffset = lsb2(bb, fromBit);
        att = Util::magicBishopAttacks(fromOffset, occupy);
        att &= attackLine;

        while (att) {
            toOffset = lsb2(att, toBit);

            if (toBit & attackerBit)
                moves->set(Move::FL_CAPTURE, m_board[fromOffset] & PIECE_MASK, fromOffset,
                           toOffset);
            else if (!mustCapture)
                moves->set(m_board[fromOffset] & PIECE_MASK, fromOffset, toOffset);

            moves++;
        }
    }

    return (unsigned)(moves - movesStart);
}

ostream &operator << (ostream &os, const Position &pos) {
    os << pos.dump();
    return os;
}

}   // namespace ChessCore
