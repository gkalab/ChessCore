//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// OpeningTree.h: Opening Tree class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Move.h>
#include <ChessCore/Database.h>
#include <string>
#include <sstream>

namespace ChessCore {
class CHESSCORE_EXPORT OpeningTreeEntry {
protected:
    uint64_t m_hashKey;
    Move m_move;
    int m_score;                // +1: white win, 0: draw or not finished, -1: black wins
    bool m_lastMove;            // true if last move in game
    unsigned m_gameNum;         // Database game number (if known)

public:
    OpeningTreeEntry():m_hashKey(0ULL), m_move(), m_score(0), m_lastMove(false), m_gameNum(0) {
    }

    OpeningTreeEntry(uint64_t hashKey, uint64_t prevHashKey, Move move, int score, bool lastMove,
                     unsigned gameNum):m_hashKey(hashKey), m_move(move), m_score(score), m_lastMove(lastMove),
        m_gameNum(
            gameNum) {
    }

    virtual ~OpeningTreeEntry() {
    }

    void init() {
        m_hashKey = 0ULL;
        m_move.init();
        m_score = 0;
        m_lastMove = false;
        m_gameNum = 0;
    }

    uint64_t hashKey() const {
        return m_hashKey;
    }

    void setHashKey(uint64_t hashKey) {
        m_hashKey = hashKey;
    }

    Move move() const {
        return m_move;
    }

    void setMove(Move move) {
        m_move = move;
    }

    int score() const {
        return m_score;
    }

    void setScore(int score) {
        m_score = score;
    }

    bool lastMove() const {
        return m_lastMove;
    }

    void setLastMove(bool lastMove) {
        m_lastMove = lastMove;
    }

    unsigned gameNum() const {
        return m_gameNum;
    }

    void setGameNum(unsigned gameNum) {
        m_gameNum = gameNum;
    }

    std::string dump() const {
        return Util::format("m_hashKey=0x%016llx, m_move=0x%04x, m_score=%d, m_lastMove=%s, m_gameNum=%u",
                            m_hashKey, (unsigned)m_move.intValue(), m_score, m_lastMove ? "true" : "false", m_gameNum);
    }
};

class CHESSCORE_EXPORT OpeningTree {
private:
    static const char *m_classname;

protected:
    std::shared_ptr<Database> m_db;
    unsigned m_longestLine;

public:
    OpeningTree(const std::string &filename);
    virtual ~OpeningTree();

    bool isOpen() const {
        return m_db && m_db->isOpen();
    }

    /**
     * Get the longest line in the classification database.
     */
    unsigned longestLine() const {
        return m_longestLine;
    }

    /**
     * Classify a game.
     *
     * @param game The game to classify.
     * @param eco Where to store the classified ECO code.
     * @param opening Where to store the name of the opening.
     * @param variation Where to store the name of the opening variation.
     *
     * @return true if the game was successfully classified, else false.
     */
    bool classify(const Game &game, std::string &eco, std::string &opening, std::string &variation);

    /**
     * Classify a game.
     *
     * @param game The game to classify.
     * @param setComment If true then the opening/variation text are stored as
     * a pre-annotation comment of the first move of the game.  Else the opening/variation
     * are discarded.
     *
     * @return true if the game was successfully classified, else false.
     */
    bool classify(Game &game, bool setComment = true);
};
} // namespace ChessCore
