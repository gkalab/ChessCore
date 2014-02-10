//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Game.cpp: Game class implementation.
//

#include <ChessCore/Game.h>
#include <ChessCore/PgnDatabase.h>
#include <ChessCore/Log.h>

#ifndef WINDOWS
#include <sys/utsname.h>
#endif // !WINDOWS

#include <algorithm>
#include <sstream>

using namespace std;

namespace ChessCore {
const char *Game::m_classname = "Game";
bool Game::m_relaxedMode = false;

Game::Game() :
    GameHeader(),
    m_startPosition(),
    m_position(),
    m_mainline(0),
    m_currentMove(0),
    m_variationStart(false)
{
    init();
}

Game::Game(const Game &other) :
    GameHeader(other),
    m_startPosition(other.m_startPosition),
    m_position(other.m_startPosition),
    m_currentMove(0),
    m_variationStart(false)
{
    m_mainline = AnnotMove::deepCopy(other.m_mainline);
}

Game::~Game() {
    init();
}

void Game::init() {
    initHeader();
    m_startPosition.setStarting();
    m_position.set(m_startPosition);
    m_variationStart = false;
    removeMoves();
}

void Game::setGame(const Game &other) {
    setHeader(other);
    m_startPosition.set(other.m_startPosition);
    m_currentMove = 0;
    m_variationStart = false;

    if (m_mainline)
        AnnotMove::deepDelete(m_mainline);

    m_mainline = AnnotMove::deepCopy(other.m_mainline);
    setPositionToStart();
}

bool Game::moveList(const AnnotMove *lastMove, vector<Move> &moves) const {
    moves.clear();

    if (m_mainline && lastMove) {
        // Move from the last move, back to the start of the mainline
        const AnnotMove *amove = lastMove;

        while (amove) {
            moves.push_back(amove->move());

            if (amove->mainline())
                amove = mainline()->prev();
            else
                amove = amove->prev();
        }

        // Now put the moves in the correct order
        reverse(moves.begin(), moves.end());
    }

    return true;
}

AnnotMove *Game::makeMove(const std::string &movetext, const std::string *annot /*=0*/,
                          std::string *formattedMove /*=0*/, bool includeMoveNum /*=false*/, GameOver *gameOver /*=0*/,
                          AnnotMove **oldNext /*=0*/) {
    Move move;

    if (m_variationStart) {
        if (!restorePriorPosition(m_currentMove))
            return 0;
    } else {
        if (m_currentMove == 0)
            if (m_mainline) {
                if (oldNext)
                    *oldNext = m_mainline;
                else
                    AnnotMove::deepDelete(m_mainline);

                m_mainline = 0;
            }

    }

    // prevPos *must* be initialised after restorePriorPosition() has been called
    Position prevPos(m_position);

    if (!move.parse(m_position, movetext.c_str())) {
        LOGERR << "Failed to parse move text '" << movetext << "'";
        return 0;
    }

    return makeMoveImpl(move, prevPos, annot, formattedMove, includeMoveNum, gameOver, oldNext);
}

AnnotMove *Game::makeMove(Move &move, const std::string *annot /*=0*/, std::string *formattedMove /*=0*/,
                          bool includeMoveNum /*=false*/, GameOver *gameOver /*=0*/, AnnotMove **oldNext /*=0*/) {
    if (m_variationStart) {
        if (!restorePriorPosition(m_currentMove))
            return 0;
    } else {
        if (m_currentMove == 0)
            if (m_mainline) {
                if (oldNext)
                    *oldNext = m_mainline;
                else
                    AnnotMove::deepDelete(m_mainline);

                m_mainline = 0;
            }

    }

    // prevPos *must* be initialised after restorePriorPosition() has been called
    Position prevPos(m_position);

    // The move might not have all the flags set, so set them now
    if (!move.complete(m_position)) {
        LOGERR << "Illegal move " << move << " (failed to complete)";
        return 0;
    }

    return makeMoveImpl(move, prevPos, annot, formattedMove, includeMoveNum, gameOver, oldNext);
}

AnnotMove *Game::makeMove(unsigned moveIndex, const std::string *annot /*=0*/, std::string *formattedMove /*=0*/,
                          bool includeMoveNum /*=false*/, GameOver *gameOver /*=0*/, AnnotMove **oldNext /*=0*/) {
    if (m_variationStart) {
        if (!restorePriorPosition(m_currentMove))
            return 0;
    } else {
        if (m_currentMove == 0)
            if (m_mainline) {
                if (oldNext)
                    *oldNext = m_mainline;
                else
                    AnnotMove::deepDelete(m_mainline);

                m_mainline = 0;
            }

    }

    // prevPos *must* be initialised after restorePriorPosition() has been called
    Position prevPos(m_position);
    Move moves[256];
    unsigned numMoves = prevPos.genMoves(moves);

    if (moveIndex >= numMoves) {
        LOGERR << "Move index out-of-range (" << moveIndex << " >= " << numMoves << ")";
        return 0;
    }

    Move move = moves[moveIndex];

    return makeMoveImpl(move, prevPos, annot, formattedMove, includeMoveNum, gameOver, oldNext);
}

AnnotMove *Game::makeMoveImpl(Move &move, const Position &prevPosition, const std::string *annot,
                              std::string *formattedMove, bool includeMoveNum, GameOver *gameOver,
                              AnnotMove **oldNext) {
    UnmakeMoveInfo umi;

    if (!m_position.makeMove(move, umi)) {
        LOGERR << "Illegal move " << move << " (failed to make move)";
        return 0;
    }

    if (gameOver)
        *gameOver = GAMEOVER_NOT;

    uint64_t hash = m_position.hashKey();
    string autoAnnot;

    // Use m_position.lastMove() as that will have updated flags (FL_CHECK etc.)
    AnnotMove *amove = new AnnotMove(m_position.lastMove(), hash);

    if (m_variationStart) {
        ASSERT(m_mainline);

        if (m_currentMove == 0) {
            m_mainline->addVariation(amove);

            if (m_mainline->mainline() == 0)
                m_mainline->setPriorPosition(prevPosition);
        } else {
            m_currentMove->addVariation(amove);

            if (m_currentMove->mainline() == 0)
                m_currentMove->setPriorPosition(prevPosition);
        }

        m_variationStart = false;
    } else {
        if (m_currentMove == 0) {
            ASSERT(m_mainline == 0);
            m_mainline = amove;
            amove->setPriorPosition(m_startPosition);
        } else {
            if (m_currentMove->next())
                m_currentMove->replaceNext(amove, oldNext);
            else
                m_currentMove->addMove(amove);
        }
    }

    m_currentMove = amove;

    if (gameOver) {
        *gameOver = isGameOver();

        if (*gameOver != GAMEOVER_NOT) {
            switch (*gameOver) {
            case GAMEOVER_MATE:
                amove->setFlags(Move::FL_MATE);
                break;

            case GAMEOVER_STALEMATE:
                amove->setFlags(Move::FL_DRAW);
                autoAnnot = "Stalemate";
                break;

            case GAMEOVER_50MOVERULE:
                amove->setFlags(Move::FL_DRAW);
                autoAnnot = "Draw by 50-move rule";
                break;

            case GAMEOVER_3FOLDREP:
                amove->setFlags(Move::FL_DRAW);
                autoAnnot = "Draw by 3-fold repetition";
                break;

            case GAMEOVER_NOMATERIAL:
                amove->setFlags(Move::FL_DRAW);
                autoAnnot = "Draw by insufficient material";
                break;

            default:
                ASSERT(false);
                break;
            }
        }
    }

    if (annot) {
        if (!annot->empty() && !autoAnnot.empty())
            amove->setPostAnnot(Util::format("%s. %s", annot->c_str(), autoAnnot.c_str()));
        else if (!annot->empty() && autoAnnot.empty())
            amove->setPostAnnot(*annot);
        else if (annot->empty() && !autoAnnot.empty())
            amove->setPostAnnot(autoAnnot);
    }

    if (formattedMove) {
        if (includeMoveNum)
            *formattedMove = prevPosition.moveNumber();

        *formattedMove += amove->san(prevPosition);
    }

    return amove;
}

AnnotMove *Game::annotMove(Move move) {
    UnmakeMoveInfo umi;

    ASSERT(!move.isNull());

    if (m_variationStart)
        return 0;

    Position posCopy(m_position);

    // The move might not have all the flags set, so set them now
    if (!move.complete(posCopy)) {
        LOGERR << "Illegal move " << move << " (failed to complete)";
        return 0;
    }

    if (!posCopy.makeMove(move, umi)) {
        LOGERR << "Illegal move " << move << " (failed to make move)";
        return 0;
    }

    return new AnnotMove(posCopy.lastMove(), 0ULL);
}

bool Game::startVariation() {
    if (m_variationStart) {
        LOGERR << "Cannot start a variation as one is already in progress";
        return false;
    }

    m_variationStart = true;
    return true;
}

bool Game::endVariation() {
    if (m_variationStart) {
        // A move wasn't actually made!
        LOGWRN << "The variation has no moves!";
        m_variationStart = false;
        return true;
    }

    // One of these should be true
    ASSERT(m_currentMove || m_mainline);

    // Go to the start of the line
    if (m_currentMove) {
        while (m_currentMove->prev())
            m_currentMove = m_currentMove->prev();

        // Go to the mainline
        ASSERT(m_currentMove->mainline());    // Else not a variation

        while (m_currentMove->mainline())
            m_currentMove = m_currentMove->mainline();

        // Set the position before the mainline move
        ASSERT(m_currentMove->priorPosition());
        m_position.set(m_currentMove->priorPosition());
    } else {
        setPositionToStart();
    }

    // Re-make the mainline move
    UnmakeMoveInfo umi;

    if (!m_position.makeMove(
            m_currentMove ? m_currentMove->move() : m_mainline->move(), umi)) {
        LOGERR << "Failed to re-make last move " << m_currentMove->dump() << " after variation end";
        return false;
    }

    return true;
}

AnnotMove *Game::addVariation(const vector<Move> &moveList) {
    if (m_currentMove == 0 ||
        moveList.empty()) {
        LOGERR << "No curent move or move list is empty";
        return 0;
    }

    bool ok = true;
    AnnotMove *firstMove = 0;
    if (startVariation()) {
        for (auto it = moveList.begin(); it != moveList.end() && ok; ++it) {
            Move move = *it;
            AnnotMove *amove = makeMove(move);
            if (amove == 0) {
                LOGERR << "Failed to make move " << move.dump();
                ok = false;
            } else if (firstMove == 0) {
                firstMove = amove;
            }
        }

        if (!endVariation()) {
            LOGERR << "Failed to end variation";
            ok = false;
        }
    } else {
        LOGERR << "Failed to start variation";
        ok = false;
    }

    if (!ok && firstMove) {
        // Remove the variation
        removeMove(firstMove);
        delete firstMove;
        firstMove = 0;
    }

    return firstMove;
}

bool Game::restorePriorPosition(const AnnotMove *amove) {
    return getPriorPosition(amove, m_position);
}

bool Game::getPriorPosition(const AnnotMove *amove, Position &position) {
    vector<Move> moves;
    moves.reserve(m_position.ply() + 1);
    bool first = true;

    if (amove) {
        while (amove->mainline())
            amove = amove->mainline();

        if (amove->priorPosition() == 0) {
            // Go to the start of the line.
            while (amove->prev()) {
                if (!first)
                    moves.push_back(amove->move());
                else
                    first = false;

                amove = amove->prev();
            }

            moves.push_back(amove->move());

            // If this is a variation then get the priorPosition from the mainline
            if (amove->mainline())
                while (amove->mainline())
                    amove = amove->mainline();

            // There should be a prior position here.
            if (amove->priorPosition() == 0) {
                LOGERR << "Expected a prior position at start of line!";
                return false;
            }
        }

        UnmakeMoveInfo umi;
        position.set(amove->priorPosition());

        for (vector<Move>::reverse_iterator it = moves.rbegin(); it < moves.rend(); ++it) {
            Move &move = *it;

            if (!position.makeMove(move, umi)) {
                LOGERR << "Failed to restore prior position by playing move " << move;
                return false;
            }
        }
    } else // amove == 0 (start of game)
        position.set(m_startPosition);

    return true;
}

void Game::removeMove(AnnotMove *amove, bool unlinkOnly /*=false*/) {
    if (amove) {
        if (amove == m_mainline) {
            removeMoves(unlinkOnly);
            return;
        }

        amove->remove(unlinkOnly);
    }
}

void Game::removeMoves(bool unlinkOnly /*=false*/) {
    if (!unlinkOnly)
        AnnotMove::deepDelete(m_mainline);

    m_mainline = 0;
    m_currentMove = 0;
}

bool Game::restoreMoves(AnnotMove *moves, AnnotMove **replaced /*=0*/) {
    if (moves->prev() == 0 && moves->mainline() == 0) {
        // Was the mainline move
        if (replaced)
            *replaced = m_mainline;

        m_mainline = moves;
        return true;
    }

    return moves->restore(replaced);
}

bool Game::promoteMove(AnnotMove *move) {
    ASSERT(m_mainline);
    ASSERT(move);
    bool mainlineAffected = move->isDirectVariation(m_mainline);

    if (move->promote()) {
        if (mainlineAffected && move->mainline() == 0)
            m_mainline = move;

        return true;
    }

    return false;
}

bool Game::demoteMove(AnnotMove *move) {
    ASSERT(m_mainline);
    ASSERT(move);
    bool mainlineAffected = move == m_mainline;

    if (move->demote()) {
        if (mainlineAffected)
            m_mainline = move->mainline();

        return true;
    }

    return false;
}

bool Game::promoteMoveToMainline(AnnotMove *move, unsigned *count) {
    ASSERT(m_mainline);
    ASSERT(move);
    bool mainlineAffected = move->isDirectVariation(m_mainline);

    if (move->promoteToMainline(count)) {
        if (mainlineAffected)
            m_mainline = move;

        return true;
    }

    return false;
}

Game::GameOver Game::isGameOver() {
    if (m_position.hmclock() >= 100)
        return GAMEOVER_50MOVERULE;

    // Test endgame situations first
    if (m_position.pieceCount(WHITE, QUEEN) == 0 && m_position.pieceCount(BLACK, QUEEN) == 0
        && m_position.pieceCount(WHITE, ROOK) == 0 && m_position.pieceCount(BLACK, ROOK) == 0
        && m_position.pieceCount(WHITE, PAWN) == 0 && m_position.pieceCount(BLACK, PAWN) == 0) {
        uint32_t wbishops = m_position.pieceCount(WHITE, BISHOP);
        uint32_t wknights = m_position.pieceCount(WHITE, KNIGHT);
        uint32_t bbishops = m_position.pieceCount(BLACK, BISHOP);
        uint32_t bknights = m_position.pieceCount(BLACK, KNIGHT);

        // K vs K, K vs K+B or K vs K+N is a draw
        if (wbishops + wknights + bbishops + bknights <= 1)
            return GAMEOVER_NOMATERIAL;

        // K+B vs K+B with same coloured bishops is a draw
        if (wbishops == 1 && bbishops == 1) {
            // There is only bishops one each side, therefore if the number on
            // the light or dark squares is zero they must be on the same
            // coloured squares...
            uint32_t wlight, wdark, blight, bdark;
            m_position.bishopSquares(WHITE, wlight, wdark);
            m_position.bishopSquares(BLACK, blight, bdark);

            if (wlight + blight == 0 || wdark + bdark == 0)
                return GAMEOVER_NOMATERIAL;
        }
    }

    // Check for mate/stalemate
    Move moves[256];
    unsigned numMoves = m_position.genMoves(moves);

    if (numMoves == 0)
        return (m_position.flags() & Position::FL_INCHECK) ? GAMEOVER_MATE : GAMEOVER_STALEMATE;

    // If there are two other instances of this key in the position history then the game
    // is drawn by 3-fold repetition.  This cannot be the case if the last move was a capture,
    // castling or promotion move.
    if (m_currentMove) {
        int count = 0;

        if (!m_currentMove->isCapture() && !m_currentMove->isCastle() && !m_currentMove->isPromotion())
            count = AnnotMove::countRepeatedPositions(m_currentMove);

        if (count >= 3)
            return GAMEOVER_3FOLDREP;
    }

    return GAMEOVER_NOT; // Game not over
}

bool Game::setMainline(const AnnotMove *amoves) {
    if (m_mainline) {
        logerr("Cannot set mainline moves as game already contains a mainline");
        return false;
    }

    m_mainline = AnnotMove::deepCopy(amoves);
    return true;
}

bool Game::findPosition(uint64_t hashKey, bool mainlineOnly, Position &foundPos) {
    return findPosition(hashKey, mainlineOnly, foundPos, m_startPosition, m_mainline);
}

bool Game::findPosition(uint64_t hashKey, bool mainlineOnly, Position &foundPos, const Position &currentPos,
                        const AnnotMove *move) {
    if (currentPos.hashKey() == hashKey) {
        foundPos = currentPos;
        return true;
    }

    Position pos = currentPos;

    while (move) {
        UnmakeMoveInfo umi;

        if (!pos.makeMove(move, umi)) {
            logerr("Failed to make move %s", move->dump(false).c_str());
            return false;
        }

        if (pos.hashKey() == hashKey) {
            foundPos = pos;
            return true;
        }

        if (!mainlineOnly && move->variation())
            for (const AnnotMove *var = move->variation(); var; var = var->variation())
                if (findPosition(hashKey, mainlineOnly, foundPos, pos, var))
                    return true;


        move = move->next();
    }

    return false;
}

Colour Game::currentMoveColour() const {
    return (m_position.ply() & 1) ? WHITE : BLACK;
}

bool Game::setCurrentMove(const AnnotMove *currentMove) {
#ifdef DEBUG

    if (m_mainline && currentMove)
        ASSERT(currentMove->isDescendant(m_mainline));
#endif // DEBUG

    if (currentMove == 0) {
        setPositionToStart();
    } else {
        if (!restorePriorPosition(currentMove))
            return false;

        UnmakeMoveInfo umi;

        if (!m_position.makeMove(currentMove->move(), umi)) {
            LOGERR << "Failed to re-make move " << currentMove->dump();
            return false;
        }
    }

    m_currentMove = const_cast<AnnotMove *> (currentMove);
    return true;
}

AnnotMove *Game::previousMove() {
    if (m_currentMove) {
        AnnotMove *prevMove = m_currentMove->prev();
        setCurrentMove(prevMove);
    }

    return m_currentMove;
}

AnnotMove *Game::nextMove() {
    AnnotMove *nextMove = 0;

    if (m_currentMove == 0)
        nextMove = m_mainline;
    else
        nextMove = m_currentMove->next();

    if (nextMove == 0) {
        // Don't allow the move to wrap around to the start (setCurrentMove(0) means 'go to start of game')
        return 0;
    }
    setCurrentMove(nextMove);
    return m_currentMove;
}

bool Game::isNextMove() {
    AnnotMove *nextMove = 0;

    if (m_currentMove == 0)
        nextMove = m_mainline;
    else
        nextMove = m_currentMove->next();

    return nextMove != 0;
}

void Game::setMoveAnnotations(const AnnotMove *move, const std::string &preAnnot, const std::string &postAnnot,
                              Nag nags[4]) {
    AnnotMove *nonConstMove = const_cast<AnnotMove *> (move);

    nonConstMove->setPreAnnot(preAnnot);
    nonConstMove->setPostAnnot(postAnnot);
    nonConstMove->setNags(nags);
}

void Game::setMoveNags(const AnnotMove *move, Nag nags[4]) {
    AnnotMove *nonConstMove = const_cast<AnnotMove *> (move);

    nonConstMove->setNags(nags);
}

bool Game::set(const string &input) {
    return PgnDatabase::readFromString(input, *this);
}

bool Game::get(string &output) const {

    return PgnDatabase::writeToString(*this, output);
}

ostream &operator<<(ostream &os, const Game &game) {
    string str;
    if (game.get(str))
        os << str;
    return os;
}

}   // namespace ChessCore
