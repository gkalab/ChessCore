//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// SqliteStatement.h: sqlite statement abstraction class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Blob.h>
#include <sqlite3.h>

namespace ChessCore {
class CHESSCORE_EXPORT SqliteStatement {
private:
    static const char *m_classname;

protected:
    sqlite3 *m_db;
    sqlite3_stmt *m_stmt;

public:
    SqliteStatement(sqlite3 *db);
    SqliteStatement(sqlite3 *db, const std::string &sql);
    ~SqliteStatement();

    bool beginTransaction();
    bool commit();
    bool rollback();
    bool setSynchronous(bool synchronous);
    bool setJournalMode(const std::string &mode);
    bool prepare(const std::string &sql);
    void clearBindings();
    void reset();
    void finalize();
    bool bind(unsigned index, const Blob &blob);
    bool bind(unsigned index, double d);
    bool bind(unsigned index, int i);
    bool bind(unsigned index, int64_t i64);
    bool bind(unsigned index, uint64_t ui64) {
        return bind(index, (int64_t)ui64);
    }

    bool bind(unsigned index, bool b) {
        return bind(index, (int)b);
    }

    bool bind(unsigned index);          // bind null
    bool bind(unsigned index, const char *s);
    bool bind(unsigned index, const std::string &s);
    int step();
    unsigned columnCount();
    bool columnBlob(unsigned index, Blob &blob);
    double columnDouble(unsigned index);
    int columnInt(unsigned index);
    int64_t columnInt64(unsigned index);
    uint64_t columnUInt64(unsigned index) {
        return (uint64_t)columnInt64(index);
    }

    bool columnBool(unsigned index) {
        return columnInt(index) != 0;
    }

    const char *columnText(unsigned index, unsigned &length);
    bool columnString(unsigned index, std::string &s);
    std::string columnString(unsigned index);
    unsigned lastInsertId();
};
}   // namespace ChessCore
