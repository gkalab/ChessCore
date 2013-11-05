//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// AnnotMove.h: AnnotMove class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Move.h>
#include <vector>
#include <ostream>

namespace ChessCore {
const size_t STORED_NAGS = 4;

class AnnotMove;
class Position;

typedef uint8_t Nag;

enum {
    NAG_NONE,               // 0
    NAG_GOOD_MOVE,          // 1
    NAG_EXCELLENT_MOVE,     // 2
    NAG_MISTAKE,            // 3
    NAG_BLUNDER,            // 4
    NAG_INTERESTING_MOVE,   // 5
    NAG_DUBIOUS_MOVE,       // 6
    NAG_WHITE_SLIGHT_ADV,   // 7
    NAG_BLACK_SLIGHT_ADV,   // 8
    NAG_WHITE_ADV,          // 9
    NAG_BLACK_ADV,          // 10
    NAG_WHITE_DECISIVE_ADV, // 11
    NAG_BLACK_DECISIVE_ADV, // 12
    NAG_EVEN,               // 13
    NAG_UNCLEAR,            // 14
    NAG_COMP_FOR_MATERIAL,  // 15
    NAG_DEVELOPMENT_ADV,    // 16
    NAG_SPACE_ADV,          // 17
    NAG_WITH_ATTACK,        // 18
    NAG_WITH_INITIATIVE,    // 19
    NAG_WITH_COUNTER_PLAY,  // 20
    NAG_ZUGZWANG,           // 21
    NAG_WITH_THE_IDEA,      // 22
    NAG_ONLY_MOVE,          // 23
    NAG_BETTER_IS,          // 24
    NAG_FILE,               // 25
    NAG_DIAGONAL,           // 26
    NAG_CENTRE,             // 27
    NAG_KINGSIDE,           // 28
    NAG_QUEENSIDE,          // 29
    NAG_WEAK_POINT,         // 30
    NAG_ENDING,             // 31
    NAG_BISHOP_PAIR,        // 32
    NAG_OPP_COLOURED_BISHOP_PAIR, // 33
    NAG_SAME_COLOURED_BISHOP_PAIR, // 34
    NAG_UNITED_PAWNS,       // 35
    NAG_SEPARATED_PAWNS,    // 36
    NAG_DOUBLED_PAWNS,      // 37
    NAG_PASSED_PAWN,        // 38
    NAG_PAWN_ADV,           // 39
    NAG_TIME,               // 40
    NAG_NOVELTY,            // 41
    NAG_WITH,               // 42
    NAG_WITHOUT,            // 43
    NAG_ETC,                // 44
    NAG_WORSE_IS,           // 45
    NAG_DIAGRAM,            // 46
    NAG_DIAGRAM_FLIPPED,    // 47
    NAG_EDITORIAL_COMMENT,  // 48
    // ---
    NUM_NAGS                // 49
};

// Annotations saved during delete annotation operations
struct SavedAnnotations {
    AnnotMove *move;        // The move that owned the annotations
    std::string preAnnot;
    std::string postAnnot;
    Nag nags[STORED_NAGS];
};

class CHESSCORE_EXPORT AnnotMove : public Move {
private:
    static const char *m_classname;

protected:
    AnnotMove *m_prev;              // Previous move
    AnnotMove *m_next;              // Next move
    AnnotMove *m_mainline;          // 'Parent' variation (0 if not a variation)
    AnnotMove *m_variation;         // Next variation of this move
    Position *m_priorPosition;      // Position *before* this move was made (variation starts only)
    uint64_t m_posHash;             // Zobrist hash of position *after* move was made
    std::string m_preAnnot;         // Annotation before move
    std::string m_postAnnot;        // Annotation after move
    Nag m_nags[STORED_NAGS];        // Numberic Annotation Glyphs

public:
    AnnotMove();
    AnnotMove(const Move &other);
    AnnotMove(const Move &other, uint64_t posHash);
    AnnotMove(const AnnotMove &other);
    AnnotMove(const AnnotMove *other);
    virtual ~AnnotMove();

    /**
     * Delete the specified move and any following or variation moves.
     *
     * @param amove The move to recursively delete.
     */
    static void deepDelete(AnnotMove *amove);

    /**
     * Make a 'deep' copy of a tree of AnnotMove objects.
     *
     * @param amove The first move in the move tree to copy.
     *
     * @return A copy of the move tree.
     */
    static AnnotMove *deepCopy(const AnnotMove *amove);

    /**
     * Remove and delete any variations from the move list
     *
     * @param amove The first move in the move list to remove variations
     * from
     * @param removed Optional vector to store the removed variations.
     */
    static void removeVariations(AnnotMove *amove, std::vector<AnnotMove *> *removed = 0);

    /**
     * Generate a move list from a vector of Move objects.
     *
     * @param moves Astd::vectorof Move objects to make the move list from.
     *
     * @return The move list.
     */
    static AnnotMove *makeMoveList(const std::vector<Move> &moves);

    /**
     * Helper method to make it easier to get to the base Move object.
     */
    Move move() const {
        return static_cast<Move> (this);
    }

    /**
     * Add a move to the end of the move list.
     *
     * @param amove The move to add.
     */
    void addMove(AnnotMove *amove);

    /**
     * Add a move to the end of the variation list.
     *
     * @param variation The variation move to add.
     * @param atEnd If true then the variation is added as the last in
     *        the list, else it is added as the first variation.
     */
    void addVariation(AnnotMove *variation, bool atEnd = true);

    /**
     * Promote this (variation) move up the variation list.
     *
     * @return true if the move was promoted successfully, else false.
     */
    bool promote();

    /**
     * Demote this move down the variation list.
     *
     * @return true if the move was demoted successfully, else false.
     */
    bool demote();

    /**
     * Promote this (variation) move to the mainline.
     *
     * @param count Optional pointer to store the number of promotions
     * that were needed to make the move the mainline move.
     *
     * @return true if the move was promoted to the mainline.
     */
    bool promoteToMainline(unsigned *count = 0);

    /**
     * Replace the next (and all following) move with the optional
     * move.
     *
     * @param amove The optional move to replace the next move.  If this
     *        is 0 then the next moves are simply removed.
     * @param oldNext If this is non-0 then the move that was next is not
     *        deleted; instead a pointer to is copied here.
     */
    void replaceNext(AnnotMove *amove, AnnotMove **oldNext = 0);

    /**
     * Remove the node from the tree and delete it.
     *
     * @param unlinkOnly If true then don't delete the removed moves, just
     *        'unlink' them from the current moves.
     *
     * @return The previous move, if there is one.
     */
    AnnotMove *remove(bool unlinkOnly = false);

    /**
     * Restore a node that was remove from the tree.
     *
     * @param replaced If this is non-0, and this node replaces another node,
     *        then the replaced node is stored here.
     *
     * @return true if the node was restored successfully, else false.
     */
    bool restore(AnnotMove **replaced = 0);

    const AnnotMove *prev() const {
        return m_prev;
    }

    AnnotMove *prev() {
        return m_prev;
    }

    const AnnotMove *next() const {
        return m_next;
    }

    AnnotMove *next() {
        return m_next;
    }

    const AnnotMove *mainline() const {
        return m_mainline;
    }

    AnnotMove *mainline() {
        return m_mainline;
    }

    const AnnotMove *variation() const {
        return m_variation;
    }

    AnnotMove *variation() {
        return m_variation;
    }

    const Position *priorPosition() const {
        return m_priorPosition;
    }

    Position *priorPosition() {
        return m_priorPosition;
    }

    void setPriorPosition(const Position &priorPosition);

    void clearPriorPosition();

    uint64_t posHash() const {
        return m_posHash;
    }

    void setPosHash(uint64_t posHash) {
        m_posHash = posHash;
    }

    /**
     * Determine if the move has any annotations or NAGs.
     *
     * @return true If the move has any annotations or NAGs.
     */
    bool hasAnnotations() const;

    /**
     * Determine if the line, starting with this move, has any
     * annotations or NAGs.
     *
     * @return true If the line has any annotations or NAGs.
     */
    bool lineHasAnnotations() const;

    /**
     * Get the top mainline of a variation.
     *
     * @return The top mainline of the variation.
     */
    const AnnotMove *topMainline() const;
    AnnotMove *topMainline();

    inline const std::string &preAnnot() const {
        return m_preAnnot;
    }

    inline void setPreAnnot(const std::string &preAnnot) {
        m_preAnnot = preAnnot;
    }

    inline const std::string &postAnnot() const {
        return m_postAnnot;
    }

    inline void setPostAnnot(const std::string &postAnnot) {
        m_postAnnot = postAnnot;
    }

    /**
     * Remove all annotations and NAGs.
     *
     * @param savedAnnotations Optional pointer to save the removed annotations.
     */
    void removeAnnotations(SavedAnnotations *savedAnnotations = 0);

    /**
     * Remove all annotations and NAGs in the line, starting with this move.
     *
     * @param removed Optional vector of saved variations where the removed
     * annotations can be stored.
     */
    void removeLineAnnotations(std::vector<SavedAnnotations> *removed = 0);

    /**
     * Remove all NAGs
     */
    void clearNags();

    /**
     * Get the move NAGs.
     *
     * @param nags The Nag array, in which to copy the move NAGs into.
     *
     * @return The number of NAGs written to the array.
     */
    unsigned nags(Nag nags[STORED_NAGS]) const;

    /**
     * Set the move NAGs.
     *
     * @param nags The Nag array terminated with NAG_NONE or upto the length of STORED_NAGS.
     *
     * @return The number of NAGs set from the array.
     */
    unsigned setNags(const Nag *nags);

    /**
     * Add a NAG.
     *
     * @param nag The NAG to add.
     *
     * @return true if the NAG was added, or false if the NAG was ignored (as
     * it is already in the move or it's NAG_NONE).
     */
    bool addNag(Nag nag);

    /**
     * Test if the move has the specified NAG.
     *
     * @param nag The NAG to test for.
     *
     * @return true if the move has the NAG, else false.
     */
    bool hasNag(Nag nag) const;

    /**
     * Return the number of NAGs
     *
     * @return The number of NAGs.
     */
    unsigned nagCount() const;

    /**
     * Save the move's annotations.
     *
     * @param savedAnnotations Where to store the move's annotations.
     */
    void saveAnnotations(SavedAnnotations &savedAnnotations) const;

    /**
     * Restore the move's annotations.
     *
     * @param savedAnnotations Where to restore the move's annotations from.
     */
    void restoreAnnotations(const SavedAnnotations &savedAnnotations);

    /**
     * @return The first move in the move list.
     */
    AnnotMove *firstMove();
    const AnnotMove *firstMove() const;

    /**
     * @return The last move in the move list.
     */
    AnnotMove *lastMove();
    const AnnotMove *lastMove() const;

    /**
     * @return The previous variation.
     */
    AnnotMove *previousVariation();
    const AnnotMove *previousVariation() const;

    /**
     * @return The next variation.
     */
    AnnotMove *nextVariation();
    const AnnotMove *nextVariation() const;

    /**
     * Determine if the line, starting with this move, has any variations.
     *
     * @return true if the line has variations.
     */
    bool lineHasVariations() const;

    /**
     * Determine if this move is part of a variation line.
     */
    bool isInVariation() const {
        return previousVariation() != 0;
    }

    /**
     * Determine the level of variation this is in.
     */
    unsigned variationLevel() const;

    /**
     * Determine if the specified move is a descendant (made after) this move.
     *
     * @param amove The potential descendant.
     *
     * @return true if the move is a descendant, else false.
     */
    bool isDescendant(const AnnotMove *amove) const;

    /**
     * Determine if the specified move is a direct variation of this move.
     *
     * @param amove The potential variation.
     *
     * @return true if the move is a direct variation, else false.
     */
    bool isDirectVariation(const AnnotMove *amove) const;

    /**
     * Count the number of moves in a line.
     *
     * @param amove The first move of the line.  This may be 0.
     *
     * @return The number of moves in a line.
     */
    static unsigned count(const AnnotMove *amove);

    /**
     * Count the number of entities in the line.
     *
     * Note that moveCount, variationCount and annotationLength should be
     * initialise to 0 before calling this method.
     *
     * @param amove The move of the line.  This may be 0.
     * @param moveCount Will hold the total number of moves in the line.
     * @param variationCount Will hold the total number of variations in the line.
     * @param symbolCount Will hold the total number of symbols in the line.
     * @param annotationsLength Will hold the total number bytes used to store
     * variation text, including trailing nul-terminators.
     */
    static void count(const AnnotMove *amove, unsigned &moveCount, unsigned &variationCount, unsigned &symbolCount,
                      unsigned &annotationsLength);

    /**
     * Count the number of times the position in the specified move has
     * occurred before.
     *
     * @param amove The move with the position to count.
     *
     * @return The number of times the position appears in the move tree
     * (always at least once as the 'amove' position is always counted).
     */
    static unsigned countRepeatedPositions(const AnnotMove *amove);

    /**
     * Dump the line recursively to a file in 'dot' file.
     *
     * @param filename The file to dump the moves to.
     *
     * @return true if the moves were dumped successfully, else false.
     */
    static bool writeToDotFile(const AnnotMove *line, const std::string &filename);

private:
    static bool writeToDotFile(const AnnotMove *line, std::ostream &os);

public:
    /**
     * @return A dump of this move and all following and variation moves.
     */
    std::string dumpLine() const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const AnnotMove &move);
};

}   // namespace ChessCore
