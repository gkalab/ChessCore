//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// OpeningTree.cpp: Opening Tree class definitions implementation.
//

#include <ChessCore/OpeningTree.h>
#include <ChessCore/Log.h>
#include <iomanip>

using namespace std;

namespace ChessCore {
const char *OpeningTree::m_classname = "OpeningTree";

OpeningTree::OpeningTree(const std::string &filename):m_longestLine(0) {
    m_db = Database::openDatabase(filename, false);
    if (m_db.get() == 0) {
        LOGERR << "Failed to open database '" << filename << "': " << m_db->errorMsg();
        return;
    }

    if (!m_db->supportsOpeningTree()) {
        LOGERR << "Database '" << filename << "' does not support opening trees";
        return;
    }

    if (m_db->countLongestLine(m_longestLine))
        LOGDBG << "Longest line: " << m_longestLine;
    else
        LOGWRN << "Failed to get the longest line length: " << m_db->errorMsg();
}

OpeningTree::~OpeningTree() {
    if (m_db) {
        m_db->close();
        m_db = 0;
    }
}

bool OpeningTree::classify(const Game &game, string &eco, string &opening, string &variation) {
    if (!isOpen()) {
        LOGERR << "Database is not open";
        return false;
    }

    eco.clear();
    opening.clear();
    variation.clear();

    bool retval = false;
    Position pos = game.startPosition();
    Position prevPos;
    unsigned depth = 0, count = 0, prevCount = 0;

    // Move through the game until a move no longer appears in the ECO
    // Classification file.
    for (const AnnotMove *move = game.mainline(); move; move = move->next()) {
        prevPos = pos;
        prevCount = count;
        depth++;
        UnmakeMoveInfo umi;

        if (!pos.makeMove(move, umi)) {
            LOGERR << "Failed to make move " << move->dump(false) << endl;
            return false;
        }

        if (!m_db->countInOpeningTree(pos.hashKey(), count)) {
            LOGERR << "Failed to get count of position in database: " << m_db->errorMsg();
            return false;
        }

        if (count == 0)
            break;
    }

    if (prevCount > 0) {
        ASSERT(prevPos.hashKey());

        // The position we want is in prevPos
        vector<OpeningTreeEntry> entries;
        unsigned gameNum = 0; // The game to use
        size_t count;

        for (int attempt = 1; attempt <= 2 && gameNum == 0; attempt++) {
            if (m_db->searchOpeningTree(prevPos.hashKey(), attempt == 1, entries)) {
                count = entries.size();
                LOGDBG << "Matched " << count << " positions on attempt " << attempt;

                if (count > 0) {
                    // Use the first entry
                    auto it = entries.begin();
                    gameNum = it->gameNum();
                }
            } else {
                LOGERR << "Failed to select opening tree on attempt " << attempt <<
                    ": " << m_db->errorMsg();
            }
        }

        if (gameNum) {
            GameHeader gameHeader;

            if (m_db->readHeader(gameNum, gameHeader)) {
                eco = gameHeader.eco();
                opening = gameHeader.white().lastName();
                variation = gameHeader.black().lastName();

                retval = true;
            } else {
                LOGERR << "Failed to read game " << gameNum;
            }
        }
    } else {
        LOGWRN << "Could not find opening classification for game";
    }

    return retval;
}

bool OpeningTree::classify(Game &game, bool setComment /*=true*/) {
    string eco, opening, variation;

    if (!classify(game, eco, opening, variation)) {
        LOGDBG << "Failed to classify opening";
        return false;
    }

    LOGDBG << "Classified opening: " << eco << " " << opening << " / " << variation;

    game.setEco(eco);

    if (setComment) {
        ASSERT(!opening.empty());
        AnnotMove *move = game.mainline();

        if (move) {
            if (!variation.empty())
                move->setPreAnnot(opening + " / " + variation);
            else
                move->setPreAnnot(opening);
        }
    }

    return true;
}
}   // namespace ChessCore
