//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// CbhDatabase.h: ChessBase CBH Database class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Database.h>
#include <fstream>

namespace ChessCore {

// CBH header
struct CHESSCORE_EXPORT CbhHeader {
    uint32_t numGames;

    std::string dump() const;
};

// CBH record
struct CHESSCORE_EXPORT CbhRecord {
    uint8_t flags;
    uint32_t cbgIndex;
    uint32_t cbaIndex;
    uint32_t cbpWhiteIndex;
    uint32_t cbpBlackIndex;
    uint32_t cbtIndex;
    uint32_t cbcIndex;
    uint32_t cbsIndex;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint16_t result;
    uint8_t roundMajor;
    uint8_t roundMinor;
    uint16_t whiteElo;
    uint16_t blackElo;
    uint16_t eco;
    bool partialGame;

    std::string dump() const;
};

// Binary tree header (used by CBP, CBT, CBC, CBS, CBE files)
struct CHESSCORE_EXPORT CbTreeHeader {
    uint32_t numRecords;
    uint32_t rootRecord;
    uint32_t recordSize;
    uint32_t firstDeleted;
    uint32_t existingRecords;

    std::string dump() const;
};

// CBP record
struct CHESSCORE_EXPORT CbpRecord {
    uint32_t leftChild;
    uint32_t rightChild;
    uint8_t height;
    std::string lastName;
    std::string firstName;
    uint32_t numGames;
    uint32_t firstGameIndex;

    std::string dump() const;
};

class CHESSCORE_EXPORT CbhDatabase : public Database {
private:
    static const char *m_classname;

protected:
    unsigned m_numGames;

    std::string m_filename;         // The name of the .CBH file
    std::fstream m_cbhFile;         // Header
    std::fstream m_cbgFile;         // Game data
    std::fstream m_cbaFile;         // Game annotations
    std::fstream m_cbpFile;         // Player data
    std::fstream m_cbtFile;         // Tournament data
    std::fstream m_cbcFile;         // Annotator data
    std::fstream m_cbsFile;         // Source data

    CbTreeHeader m_cbpHeader;       // Header from CBP (player) file

public:
    CbhDatabase();
    CbhDatabase(const std::string &filename, bool readOnly);
    virtual ~CbhDatabase();

    const char *databaseType() const {
        return "CBH";
    }

    bool supportsOpeningTree() const {
        return false;
    }

    bool needsIndexing() const {
        return false;
    }

    bool supportsSearching() const {
        return false;
    }

    bool open(const std::string &filename, bool readOnly);
    bool close();
    bool readHeader(unsigned gameNum, GameHeader &gameHeader);
    bool read(unsigned gameNum, Game &game);
    bool write(unsigned gameNum, const Game &game);

    unsigned numGames();
    unsigned firstGameNum();
    unsigned lastGameNum();
    bool gameExists(unsigned gameNum);

    inline const std::string &filename() const {
        return m_filename;
    }

protected:
    bool cbhReadHeader(CbhHeader &cbhHeader);
    bool cbhReadRecord(unsigned gameNum, CbhRecord &cbhRecord);
    bool cbpReadHeader(CbTreeHeader &cbpHeader);
    bool cbpReadRecord(unsigned index, CbpRecord &cbpRecord);

    bool readFile(std::fstream &file, const char *filetype, unsigned offset, void *buffer, unsigned len);
};
} // namespace ChessCore
