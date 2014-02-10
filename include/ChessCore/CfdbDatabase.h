//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// CfdbDatabase.h: ChessFront Database class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Database.h>
#include <ChessCore/Player.h>
#include <ChessCore/Blob.h>
#include <ChessCore/Bitstream.h>

#include <sstream>
#include <sqlite3.h>

namespace ChessCore {
class CHESSCORE_EXPORT CfdbDatabase:public Database {
private:
    static const char *m_classname;

protected:
    enum {
        CURRENT_SCHEMA_VERSION = 1
    };

    static unsigned m_sqliteVersion;

    std::string m_filename;
    sqlite3 *m_db;

public:
    static unsigned currentSchemaVersion() {
        return CURRENT_SCHEMA_VERSION;
    }

    static unsigned sqliteVersion() {
        return m_sqliteVersion;
    }

    CfdbDatabase();
    CfdbDatabase(const std::string &filename, bool readOnly);
    virtual ~CfdbDatabase();

    const char *databaseType() const {
        return "CFDB";
    }

    bool supportsEditing() const {
        return true;
    }
    
    bool supportsOpeningTree() const {
        return true;
    }

    bool needsIndexing() const {
        return false;
    }

    bool supportsSearching() const {
        return true;
    }

    bool open(const std::string &filename, bool readOnly);
    bool close();
    bool readHeader(unsigned gameNum, GameHeader &gameHeader);
    bool read(unsigned gameNum, Game &game);
    bool write(unsigned gameNum, const Game &game);

    bool buildOpeningTree(unsigned gameNum, unsigned depth, DATABASE_CALLBACK_FUNC callback, void *contextInfo);
    bool searchOpeningTree(uint64_t hashKey, bool lastMoveOnly, std::vector<OpeningTreeEntry> &entries);
    bool countInOpeningTree(uint64_t hashKey, unsigned &count);
    bool countLongestLine(unsigned &count);

    bool search(const DatabaseSearchCriteria &searchCriteria, const DatabaseSortCriteria &sortCriteria,
                DATABASE_CALLBACK_FUNC callback, void *contextInfo, int offset = 0, int limit = 0);

    unsigned numGames();
    unsigned firstGameNum();
    unsigned lastGameNum();
    bool gameExists(unsigned gameNum);

    inline const std::string &filename() const {
        return m_filename;
    }

protected:
    bool createSchema();
    bool checkSchema();
    bool decodeMoves(Game &game, const Blob &moves, const Blob &annotations);
    bool encodeMoves(const Game &game, Blob &moves, Blob &annotations);
    bool encodeMoves(const AnnotMove *amove, Bitstream &moveBitstream, Blob &annotations, bool isVariation);
    unsigned selectPlayer(const Player &player);
    bool selectPlayer(unsigned id, Player &player);
    unsigned insertPlayer(const Player &player);
    bool updatePlayer(unsigned id, const Player &player);
    unsigned selectEvent(const std::string &name);
    bool selectEvent(unsigned id, std::string &name);
    unsigned insertEvent(const std::string &name);
    bool updateEvent(unsigned id, const std::string &name);
    unsigned selectSite(const std::string &name);
    bool selectSite(unsigned id, std::string &name);
    unsigned insertSite(const std::string &name);
    bool updateSite(unsigned id, const std::string &name);
    unsigned selectAnnotator(const std::string &name);
    bool selectAnnotator(unsigned id, std::string &name);
    unsigned insertAnnotator(const std::string &name);
    bool updateAnnotator(unsigned id, const std::string &name);
    const char *getAnnot(const char *pannot, const char *annotations, unsigned annotationsLen);
    void setDbErrorMsg(const char *fmt, ...);
    static std::string comparisonString(DatabaseComparison comparison);
    static std::string orderString(DatabaseOrder order);
};
} // namespace ChessCore
