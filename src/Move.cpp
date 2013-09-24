//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Move.cpp: Move class implementation.
//

#include <ChessCore/Move.h>
#include <ChessCore/Position.h>
#include <ChessCore/Log.h>
#include <string.h>
#include <iomanip>
#include <sstream>

using namespace std;

namespace ChessCore {
const char *Move::m_classname = "Move";

static struct {
    uint16_t flag;
    const char *text;
} flagsText[] = {
    {   Move::FL_CASTLE_KS,    "CASTLE_KS"},
    {   Move::FL_CASTLE_QS,    "CASTLE_QS"},
    {   Move::FL_EP_MOVE,      "EP_MOVE"},
    {   Move::FL_EP_CAP,       "EP_CAP"},
    {   Move::FL_PROMOTION,    "PROMOTION"},
    {   Move::FL_CAPTURE,      "CAPTURE"},
    {   Move::FL_CHECK,        "CHECK"},
    {   Move::FL_DOUBLE_CHECK, "DOUBLE_CHECK"},
    {   Move::FL_MATE,         "MATE"},
    {   Move::FL_DRAW,         "DRAW"},
    {   Move::FL_ILLEGAL,      "ILLEGAL"},
    {   Move::FL_CAN_MOVE,     "CAN_MOVE"}
};
const unsigned numFlagsText = (sizeof(flagsText) / sizeof(flagsText[0]));

void Move::reverseFromTo() {
    Square from = m_from;

    m_from = m_to;
    m_to = from;
}

string Move::san(const Position &pos, const char *pieceMap /*=0*/) const {
    unsigned i, numMoves, actualMove = 999;
    Move moves[256];
    char ambigFile, ambigRank;
    ostringstream oss;

    if (pieceMap == 0)
        pieceMap = pieceChars;

    if (isNull()) {
        oss << "null";
        return oss.str();
    } else if (isCastleKS()) {
        oss << "O-O";
        return oss.str();
    } else if (isCastleQS()) {
        oss << "O-O-O";
        return oss.str();
    }

    // If a piece (not a pawn) is moving, check for ambiguity in the
    // from file/rank by looking for other pieces that can move to
    // the same square.
    ambigFile = '\0';
    ambigRank = '\0';
    numMoves = pos.genMoves(moves);

    for (i = 0; i < numMoves; i++) {
        uint8_t otherFrom, otherTo;

        if (piece() != moves[i].piece())
            continue;

        otherFrom = moves[i].from();
        otherTo = moves[i].to();

        if (otherTo == to() && otherFrom != from()) {
            if (offsetRank(otherFrom) == offsetRank(from()))
                ambigFile = offsetFile(from()) + 'a';

            if (offsetFile(otherFrom) == offsetFile(from()))
                ambigRank = offsetRank(from()) + '1';

            if (ambigFile == '\0' && ambigRank == '\0')
                ambigFile = offsetFile(from()) + 'a';
        }

        if (actualMove == 999 && otherTo == to() && otherFrom == from() &&
            (!moves[i].isPromotion() || moves[i].prom() == prom())) {
            actualMove = i;
        }
    }

    if (actualMove == 999) {
        LOGERR << "Didn't find legal move " << dump() << " in position:\n" << pos.dump();
        return "";
    }

#ifdef _DEBUG
    // Only check these move flags as others can be set only after the move
    // has been played within a Position or Game
#define CHECK_FLAGS(f) (f & (FL_CASTLE_KS | FL_CASTLE_QS | FL_EP_MOVE | FL_EP_CAP | FL_PROMOTION | FL_CAPTURE))

    if (CHECK_FLAGS(moves[actualMove].flags()) != CHECK_FLAGS(flags())) {
        LOGDBG << "Wrong flags during SAN generation! actual=0x" <<
            hex << CHECK_FLAGS(moves[actualMove].flags()) << ", mine=0x" <<
            hex << CHECK_FLAGS(flags());
    }

#undef CHECK_FLAGS
#endif // _DEBUG

    if (piece() == PAWN) {
        if (isCapture())
            oss << char(offsetFile(from()) + 'a');
    } else {
        oss << pieceMap[piece()];

        if (ambigFile != '\0')
            oss << ambigFile;
    }

    if (ambigRank != '\0')
        oss << ambigRank;

    if (isCapture())
        oss << 'x';

    oss << char(offsetFile(to()) + 'a');
    oss << char(offsetRank(to()) + '1');

    if (isPromotion()) {
        oss << '=';
        oss << pieceMap[prom()];
    }

    if (isMate())
        oss << '#';
    else if (isCheck())
        oss << '+';

    return oss.str();
}

string Move::coord(bool uciCompliant /*=false*/) const {
    if (isNull()) {
        if (uciCompliant)
            return "0000";
        else
            return "null";
    }

    ostringstream oss;
    oss << char(offsetFile(from()) + 'a');
    oss << char(offsetRank(from()) + '1');
    oss << char(offsetFile(to()) + 'a');
    oss << char(offsetRank(to()) + '1');

    if (isPromotion()) {
        if (uciCompliant)
            oss << char(tolower(pieceChars[prom()]));
        else
            oss << '=' << pieceChars[prom()];
    }

    return oss.str();
}

#define ISFILELETTER(x)      ((x) >= 'a' && (x) <= 'h')
#define ISRANKNUMBER(x)      ((x) >= '1' && (x) <= '8')
#define ISPROMPIECELETTER(x) (strchr("RNBQrnbq", (x)) != 0)
#define ISPIECELETTER(x)     (strchr("PRNBQK", (x)) != 0)
#define UNSET 127

bool Move::parse(const Position &pos, const string &str) {
    unsigned f, i, numMoves;
    const char *p;
    BoardFile fileFrom, fileTo;
    BoardRank rankFrom, rankTo;
    Colour moveSide;
    Move moves[256];

    init();

    moveSide = toOppositeColour(pos.ply());

    const char *text = str.c_str();
    size_t len = str.length();

    if (len >= 4 && ISFILELETTER(*text) && ISRANKNUMBER(*(text + 1)) && ISFILELETTER(*(text + 2))
        && ISRANKNUMBER(*(text + 3))) {
        // Co-ordinate
        fileFrom = (BoardFile)(*text - 'a');
        rankFrom = (BoardRank)(*(text + 1) - '1');
        fileTo = (BoardFile)(*(text + 2) - 'a');
        rankTo = (BoardRank)(*(text + 3) - '1');
        setFrom(fileRankOffset(fileFrom, rankFrom));
        setTo(fileRankOffset(fileTo, rankTo));
        setPiece(pos.piece(from()));

        if (len >= 5 && ISPROMPIECELETTER(*(text + 4))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 4)));
        } else if (len >= 6 && *(text + 4) == '=' && ISPROMPIECELETTER(*(text + 5))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 5)));
        }

        return complete(pos);
    } else if (len >= 5 && ISFILELETTER(*text) && ISRANKNUMBER(*(text + 1))
               && (*(text + 2) == '-' || *(text + 2) == 'x') && ISFILELETTER(*(text + 3))
               && ISRANKNUMBER(*(text + 4))) {
        // Co-ordinate, with hyphen or capture
        fileFrom = (BoardFile)(*text - 'a');
        rankFrom = (BoardRank)(*(text + 1) - '1');
        fileTo = (BoardFile)(*(text + 3) - 'a');
        rankTo = (BoardRank)(*(text + 4) - '1');
        setFrom(fileRankOffset(fileFrom, rankFrom));
        setTo(fileRankOffset(fileTo, rankTo));
        setPiece(pos.piece(from()));

        if (len >= 6 && ISPROMPIECELETTER(*(text + 5))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 5)));
        } else if (len >= 7 && *(text + 5) == '=' && ISPROMPIECELETTER(*(text + 6))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 6)));
        }

        return complete(pos);
    } else if (len >= 5 && ISPIECELETTER(*text) && ISFILELETTER(*(text + 1)) &&
               ISRANKNUMBER(*(text + 2))
               && ISFILELETTER(*(text + 3)) && ISRANKNUMBER(*(text + 4))) {
        // Long Algebraic
        fileFrom = (BoardFile)(*(text + 1) - 'a');
        rankFrom = (BoardRank)(*(text + 2) - '1');
        fileTo = (BoardFile)(*(text + 3) - 'a');
        rankTo = (BoardRank)(*(text + 4) - '1');
        setFrom(fileRankOffset(fileFrom, rankFrom));
        setTo(fileRankOffset(fileTo, rankTo));
        setPiece(pos.piece(from()));

        if (len >= 8 && *(text + 5) == '=' && ISPROMPIECELETTER(*(text + 6))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 6)));
        }

        return complete(pos);
    } else if (len >= 6 && ISPIECELETTER(*text) && ISFILELETTER(*(text + 1)) &&
               ISRANKNUMBER(*(text + 2))
               && (*(text + 3) == '-' || *(text + 3) == 'x') && ISFILELETTER(*(text + 4))
               && ISRANKNUMBER(*(text + 5))) {
        // Long Algebraic, with hyphen or capture
        fileFrom = (BoardFile)(*(text + 1) - 'a');
        rankFrom = (BoardRank)(*(text + 2) - '1');
        fileTo = (BoardFile)(*(text + 4) - 'a');
        rankTo = (BoardRank)(*(text + 5) - '1');
        setFrom(fileRankOffset(fileFrom, rankFrom));
        setTo(fileRankOffset(fileTo, rankTo));
        setPiece(pos.piece(from()));

        if (len >= 9 && *(text + 6) == '=' && ISPROMPIECELETTER(*(text + 7))) {
            setFlags(FL_PROMOTION);
            setProm(pieceFromText(*(text + 7)));
        }

        return complete(pos);
    } else {
        if ((len == 2 && (strcasecmp(text, "OO") == 0 || strcasecmp(text, "00") == 0)) ||
            (len == 3 && (strcasecmp(text, "O-O") == 0 || strcasecmp(text, "0-0") == 0))) {
            // Kingside castling
            if (moveSide == WHITE) {
                setFrom(E1);
                setTo(G1);
            } else {
                setFrom(E8);
                setTo(G8);
            }

            setPiece(KING);
            setFlags(FL_CASTLE_KS);
            return complete(pos);
        } else if ((len == 3 && (strcasecmp(text, "OOO") == 0 || strcmp(text, "000") == 0)) ||
            (len == 5 && (strcasecmp(text, "O-O-O") == 0 || strcmp(text, "0-0-0") == 0))) {
            // Queenside castling
            if (moveSide == WHITE) {
                setFrom(E1);
                setTo(C1);
            } else {
                setFrom(E8);
                setTo(C8);
            }

            setPiece(KING);
            setFlags(FL_CASTLE_QS);
            return complete(pos);
        }

        fileFrom = (BoardFile)(UNSET);
        rankFrom = (BoardRank)(UNSET);

        p = text;

        // Piece
        if (ISPIECELETTER(*p)) {
            setPiece(pieceFromText(*p++));

            // Disambiguations
            f = 0;

            for (i = 0; i < 5; i++) {
                if (ISFILELETTER(*(p + i)) || ISRANKNUMBER(*(p + i)))
                    f++;
                else if (*(p + i) != 'x')
                    break;
            }

            if (f == 4) {
                fileFrom = (BoardFile)(*p++ - 'a');
                rankFrom = (BoardRank)(*p++ - '1');
            } else if (f == 3) {
                // File OR rank disambiguation
                if (ISFILELETTER(*p))
                    fileFrom = (BoardFile)(*p++ - 'a');
                else if (ISRANKNUMBER(*p))
                    rankFrom = (BoardRank)(*p++ - '1');
            }

            // Capture
            if (*p == 'x') {
                setFlags(FL_CAPTURE);
                p++;
            }
        } else {
            setPiece(PAWN);

            // Capture
            if (ISFILELETTER(*p) && *(p + 1) == 'x') {
                fileFrom = (BoardFile)(*p - 'a');
                p += 2;
                setFlags(FL_CAPTURE);
            }
        }

        if (ISFILELETTER(*p)) {
            fileTo = (BoardFile)(*p++ - 'a');
        } else {
            LOGERR << "Missing file in move '" << str << "'";
            return false; // No file
        }

        if (ISRANKNUMBER(*p)) {
            rankTo = (BoardRank)(*p++ - '1');
        } else {
            LOGERR << "Missing rank in move '" << str << "'";
            return false; // No rank
        }

        // Promotion
        if (piece() == PAWN && *p != '\0') {
            if (*p == '=' && ISPROMPIECELETTER(*(p + 1))) {
                setFlags(FL_PROMOTION);
                setProm(pieceFromText(*(p + 1)));
            } else if (ISPROMPIECELETTER(*p)) {
                setFlags(FL_PROMOTION);
                setProm(pieceFromText(*p));
            }
        }

        setTo(fileRankOffset(fileTo, rankTo));

        // Check or mate?
        if (*p == '+')
            setFlags(FL_CHECK);
        else if (*p == '#')
            setFlags(FL_MATE);

        numMoves = pos.genMoves(moves);
        vector<Move> found;

        for (i = 0; i < numMoves; i++) {
            if (moves[i].piece() != piece() || moves[i].to() != to())
                continue;

            if (isPromotion()) {
                if (!moves[i].isPromotion())
                    continue;
                else if (moves[i].prom() != prom())
                    continue;
            }

            if (fileFrom == UNSET && rankFrom == UNSET) {
                found.push_back(moves[i]);
            } else if (fileFrom == UNSET) {
                if (offsetRank(moves[i].from()) == rankFrom) {
                    found.push_back(moves[i]);
                }
            } else if (rankFrom == UNSET) {
                if (offsetFile(moves[i].from()) == fileFrom) {
                    found.push_back(moves[i]);
                }
            } else // fileFrom != UNSET && rankFrom != UNSET
            if (moves[i].from() == fileRankOffset(fileFrom, rankFrom)) {
                found.push_back(moves[i]);
            }
        }

        if (found.size() == 1) {
            set(found[0]);
            return complete(pos);
        } else if (found.size() > 1) {
            ostringstream oss;
            bool first = true;
            for (auto it = found.begin(); it != found.end(); ++it) {
                Move m = *it;
                if (!first)
                    oss << ", ";
                else
                    first = false;
                oss << m.san(pos);
            }
            LOGERR << "Move '" << str << "' is ambiguous! Could be any of: " << oss.str() << ". Position:\n" << pos.dump();
        } else {
            LOGERR << "Move '" << str << "' is invalid";
        }
    }

    return false;
}

bool Move::complete(const Position &pos, bool suppressError /*=false*/) {
    unsigned i, numMoves;
    Move moves[256];

    numMoves = pos.genMoves(moves);

    for (i = 0; i < numMoves; i++)
        if (equals(moves[i])) {
            set(moves[i]);
            return true;
        }

    if (!suppressError) {
        LOGERR << "Failed to complete move " << dump() << ". Position:" << endl << pos;

#if defined(_DEBUG)
        pos.sanityCheck();

        // Dump the moves that were generated then
        LOGDBG << "Generated " << numMoves << " moves";

        for (i = 0; i < numMoves; i++)
            LOGDBG << "moves[" << i << "]=" << moves[i];
#endif // _DEBUG
    }

    return false;
}

Piece Move::pieceFromText(char text) {
    switch (text) {
    case 'p':
    case 'P':
        return PAWN;

    case 'r':
    case 'R':
        return ROOK;

    case 'n':
    case 'N':
        return KNIGHT;

    case 'b':
    case 'B':
        return BISHOP;

    case 'q':
    case 'Q':
        return QUEEN;

    case 'k':
    case 'K':
        return KING;

    default:
        break;
    }

    return EMPTY;
}

string Move::dump(bool includeFlags /*=true*/) const {
    if (isNull())
        return "null";

    ostringstream oss;

    Piece pce = piece();

    if (pce >= PAWN && pce <= KING)
        oss << pieceChars[pce];

    oss << coord();

    if (includeFlags)
        for (unsigned i = 0; i < numFlagsText; i++)
            if ((m_flags & flagsText[i].flag) != 0)
                oss << ' ' << flagsText[i].text;

    return oss.str();
}

string Move::dump(const vector<Move> &moveList) {
    ostringstream oss;
    bool first = true;
    for (auto it = moveList.begin(); it != moveList.end(); ++it) {
        if (!first)
            oss << ", ";
        else
            first = false;
        oss << *it;
    }
    return oss.str();
}

ostream &operator << (ostream &os, const Move &move) {
    os << move.dump();
    return os;
}

}   // namespace ChessCore
