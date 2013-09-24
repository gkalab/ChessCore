//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Database.h: Database abstract base class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Game.h>
#include <memory>
#include <vector>
#include <set>
#include <sstream>

namespace ChessCore {

class Database;
class OpeningTreeEntry;
class DatabaseErrorString;

extern "C"
{
/**
 * Callback function for long operations.
 *
 * @param gameNum The number of the game being processed (1-based).
 * @param percentComplete Processing progress. (0.0 to 100.0).
 * @param contextInfo Context Info passed to the database method.
 *
 * @return false to terminate processing, else true.
 */
typedef bool (*DATABASE_CALLBACK_FUNC)(unsigned gameNum, float percentComplete, void *contextInfo);
}

//
// Sort and search data types.  Note that not every field can be used in
// searching and sorting for some implementations.
//
typedef std::vector<unsigned>   DatabaseGameList;

typedef enum {
    DATABASE_FIELD_NONE,
    DATABASE_FIELD_GAME_NUM,        // Sorting only
    DATABASE_FIELD_WHITEPLAYER,
    DATABASE_FIELD_BLACKPLAYER,
    DATABASE_FIELD_PLAYER,          // White *or* black (searching only)
    DATABASE_FIELD_EVENT,
    DATABASE_FIELD_SITE,
    DATABASE_FIELD_ROUND,           // Sorting only
    DATABASE_FIELD_DATE,
    DATABASE_FIELD_ECO,
    DATABASE_FIELD_RESULT,          // Sorting only
} DatabaseField;

typedef enum {
    DATABASE_ORDER_NONE,
    DATABASE_ORDER_ASCENDING,
    DATABASE_ORDER_DESCENDING
} DatabaseOrder;

typedef enum {
    DATABASE_COMPARE_NONE,
    DATABASE_COMPARE_EQUALS,
    DATABASE_COMPARE_STARTSWITH,
    DATABASE_COMPARE_CONTAINS,
    DATABASE_COMPARE_FLAG_MASK          = 0xff00,
    DATABASE_COMPARE_CASE_SENSITIVE     = 0x0000,
    DATABASE_COMPARE_CASE_INSENSITIVE   = 0x8000
} DatabaseComparison;

// These methods do the dirty work of combining database comparison values with
// flags, including the necessary casting to keep the compiler happy...
inline DatabaseComparison databaseComparison(DatabaseComparison comparison, DatabaseComparison flags) {
    return (DatabaseComparison)((unsigned)comparison | (unsigned)flags);
}

inline DatabaseComparison databaseComparisonNoFlags(DatabaseComparison comparison) {
    return (DatabaseComparison)((unsigned)comparison & ~(unsigned)DATABASE_COMPARE_FLAG_MASK);
}

inline bool databaseComparisonCaseInsensitive(DatabaseComparison comparison) {
    return ((unsigned)comparison & (unsigned)DATABASE_COMPARE_CASE_INSENSITIVE) != 0;
}

//
// How to describe search criteria
//

struct DatabaseSortDescriptor {
    DatabaseField field;
    DatabaseOrder order;
};

typedef std::vector<DatabaseSortDescriptor> DatabaseSortCriteria;

struct DatabaseSearchDescriptor {
    DatabaseField field;
    DatabaseComparison comparison;
    std::string value;
};

typedef std::vector<DatabaseSearchDescriptor> DatabaseSearchCriteria;

extern "C"{
/**
 * Database subclass factory method.  This should be registered with
 * the Database base class during initialisation.
 *
 * @param dburl The database URL.  File-based databases should be prefixed with the
 * URL scheme file:/, however this shouldn't be mandatory.
 * @param readOnly If true then open the database in read-only mode.
 *
 * @return The Database subclass for the specified database URL, or 0 if a subclass implementation
 * could not be found.
 */
typedef std::shared_ptr<Database> (*DATABASE_FACTORY_FUNC)(const std::string &dburl, bool readOnly);
}   // extern "C"

class CHESSCORE_EXPORT Database {
public:
    // Database access modes
    enum Access {
        ACCESS_NONE,    // Cannot read or write
        ACCESS_READONLY, // Can only be read
        ACCESS_READWRITE // Can be read and written
    };

private:
    friend class DatabaseErrorString;
    static const char *m_classname;
    static std::set<DATABASE_FACTORY_FUNC> &_factories();
protected:
    bool m_isOpen;
    Access m_access;
    std::string m_errorMsg;

public:
    /**
     * Register a subclass factory method.  Subclass factory methods are asked in turn if
     * they can open the specified database, until one succeeds or no more factories exist
     *
     * This can be implemented with the subclass using the following code:
     *
     * static Database *databaseFactory(const string &dburl) {
     *     if (Util::endsWith(dburl, ".xyz", false)) {
     *         return new MyDatabase(dburl);
     *     }
     *     return 0;
     * }
     *
     * static bool registered = Database::registerFactory(databaseFactory);
     *
     * @param factory The database factory method.
     *
     * @return true if the factory was successfully registered, or false if an error occurred.
     */
    static bool registerFactory(DATABASE_FACTORY_FUNC factory);

    Database();
    Database(const std::string &filename, bool readOnly);
    virtual ~Database();

    /**
     * @return The type of the database.
     */
    virtual const char *databaseType() const {
        return NULL;
    }

    /**
     * @return true if the database supports an opening tree, else false.
     */
    virtual bool supportsOpeningTree() const {
        return false;
    }

    /**
     * @return true if the database needs indexing in order to support random-access I/O.
     */
    virtual bool needsIndexing() const {
        return false;
    }

    /**
     * @return true if the database supports searching.
     */
    virtual bool supportsSearching() const {
        return false;
    }

    /**
     * Open or create the database.
     *
     * @param filename The database filename.
     * @param readOnly If true then open the database read-only, else
     * attempt to open the database read-write.  The actual access
     * obtained is accessible using the access() method once open()
     * succeedes
     *
     * @return true if the database opened successfully, else false.
     */
    virtual bool open(const std::string &filename, bool readOnly) = 0;

    /**
     * Close the database.
     *
     * @return true if the database closed successfully, else false.
     */
    virtual bool close() = 0;

    /**
     * Read the specified game header from the database.
     *
     * @param gameNum The game header number to read, starting at 1.
     * @param gameHeader The GameHeader instance in which to read the game header.
     *
     * @return true if the game header was read successfully, else false.
     */
    virtual bool readHeader(unsigned gameNum, GameHeader &gameHeader) = 0;

    /**
     * Read the specified game from the database.
     *
     * @param gameNum The game number to read, starting at 1.
     * @param game The Game instance in which to read the game.
     *
     * @return true if the game was read successfully, else false.
     */
    virtual bool read(unsigned gameNum, Game &game) = 0;

    /**
     * Write the specified game to the database.
     *
     * @param gameNum The game number to read, starting at 1.  This value is
     * implementation-specific; some allow this to be 0 and others will only
     * allow this value to equal numGames() + 1.
     * @param game The game instance to write.
     *
     * @return true if the game was written successfully, else false.
     */
    virtual bool write(unsigned gameNum, const Game &game) = 0;

    /**
     * Rebuild the opening tree for one or more games.
     *
     * Only available if supportsOpeningTree() returns true.
     *
     * @param gameNum The game to rebuild the opening tree for.  If this is
     * 0 then the opening tree is rebuilt for all games in the database.
     * @param depth The number of half-moves to include in the opening tree.
     * @param callback An optional callback function, used to provide feedback
     * of the indexing process.
     * @param contextInfo Context Info to pass to the callback function.
     *
     * @return true if the opening tree was built successfully, else false.
     */
    virtual bool buildOpeningTree(unsigned gameNum, unsigned depth, DATABASE_CALLBACK_FUNC callback, void *contextInfo);

    /**
     * Search the opening tree for games containing the specified position.
     *
     * Only available if supportsOpeningTree() returns true.
     *
     * @param hashKey The hash key of the position to search for.
     * @param lastMoveOnly If true, fetch entries where last_move is true, else fetch
     *                all entries matching the position.
     * @param entries Where to store the OpeningTreeEntry objects that match the
     *        position specified.
     *
     * @return true if the game list was successfully populated, else false.
     */
    virtual bool searchOpeningTree(uint64_t hashKey, bool lastMoveOnly, std::vector<OpeningTreeEntry> &entries);

    /**
     * Count the number of opening tree entries with the specified key.
     *
     * Only available if supportsOpeningTree() returns true.
     *
     * @param hashKey The hash key of the position.
     * @param count Where to store the count.
     *
     * @return true if the count was successful, else false.
     */
    virtual bool countInOpeningTree(uint64_t hashKey, unsigned &count);

    /**
     * Count the longest line in the opening tree.
     *
     * Only available if supportsOpeningTree() returns true.
     *
     * @param count Where to store the count.
     *
     * @return true if the count was successful, else false.
     */
    virtual bool countLongestLine(unsigned &count);

    /**
     * Test if the database already has a valid index file.  This has the
     * side effect of opening the index file if it's successfully.
     *
     * Only available if needsIndexing() returns true.
     *
     * @return true if the database has a valid index file.
     */
    virtual bool hasValidIndex();

    /**
     * Index the database.
     *
     * Only available if needsIndexing() returns true.
     *
     * @param callback An optional callback function, used to provide feedback
     * of the indexing process.
     * @param contextInfo Context Info to pass to the callback function.
     *
     * @return true if indexing was successful, else false.
     */
    virtual bool index(DATABASE_CALLBACK_FUNC callback, void *contextInfo);

    /**
     * Search the database, returning results in the specified order.
     *
     * Only available if supportsSearching() returns true.
     *
     * @param searchCriteria The fields to search.  This can be empty if all
     * that's required is the games in ascending order.
     * @param sortCriteria The order in which results should be returned.  If
     * this is empty then { DATABASE_FIELD_GAME_NUM, DATABASE_ORDER_ASCENDING }
     * is used.
     * @param callback A mandatory callback function, used to provide a game number
     * found during searching.  The percentComplete parameter will always be called
     * with the value 0.0f.
     * @param contextInfo Context Info to pass to the callback function.
     * @param offset Optional offset (1-based). If this is non-zero then the
     * results are returned from the given offset.
     * @param limit Optional limit.  If this is non-zero then the result size
     * is limited to this value.
     *
     * @return true If the search was successful, else false.
     */
    virtual bool search(const DatabaseSearchCriteria &searchCriteria, const DatabaseSortCriteria &sortCriteria,
                        DATABASE_CALLBACK_FUNC callback, void *contextInfo, int offset = 0, int limit = 0);

    /**
     * @return The number of games in the database.
     */
    virtual unsigned numGames() = 0;

    /**
     * @return The number of the first game in the database.
     */
    virtual unsigned firstGameNum() = 0;

    /**
     * @return The number of the last game in the database.
     */
    virtual unsigned lastGameNum() = 0;

    /**
     * @return true if the game exists in the database.
     */
    virtual bool gameExists(unsigned gameNum) = 0;

    /**
     * @return The name of the primary database file.
     */
    virtual const std::string &filename() const = 0;

    /**
     * @return true if the database is open.
     */
    bool isOpen() const {
        return m_isOpen;
    }

    /**
     * @return The database access.
     */
    Access access() const {
        return m_access;
    }

    /**
     * @return The last error that occurred on the database.
     */
    const std::string &errorMsg() const {
        return m_errorMsg;
    }

protected:
    /**
     * Set the last error message.
     *
     * @param message A printf-like string for formatting the following arguments.
     */
    void setErrorMsg(const char *message, ...);

    /**
     * Clear the last error message.
     */
    void clearErrorMsg() {
        m_errorMsg.clear();
    }

public:
    /**
     * Determine if the specified database file can be opened.
     *
     * @param dburl The database URL.  File-based databases should be prefixed with the
     * URL scheme file:/, however this shouldn't be mandatory.
     *
     * @return true if the database file can be opened, else false.
     */
    static bool canOpenDatabase(const std::string &dburl);

    /**
     * Allocate and return a Database subclass implementation that can open or create the
     * specified database.
     *
     * @param dburl The database URL.  File-based databases should be prefixed with the 
     * URL scheme file:/, however this shouldn't be mandatory.
     * @param readOnly If true then open the database in read-only mode.
     *
     * @return The Database subclass for the specified database URL, or 0 if a subclass implementation
     * could not be found.
     */
    static std::shared_ptr<Database> openDatabase(const std::string &dburl, bool readOnly);
};

//
// Set the Database::m_errorMsg via a streams interface
//
class DatabaseErrorString {
protected:
    std::ostringstream m_stream;
    Database &m_database;

public:
    DatabaseErrorString(Database &database);
    virtual ~DatabaseErrorString();

    std::ostringstream &get();

    // No copy
private:
    DatabaseErrorString(const DatabaseErrorString &);
    DatabaseErrorString &operator=(const DatabaseErrorString &);
};

// Can only be used from Database subclasses
#define DBERROR DatabaseErrorString(*this).get()

}   // namespace ChessCore
