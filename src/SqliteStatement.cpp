//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// SqliteStatement.cpp: sqlite abstract class implementation.
//

#include <ChessCore/SqliteStatement.h>
#include <ChessCore/CfdbDatabase.h>
#include <ChessCore/Util.h>
#include <string.h>

using namespace std;

namespace ChessCore {
#define DEBUG_STATEMENTS 0

const char *SqliteStatement::m_classname = "SqliteStatement";

#if DEBUG_STATEMENTS
#define RESULT_CODE(x) {x, #x}
static struct {
    int resultCode;
    const char *desc;
} resultCodes[] = {
    RESULT_CODE(SQLITE_OK),
    RESULT_CODE(SQLITE_ERROR),
    RESULT_CODE(SQLITE_INTERNAL),
    RESULT_CODE(SQLITE_PERM),
    RESULT_CODE(SQLITE_ABORT),
    RESULT_CODE(SQLITE_BUSY),
    RESULT_CODE(SQLITE_LOCKED),
    RESULT_CODE(SQLITE_NOMEM),
    RESULT_CODE(SQLITE_READONLY),
    RESULT_CODE(SQLITE_INTERRUPT),
    RESULT_CODE(SQLITE_IOERR),
    RESULT_CODE(SQLITE_CORRUPT),
    RESULT_CODE(SQLITE_NOTFOUND),
    RESULT_CODE(SQLITE_FULL),
    RESULT_CODE(SQLITE_CANTOPEN),
    RESULT_CODE(SQLITE_PROTOCOL),
    RESULT_CODE(SQLITE_EMPTY),
    RESULT_CODE(SQLITE_SCHEMA),
    RESULT_CODE(SQLITE_TOOBIG),
    RESULT_CODE(SQLITE_CONSTRAINT),
    RESULT_CODE(SQLITE_MISMATCH),
    RESULT_CODE(SQLITE_MISUSE),
    RESULT_CODE(SQLITE_NOLFS),
    RESULT_CODE(SQLITE_AUTH),
    RESULT_CODE(SQLITE_FORMAT),
    RESULT_CODE(SQLITE_RANGE),
    RESULT_CODE(SQLITE_NOTADB),
    RESULT_CODE(SQLITE_ROW),
    RESULT_CODE(SQLITE_DONE)
};

const unsigned numResultCodes = sizeof(resultCodes) / sizeof(resultCodes[0]);

inline const char *resultCode(int resultCode) {
    for (unsigned i = 0; i < numResultCodes; i++)
        if (resultCodes[i].resultCode == resultCode)
            return resultCodes[i].desc;

    return "Unknown";
}

#define CALLFUNC(x) \
    do {rv = (x); logdbg(#x " returned %s", resultCode(rv)); } while (false)
#define CALLPREPARE(x) \
    do {rv = (x); logdbg("'%s' returned %s", sql.c_str(), resultCode(rv)); } while (false)
#define CALLBIND(x) \
    do {rv = (x); logdbg("bind index %u returned %s", index, resultCode(rv)); } while (false)
#define CALLBINDCP(x) \
    do {rv = (x); logdbg("bind '%s' to index %u returned %s", s, index, resultCode(rv)); } while (false)
#define CALLBINDSTR(x) \
    do {rv = (x); logdbg("bind '%s' to index %u returned %s", s.c_str(), index, resultCode(rv)); } while (false)

#else // !DEBUG_STATEMENTS
#define CALLFUNC(x)    rv = (x)
#define CALLPREPARE(x) rv = (x)
#define CALLBIND(x)    rv = (x)
#define CALLBINDCP(x)  rv = (x)
#define CALLBINDSTR(x) rv = (x)
#endif // DEBUG_STATEMENTS

SqliteStatement::SqliteStatement(sqlite3 *db) :
    m_db(db),
    m_stmt(0)
{
}

SqliteStatement::SqliteStatement(sqlite3 *db, const string &sql) :
    m_db(db),
    m_stmt(0)
{
    prepare(sql);
}

SqliteStatement::~SqliteStatement() {
    finalize();
    m_db = 0;
}

bool SqliteStatement::beginTransaction() {
    return prepare("BEGIN TRANSACTION") && step() == SQLITE_DONE;
}

bool SqliteStatement::commit() {
    return prepare("COMMIT") && step() == SQLITE_DONE;
}

bool SqliteStatement::rollback() {
    return prepare("ROLLBACK") && step() == SQLITE_DONE;
}

bool SqliteStatement::setSynchronous(bool synchronous) {
    string sql = Util::format("PRAGMA synchronous = %s", synchronous ? "ON" : "OFF");

    return prepare(sql) && step() == SQLITE_DONE;
}

bool SqliteStatement::setJournalMode(const string &mode) {
    string sql = Util::format("PRAGMA journal_mode = %s", mode.c_str());

    return prepare(sql) && step() == SQLITE_ROW;
}

bool SqliteStatement::prepare(const string &sql) {
    finalize();

    int rv;
    CALLPREPARE(sqlite3_prepare_v2(m_db, sql.c_str(), (int)sql.length(), &m_stmt, 0));
    return rv == SQLITE_OK;
}

void SqliteStatement::clearBindings() {
    int rv;

    CALLFUNC(sqlite3_clear_bindings(m_stmt));
}

void SqliteStatement::reset() {
    int rv;

    CALLFUNC(sqlite3_reset(m_stmt));
}

void SqliteStatement::finalize() {
    if (m_stmt) {
        int rv;
        CALLFUNC(sqlite3_finalize(m_stmt));
        m_stmt = 0;
    }
}

bool SqliteStatement::bind(unsigned index, const Blob &blob) {
    int rv;

    if (blob.data() && blob.length())
        CALLBIND(sqlite3_bind_blob(m_stmt, (int)index, blob.data(), (int)blob.length(), SQLITE_STATIC));
    else
        CALLBIND(sqlite3_bind_null(m_stmt, (int)index));

    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index, double d) {
    int rv;

    CALLBIND(sqlite3_bind_double(m_stmt, (int)index, d));
    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index, int i) {
    int rv;

    CALLBIND(sqlite3_bind_int(m_stmt, (int)index, i));
    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index, int64_t i64) {
    int rv;

    CALLBIND(sqlite3_bind_int64(m_stmt, (int)index, i64));
    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index) {
    int rv;

    CALLBIND(sqlite3_bind_null(m_stmt, (int)index));
    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index, const char *s) {
    int rv;

    if (s && *s)
        CALLBINDCP(sqlite3_bind_text(m_stmt, (int)index, s, (int)strlen(s), SQLITE_STATIC));
    else
        CALLBIND(sqlite3_bind_null(m_stmt, (int)index));

    return rv == SQLITE_OK;
}

bool SqliteStatement::bind(unsigned index, const string &s) {
    int rv;

    if (!s.empty())
        CALLBINDSTR(sqlite3_bind_text(m_stmt, (int)index, s.c_str(), (int)s.length(), SQLITE_STATIC));
    else
        CALLBIND(sqlite3_bind_null(m_stmt, (int)index));

    return rv == SQLITE_OK;
}

int SqliteStatement::step() {
    int rv;

    CALLFUNC(sqlite3_step(m_stmt));

    if (rv != SQLITE_ROW && CfdbDatabase::sqliteVersion() <= 3006023)
        sqlite3_reset(m_stmt);  // Done automatically in v3.6.23+

    return rv;
}

unsigned SqliteStatement::columnCount() {
    return (unsigned)sqlite3_column_count(m_stmt);
}

bool SqliteStatement::columnBlob(unsigned index, Blob &blob) {
    blob.free();
    return blob.set((const uint8_t *)sqlite3_column_blob(m_stmt, (int)index),
                    (unsigned)sqlite3_column_bytes(m_stmt, (int)index));
}

double SqliteStatement::columnDouble(unsigned index) {
    return sqlite3_column_double(m_stmt, (int)index);
}

int SqliteStatement::columnInt(unsigned index) {
    return sqlite3_column_int(m_stmt, (int)index);
}

int64_t SqliteStatement::columnInt64(unsigned index) {
    return sqlite3_column_int64(m_stmt, (int)index);
}

const char *SqliteStatement::columnText(unsigned index, unsigned &length) {
    length = (unsigned)sqlite3_column_bytes(m_stmt, (int)index);
    return (const char *)sqlite3_column_text(m_stmt, (int)index);
}

bool SqliteStatement::columnString(unsigned index, string &s) {
    s.clear();
    unsigned length = 0;
    const char *text = columnText(index, length);

    if (text == 0)
        return false;

    s.assign(text, length);
    return true;
}

string SqliteStatement::columnString(unsigned index) {
    string str;

    columnString(index, str);
    return str;
}

unsigned SqliteStatement::lastInsertId() {
    return (unsigned)sqlite3_last_insert_rowid(m_db);
}
}   // namespace ChessCore
