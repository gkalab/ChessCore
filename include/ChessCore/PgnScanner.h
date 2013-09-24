//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// PgnScanner.h: Defines used by the lexical scanner for parsing PGN games.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <istream>

namespace ChessCore {
//
// PgnScannerContext keeps context between PgnDatabase and PgnScanner
//
class CHESSCORE_EXPORT PgnScannerContext {
private:
    static const char *m_classname;

protected:
    void *m_scanner;
    std::istream &m_stream;
    unsigned m_lineNumber;

public:
    PgnScannerContext(std::istream &stream);
    virtual ~PgnScannerContext();
    void *scanner();
    int read(void *buffer, unsigned len);
    int lex();
    void restart();
    void flush();
    char *text() const;
    unsigned lineNumber() const;
    void setLineNumber(unsigned lineNumber);
    void incLineNumber(unsigned amount = 1);
protected:
    void initScanner();
    void destroyScanner();
};

#define A_PGN_EVENT        300
#define A_PGN_SITE         301
#define A_PGN_DATE         302
#define A_PGN_ROUND        303
#define A_PGN_WHITE        304
#define A_PGN_BLACK        305
#define A_PGN_RESULT       306
#define A_PGN_SETUP        307
#define A_PGN_FEN          308
#define A_PGN_ANNOTATOR    309
#define A_PGN_ECO          310
#define A_PGN_WHITEELO     311
#define A_PGN_BLACKELO     312
#define A_PGN_OPENING      313
#define A_PGN_VARIATION    314
#define A_PGN_XXX          315

// PGN header entries
#define IS_PGN_HEADER(x)  ((x) >= A_PGN_EVENT && (x) <= A_PGN_XXX)

#define A_WHITE_MOVENUM    350
#define A_BLACK_MOVENUM    351

// Move numbers
#define IS_PGN_MOVENUM(x) ((x) == A_WHITE_MOVENUM || (x) == A_BLACK_MOVENUM)

// Moves
#define A_PAWN_MOVE        355
#define A_PAWN_CAPTURE     356
#define A_PIECE_MOVE       357
#define A_PIECE_CAPTURE    358
#define A_SHORT_CASTLE     359
#define A_LONG_CASTLE      360

#define IS_PGN_MOVE(x)      ((x) >= A_PAWN_MOVE && (x) <= A_LONG_CASTLE)
#define PGN_MOVE_TO_ZERO(x) ((x) - A_PAWN_MOVE)

// Evaluation
#define A_CHECK            380
#define A_MATE             381
#define A_GOOD_MOVE        382
#define A_BAD_MOVE         383
#define A_INTERESTING_MOVE 384
#define A_DUBIOUS_MOVE     385
#define A_BRILLIANT_MOVE   386
#define A_BLUNDER_MOVE     387
#define A_NAG              388
#define A_NAG_MATE         389
#define A_NAG_NOVELTY      390

#define IS_PGN_EVAL(x)   ((x) >= A_CHECK && (x) <= A_NAG_NOVELTY)

// Results
#define A_WHITE_WINS       400
#define A_BLACK_WINS       401
#define A_UNFINISHED       402
#define A_DRAW             403

#define IS_PGN_RESULT(x) ((x) >= A_WHITE_WINS && (x) <= A_DRAW)

// Miscellaneous
#define A_COMMENT          420
#define A_ROL_COMMENT      421
#define A_VARSTART         422
#define A_VAREND           423
} // namespace ChessCore
