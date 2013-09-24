//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Game.h: Game class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Mutex.h>
#include <ChessCore/GameHeader.h>
#include <ChessCore/Position.h>
#include <ChessCore/AnnotMove.h>

namespace ChessCore {
class CHESSCORE_EXPORT Game : public GameHeader {
private:
    static const char *m_classname;

public:
    enum GameOver {
        GAMEOVER_NOT = 0,           // Not game over
        GAMEOVER_MATE = 1,          // Mate
        GAMEOVER_STALEMATE = 2,     // Draw; stalemate
        GAMEOVER_50MOVERULE = 3,    // Draw; 50-move rule
        GAMEOVER_3FOLDREP = 4,      // Draw; 3-fold repetition
        GAMEOVER_NOMATERIAL = 5,    // Draw; Insufficient material
        GAMEOVER_TIME = 6           // Lost on time
    };

protected:
    static bool m_relaxedMode;

    Position m_startPosition;       // Game starting position
    Position m_position;            // Current position
    AnnotMove *m_mainline;          // First mainline move in doubly-linked list
    AnnotMove *m_currentMove;       // The current move (normally the last one made)
    bool m_variationStart;          // The next move made will be a variation of the current move

public:
    Game();
    Game(const Game &other);

    virtual ~Game();

    void init();

    /**
     * Set relaxed mode.
     *
     * @param relaxedMode true if relaxed mode
     */
    static inline void setRelaxedMode(bool relaxedMode) {
        m_relaxedMode = relaxedMode;
    }

    static inline bool relaxedMode() {
        return m_relaxedMode;
    }

    void setGame(const Game &other);

    /**
     * Return the starting position and a list of moves leading up to the specified move.
     *
     * @param lastMove The last move to include in the list of moves. This
     * may be 0 in order to get just the initial starting position.
     * @param moves Where to store the list of moves.
     *
     * @return true if the move list was generated successfully, else false.
     */
    bool moveList(const AnnotMove *lastMove, std::vector<Move> &moves) const;

    /**
     * Make a move on the board.
     *
     * @param movetext String containing move text.
     * @param annot If this is non-0 then it contains an annotation to add to move.
     * @param formattedMove If this is non-0 then the formatted move is stored in
     *        the string.
     * @param includeMoveNum If true then include the move number with the formatted
     *        move string.
     * @param gameOver If this is non-0 then the gameover indicator is stored here.
     * @param oldNext If this is non-0 then the move that was next is not
     *        deleted, instead a pointer to is copied here.
     *
     * @return The newly allocated AnnotMove object, else 0 if an error occurred.
     */
    AnnotMove *makeMove(const std::string &movetext, const std::string *annot = 0, std::string *formattedMove = 0,
                        bool includeMoveNum = false, GameOver *gameOver = 0, AnnotMove **oldNext = 0);

    /**
     * Make a move on the board.
     *
     * @param move The move to make.  The flags in the move are updated.
     * @param annot If this is non-0 then it contains an annotation to add to move.
     * @param formattedMove If this is non-0 then the formatted move is stored in
     *        the string.
     * @param includeMoveNum If true then include the move number with the formatted
     *        move string.
     * @param gameOver If this is non-0 then the gameover indicator is stored here.
     * @param oldNext If this is non-0 then the move that was next is not
     *        deleted, instead a pointer to is copied here.
     *
     * @return The newly allocated AnnotMove object, else 0 if an error occurred.
     */
    AnnotMove *makeMove(Move &move, const std::string *annot = 0, std::string *formattedMove = 0,
                        bool includeMoveNum = false, GameOver *gameOver = 0, AnnotMove **oldNext = 0);

    /**
     * Make a move on the board.
     *
     * @param moveIndex The index of the move in the moves array that will be generated
     *        from the position.
     * @param annot If this is non-0 then it contains an annotation to add to move.
     * @param formattedMove If this is non-0 then the formatted move is stored in
     *        the string.
     * @param includeMoveNum If true then include the move number with the formatted
     *        move string.
     * @param gameOver If this is non-0 then the gameover indicator is stored here.
     * @param oldNext If this is non-0 then the move that was next is not
     *        deleted, instead a pointer to is copied here.
     *
     * @return The newly allocated AnnotMove object, else 0 if an error occurred.
     */
    AnnotMove *makeMove(unsigned moveIndex, const std::string *annot = 0, std::string *formattedMove = 0,
                        bool includeMoveNum = false, GameOver *gameOver = 0, AnnotMove **oldNext = 0);

protected:
    /**
     * Common internal method used by public makeMove() methods.
     */
    AnnotMove *makeMoveImpl(Move &move, const Position &prevPosition, const std::string *annot,
                            std::string *formattedMove, bool includeMoveNum, GameOver *gameOver, AnnotMove **oldNext);

public:
    /**
     * Get the AnnotMove equivalent of the move.
     *
     * @param move The move.
     *
     * @return The AnnotMove object, which must be deleted when finished with.  0 is
     * returned if the move is illegal or if a variation has been started in the game
     * (see startVariation()/endVariation()).
     */
    AnnotMove *annotMove(Move move);

    /**
     * Indicate that the next call to makeMove() will be a variation of the current
     * move (m_move).
     *
     * @return true if the Game object is in the correct state to start the variation.
     */
    bool startVariation();

    /**
     * Finish the current variation line and restore the position and last move
     * to the mainline values.
     *
     * @return true if the Game object is in the correct state to end the variation.
     */
    bool endVariation();

    /**
     * Add a list of moves as a variation to the current move.  This method will
     * call startVariation() before, and endVariation() after, the variation moves
     * are added.
     *
     * @param moveList The variation to add to the current move.  The current
     * move will not change.
     *
     * @return The first move of the variation, or 0 if an error occurs.
     */
    AnnotMove *addVariation(const std::vector<Move> &moveList);

    /**
     * Restore the current position to that *before* the specified move.
     *
     * @param amove The move, which must be part of the game's move tree.
     *
     * @return true if the position was restored successfully, else false.
     */
    bool restorePriorPosition(const AnnotMove *amove);

    /**
     * Get the position before the specified move.
     *
     * @param amove The move, which must be part of the game's move tree.
     * @param position Where to store the position.
     *
     * @return true if the position was successfully retrieved, else false.
     */
    bool getPriorPosition(const AnnotMove *amove, Position &position);

    /**
     * Remove the specified move.
     *
     * @param amove The move to remove.  If amove == mainline() then removeMoves()
     *        is called.
     * @param unlinkOnly If true then don't delete the removed moves, just
     *        'unlink' them from the current moves.
     */
    void removeMove(AnnotMove *amove, bool unlinkOnly = false);

    /**
     * Remove all moves.
     *
     * @param unlinkOnly If true then don't delete the removed moves, just
     *        'unlink' them from the current moves.
     */
    void removeMoves(bool unlinkOnly = false);

    /**
     * Restore moves that were removed from the move tree.
     *
     * @param replaced If this is non-0, and this node replaces another node,
     *        then the replaced node is stored here.
     *
     * @return true if the node was restored successfully, else false.
     */
    bool restoreMoves(AnnotMove *moves, AnnotMove **replaced = 0);

    /**
     * Promote a variation move up the variation list, correcting the mainline
     * move if it is affected.
     *
     * @param move The move to promote.
     *
     * @return true if the move was promoted successfully, else false.
     */
    bool promoteMove(AnnotMove *move);

    /**
     * Demote a move down the variation list, correcting the mainline move if
     * it is affected.
     *
     * @param move The move to demote.
     *
     * @return true if the move was demoted successfully, else false.
     */
    bool demoteMove(AnnotMove *move);

    /**
     * Promote a move to the mainline, correcting the mainline move if it
     * is affected.
     *
     * @param move The move to promote.
     * @param count Optional pointer to store the number of promotions
     * that were needed to make the move the mainline move.
     *
     * @return true if the move was promoted to the mainline.
     */
    bool promoteMoveToMainline(AnnotMove *move, unsigned *count = 0);

    /**
     * Check if the game is over.
     *
     * This method can detect all gameover conditions apart from:
     *   GAMEOVER_TIME: this must be done by a time-keeping element.
     *
     * @return GAMEOVER_xxx value.
     */
    GameOver isGameOver();

    inline const Position &startPosition() const {
        return m_startPosition;
    }

    inline std::string startPositionFen() const {
        return m_startPosition.fen();
    }

    inline void setStartPosition(const Position &pos) {
        m_startPosition.set(pos);
    }

    inline void setStartPosition(const Position *pos) {
        m_startPosition.set(pos);
    }

    inline Position::Legal setStartPosition(const char *fen) {
        return m_startPosition.setFromFen(fen);
    }

    inline Position::Legal setStartPosition(const std::string &fen) {
        return m_startPosition.setFromFen(fen.c_str());
    }

    inline bool isPartialGame() const {
        return !m_startPosition.isStarting();
    }

    inline Position &position() {
        return m_position;
    }

    inline const Position &position() const {
        return m_position;
    }

    inline void setPosition(const Position &pos) {
        m_position.set(pos);
    }

    inline void setPosition(const Position *pos) {
        m_position.set(pos);
    }

    inline void setPositionToStart() {
        m_position.set(m_startPosition);
        m_currentMove = 0;
    }

    inline uint16_t ply() const {
        return m_position.ply();
    }

    inline const AnnotMove *mainline() const {
        return m_mainline;
    }

    inline AnnotMove *mainline() {
        return m_mainline;
    }

    /**
     * Set the mainline with a *deep copy* of the specified moves.
     *
     * @param amoves The moves to set as the mainline.
     *
     * @return false if a mainline already exists, else true.
     */
    bool setMainline(const AnnotMove *amoves);

    inline const AnnotMove *currentMove() const {
        return m_currentMove;
    }

    inline AnnotMove *currentMove() {
        return m_currentMove;
    }

    /**
     * Find the specified position in the game.
     *
     * @param hashKey The hash key of the position to find.
     * @param mainlineOnly If true then search the mainline only, else
     *        search all variations as well.
     * @param foundPos Where to store the found position.
     *
     * @return true if the position was found, else false.
     */
    bool findPosition(uint64_t hashKey, bool mainlineOnly, Position &foundPos);

protected:
    /**
     * Helper for findPosition().
     */
    bool findPosition(uint64_t hashKey, bool mainlineOnly, Position &foundPos, const Position &currentPos,
                      const AnnotMove *move);

public:
    Colour currentMoveColour() const;

    /**
     * Set the 'current move' to the specified move.  This will
     * reset the position in the game to the position prior to
     * the current move.
     *
     * @param currentMove The move to set as the current move.  If
     * this is 0 then the current move is set to the first move and the
     * position is set to the starting position.
     *
     * @return true if the move was set correctly, else false.
     */
    bool setCurrentMove(const AnnotMove *currentMove);

    /**
     * Get the previous to the current move.  This will reset the position
     * in the game to the position prior to the previous move.
     *
     * @return The previous move, which will become the new current move.
     */
    AnnotMove *previousMove();

    /**
     * Get the move after the current move.  This will reset the position
     * in the game to the position prior to the next move.
     *
     * @return The next move, which will become the new current move.
     */
    AnnotMove *nextMove();

    /**
     * Test if a move follows the current move.
     *
     * @return true if a move follows the current move, else false.
     */
    bool isNextMove();

    /**
     * @return The number of half-moves in the mainline.
     */
    inline unsigned countMainline() const {
        return AnnotMove::count(m_mainline);
    }

    /**
     * @return The 'ply' (halfmove) of the next expected move.  This will
     * vary depending on whether a variation has started or not.
     */
    inline unsigned nextPly() const {
        return (unsigned)m_position.ply() + (m_variationStart ? 0 : 1);
    }

    /**
     * Update the annotation strings and NAG symbols of the specified move.
     *
     * @param move The move to update.
     * @param preAnnot The pre-move annotation string.
     * @param postAnnot The post-move annotation string.
     * @param nags The NAG symbols.
     */
    void setMoveAnnotations(const AnnotMove * move, const std::string & preAnnot,
                            const std::string & postAnnot, Nag nags[4]);

    /**
     * Update the NAG symbols of the specified move.
     *
     * @param move The move to update.
     * @param nags The NAG symbols.
     */
    void setMoveNags(const AnnotMove * move, Nag nags[4]);

    /**
     * Set the game object from a string, in PGN format.
     *
     * @param input The string containing the game information in PGN format.
     *
     * @return true if the game was created successfully, else false.
     */
    bool set(const std::string &input);

    /**
     * Get the game object to a string, in PGN format.
     *
     * @param output Where to store the game information in PGN format.
     *
     * @return true if the game was retrieved successfully, else false.
     */
    bool get(std::string &output) const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Game &game);
};

} // namespace ChessCore
