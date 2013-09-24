//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// PgnDatabase.h: PGN Database class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/PgnScanner.h>
#include <ChessCore/Database.h>
#include <ChessCore/IndexManager.h>
#include <fstream>

namespace ChessCore {
class CHESSCORE_EXPORT PgnDatabase : public Database {
private:
    static const char *m_classname;

protected:
    // m_relaxedParsing allows 'spurious characters' to be ignored when parsing
    static bool m_relaxedParsing;

    // Manages the PGN index files in a temporary directory.  This is intialised
    // the first time this class is used.
    static IndexManager m_indexManager;

    // m_nagMap maps PGN tags (the index of the array) to AnnotMove::Nag's
    enum {
        NUM_PGN_NAGS = 256
    };
    static Nag m_nagMap[NUM_PGN_NAGS];

    std::string m_pgnFilename;
    std::fstream m_pgnFile;
    std::string m_indexFilename;
    std::fstream m_indexFile;
    PgnScannerContext m_context;
    unsigned m_numGames;

public:
    static bool isRelaxedParsing() {
        return m_relaxedParsing;
    }

    static void setRelaxedParsing(bool relaxedParsing) {
        m_relaxedParsing = relaxedParsing;
    }

    static Nag fromPgnNag(unsigned nag);
    static unsigned toPgnNag(Nag nag);

    static bool initIndexManager();

    PgnDatabase();
    PgnDatabase(const std::string &filename, bool readOnly);
    virtual ~PgnDatabase();

    const char *databaseType() const {
        return "PGN";
    }

    bool supportsOpeningTree() const {
        return false;
    }

    bool needsIndexing() const {
        return true;
    }

    bool supportsSearching() const {
        return false;
    }

    bool open(const std::string &filename, bool readOnly);
    bool close();
    bool readHeader(unsigned gameNum, GameHeader &gameHeader);
    bool read(unsigned gameNum, Game &game);
    bool write(unsigned gameNum, const Game &game);

    bool hasValidIndex();
    bool index(DATABASE_CALLBACK_FUNC callback, void *contextInfo);

    // Special methods to allow games to be read from/written to strings
    // (as PGN is the standard game interchange format).
    static bool readFromString(const std::string &input, Game &game);
    static unsigned readMultiFromString(const std::string &input, std::vector<Game *> &games,
                                        DATABASE_CALLBACK_FUNC callback, void *contextInfo);
    static bool writeToString(const Game &game, std::string &output);

    unsigned numGames() {
        return m_numGames;
    }

    unsigned firstGameNum() {
        return m_numGames > 0 ? 1 : 0;
    }

    unsigned lastGameNum() {
        return m_numGames;
    }

    bool gameExists(unsigned gameNum) {
        return gameNum >= firstGameNum() && gameNum <= lastGameNum();
    }

    inline const std::string &filename() const {
        return m_pgnFilename;
    }

protected:
    static bool read(PgnScannerContext &context, Game &game, std::string &errorMsg);
    static bool readRoster(PgnScannerContext &context, int token, GameHeader &gameHeader, std::string &errorMsg);
    static bool write(std::ostream &output, const Game &game, std::string &errorMsg);
    static bool writeMoves(std::ostream &output, const AnnotMove *amove, unsigned &width, std::string &errorMsg);
    static void writeText(std::ostream &output, const std::string &text, unsigned &width, bool insertSpace = true);
    static std::string getTagString(PgnScannerContext &context, std::string &errorMsg);
    static std::string formatTagString(const std::string &str);
    static void setPlayerName(Player &player, const std::string &data);
    static void setOpening(Player &player, const std::string &data);
    bool seekGameNum(unsigned gameNum, uint32_t &linenum);

public:
    bool readIndex(unsigned gameNum, uint64_t &offset, uint32_t &linenum);
    bool writeIndex(unsigned gameNum, uint64_t offset, uint32_t linenum);
};
} // namespace ChessCore
