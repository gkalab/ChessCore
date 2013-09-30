//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Database.cpp: Database abstract base class implementation.
//

#include <ChessCore/Database.h>
#include <ChessCore/Log.h>
#include <stdio.h>

using namespace std;

namespace ChessCore {
// ==========================================================================
// class Database
// ==========================================================================

const char *Database::m_classname = "Database";

// Resolve initialisation order issues
set<DATABASE_FACTORY_FUNC> &Database::_factories() {
    static set<DATABASE_FACTORY_FUNC> factories;
    return factories;
}

bool Database::registerFactory(DATABASE_FACTORY_FUNC factory) {
    _factories().insert(factory);
    return true;
}

Database::Database() :
    m_isOpen(false),
    m_access(ACCESS_NONE),
    m_errorMsg() {
}

Database::Database(const string &filename, bool readOnly) :
    m_isOpen(false),
    m_access(ACCESS_NONE),
    m_errorMsg() {
}

Database::~Database() {
}

bool Database::buildOpeningTree(unsigned gameNum, unsigned depth, DATABASE_CALLBACK_FUNC callback, void *contextInfo) {
    DBERROR << "Opening tree is not supported";
    return false;
}

bool Database::searchOpeningTree(uint64_t hashKey, bool lastMoveOnly, vector<OpeningTreeEntry> &entries) {
    DBERROR << "Opening tree is not supported";
    return false;
}

bool Database::countInOpeningTree(uint64_t hashKey, unsigned &count) {
    DBERROR << "Opening tree is not supported";
    return false;
}

bool Database::countLongestLine(unsigned &count) {
    DBERROR << "Opening tree is not supported";
    return false;
}

bool Database::hasValidIndex() {
    DBERROR << "Indexing not supported";
    return false;
}

bool Database::index(DATABASE_CALLBACK_FUNC callback, void *contextInfo) {
    DBERROR << "Indexing not supported";
    return false;
}

bool Database::search(const DatabaseSearchCriteria &searchCriteria, const DatabaseSortCriteria &sortCriteria,
                      DATABASE_CALLBACK_FUNC callback, void *contextInfo, int offset /*=0*/, int limit /*=0*/) {
    DBERROR << "Searching not supported";
    return false;
}

void Database::setErrorMsg(const char *message, ...) {
    char buffer[1024];
    va_list va;

    va_start(va, message);
    vsprintf(buffer, message, va);
    va_end(va);

    m_errorMsg = buffer;
}

bool Database::canOpenDatabase(const std::string &dbname) {
    shared_ptr<Database> instance = openDatabase(dbname, true);
    return instance.get() != 0;
}

shared_ptr<Database> Database::openDatabase(const string &dbname, bool readOnly) {
    shared_ptr<Database> instance;
    set<DATABASE_FACTORY_FUNC> &factories = _factories();
    for (auto it = factories.cbegin(); it != factories.cend() && !instance; ++it) {
        DATABASE_FACTORY_FUNC factory = *it;
        instance = (factory)(dbname, readOnly);
    }
    return instance;
}

// ==========================================================================
// class DatabaseErrorString
// ==========================================================================

DatabaseErrorString::DatabaseErrorString(Database &database):m_stream(), m_database(database) {
}

DatabaseErrorString::~DatabaseErrorString() {
    m_database.m_errorMsg = m_stream.str();
}

std::ostringstream &DatabaseErrorString::get() {
    return m_stream;
}
}   // namespace ChessCore
