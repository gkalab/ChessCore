//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// CfdbDatabase.cpp: ChessFront Database class implementation.
//

#define VERBOSE_LOGGING 0
#define DEBUG_BLOBS     0       // Debug blobs

#include <ChessCore/CfdbDatabase.h>
#include <ChessCore/SqliteStatement.h>
#include <ChessCore/OpeningTree.h>
#include <ChessCore/Log.h>
#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <set>

using namespace std;

namespace ChessCore {
// Constants used by encoded moves

// Database Factory
static shared_ptr<Database> databaseFactory(const string &dburl, bool readOnly) {
    shared_ptr<Database> db;
    if (Util::endsWith(dburl, ".cfdb", false)) {
        db.reset(new CfdbDatabase(dburl, readOnly));
    }
    return db;
}

static bool registered = Database::registerFactory(databaseFactory);

// The first bit shows the type of the encoded move:
const unsigned ENCMOVE_TYPE_BITSIZE =       2;
const uint32_t ENCMOVE_TYPE_MOVE =          0x0;
const uint32_t ENCMOVE_TYPE_ANNOTMOVE =     0x1;
const uint32_t ENCMOVE_TYPE_VARSTART =      0x2;
const uint32_t ENCMOVE_TYPE_VAREND =        0x3;

// ENCMOVE_MOVE
const unsigned ENCMOVE_MOVE_BITSIZE =       8;
const uint32_t ENCMOVE_MOVE_INDEX_MASK =    0x00ff;

// ENCMOVE_ANNOTMOVE (ENCMOVE_MOVE plus bits)
const unsigned ENCMOVE_ANNOTMOVE_BITSIZE =  11;
const uint32_t ENCMOVE_PRE_ANNOT_BIT =      0x0100;
const uint32_t ENCMOVE_POST_ANNOT_BIT =     0x0200;
const uint32_t ENCMOVE_NAGS_BIT =           0x0400;

const char *CfdbDatabase::m_classname = "CfdbDatabase";
unsigned CfdbDatabase::m_sqliteVersion = 0;

CfdbDatabase::CfdbDatabase() :
    Database(),
    m_filename(),
    m_db(0) {
    if (m_sqliteVersion == 0) {
        m_sqliteVersion = (unsigned)sqlite3_libversion_number();
        LOGINF << "sqlite version: " << m_sqliteVersion;

        if (sqlite3_threadsafe() == 0) LOGERR << "sqlite3 database is not thread-safe";
    }
}

CfdbDatabase::CfdbDatabase(const string &filename, bool readOnly) :
    Database(),
    m_filename(),
    m_db(0) {
    if (m_sqliteVersion == 0) {
        m_sqliteVersion = (unsigned)sqlite3_libversion_number();
        LOGINF << "sqlite version: " << m_sqliteVersion;
    }

    open(filename, readOnly);
}

CfdbDatabase::~CfdbDatabase() {
    close();
}

bool CfdbDatabase::open(const string &filename, bool readOnly) {
    clearErrorMsg();

    if (m_isOpen) close();

    bool exists = Util::fileExists(filename);

    if (!exists && readOnly) {
        DBERROR << "Database file does not exist";
        return false;
    }

    unsigned flags = SQLITE_OPEN_FULLMUTEX;

    if (readOnly) flags |= SQLITE_OPEN_READONLY;
    else flags |= SQLITE_OPEN_READWRITE;

    if (!exists) flags |= SQLITE_OPEN_CREATE;

    if (sqlite3_open_v2(filename.c_str(), &m_db, flags, 0) == SQLITE_OK) m_isOpen = true;
    else DBERROR << "Failed to create ChessFront database '" << filename << "'";

    if (m_isOpen) {
        SqliteStatement stmt(m_db);

        if (!stmt.setSynchronous(false)) LOGWRN << "Failed to turn off synchronous mode: " << sqlite3_errmsg(m_db);

        if (!stmt.setJournalMode("MEMORY")) LOGWRN << "Failed to set journal_mode to MEMORY: " << sqlite3_errmsg(m_db);

        if (!exists) {
            // Create the schema
            if (!createSchema()) close();
        } else if (!checkSchema()) close();

        if (m_isOpen) {
            m_filename = filename;
            m_access = readOnly ? ACCESS_READONLY : ACCESS_READWRITE;
        }
    }

    return m_isOpen;
}

bool CfdbDatabase::close() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = 0;
    }

    m_filename.clear();
    m_isOpen = false;
    m_access = ACCESS_NONE;

    return true;
}

bool CfdbDatabase::readHeader(unsigned gameNum, GameHeader &gameHeader) {
    //LOGDBG << "gameNum=" << gameNum;

    clearErrorMsg();

    gameHeader.setReadFail(true);

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    // Note: no gameHeader.initHeader() as the caller will have already done that!

    SqliteStatement stmt(m_db);
    bool retval = false;
    int rv;

    if (stmt.prepare("SELECT white_player_id, black_player_id, event_id, site_id, "
                     "date, round_major, round_minor, result, annotator_id, eco, white_elo, black_elo, time_control "
                     "FROM game WHERE game_id = ?") &&
        stmt.bind(1, (int)gameNum)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            Player player;
            string eco, name;
            Blob timeControl;

            unsigned whitePlayerId = stmt.columnInt(0);
            unsigned blackPlayerId = stmt.columnInt(1);
            unsigned eventId = stmt.columnInt(2);
            unsigned siteId = stmt.columnInt(3);
            unsigned date = stmt.columnInt(4);
            unsigned roundMajor = stmt.columnInt(5);
            unsigned roundMinor = stmt.columnInt(6);
            unsigned result = stmt.columnInt(7);
            unsigned annotatorId = stmt.columnInt(8);
            stmt.columnString(9, eco);
            unsigned whiteElo = stmt.columnInt(10);
            unsigned blackElo = stmt.columnInt(11);
            stmt.columnBlob(12, timeControl);

            if (whitePlayerId &&
                selectPlayer(whitePlayerId, player)) {
                player.setElo(whiteElo);
                gameHeader.setWhite(player);
            }

            if (blackPlayerId &&
                selectPlayer(blackPlayerId, player)) {
                player.setElo(blackElo);
                gameHeader.setBlack(player);
            }

            if (eventId) {
                selectEvent(eventId, name);
                if (name.length() > 0)
                    gameHeader.setEvent(name);
            }

            if (siteId) {
                selectSite(siteId, name);
                if (name.length() > 0)
                    gameHeader.setSite(name);
            }

            gameHeader.setDay(date % 100);
            gameHeader.setMonth((date / 100) % 100);
            gameHeader.setYear(date / 10000);

            gameHeader.setRoundMajor(roundMajor);
            gameHeader.setRoundMinor(roundMinor);

            gameHeader.setResult((GameHeader::Result)result);

            if (annotatorId) {
                selectAnnotator(annotatorId, name);
                if (name.length() > 0)
                    gameHeader.setAnnotator(name);
            }

            if (eco.length() > 0)
                gameHeader.setEco(eco);

            gameHeader.timeControl().setFromBlob(timeControl);

            retval = true;
            //LOGDBG << "Read game " << gameNum << " header";
        } else if (rv == SQLITE_DONE) {
            LOGDBG << "Game " << gameNum << " does not exist";
        } else {
            setDbErrorMsg("Failed to select game %u", gameNum);
        }
    } else {
        setDbErrorMsg("Failed to prepare game select statement");
    }

    gameHeader.setReadFail(!retval);
    return retval;
}

bool CfdbDatabase::read(unsigned gameNum, Game &game) {
    //LOGDBG << "gameNum=" << gameNum;

    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    game.init();

    if (!readHeader(gameNum, game)) return false;

    game.setReadFail(true);

    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT partial, moves, annotations FROM game WHERE game_id = ?") &&
        stmt.bind(1, (int)gameNum)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            Blob partial, moves, annotations;
            stmt.columnBlob(0, partial);
            stmt.columnBlob(1, moves);
            stmt.columnBlob(2, annotations);

#if DEBUG_BLOBS
            LOGDBG << "partial " << partial;
            LOGDBG << "moves " << moves;
            LOGDBG << "annotations " << annotations;
#endif // DEBUG_BLOBS

            if (partial.length() > 0) {
                Position pos;

                if (pos.setFromBlob(partial) == Position::LEGAL) game.setStartPosition(pos);
                else {
                    LOGERR << "Invalid starting position in binary object";
                    return false;
                }
            }

            game.setPositionToStart();

            if (moves.length() > 0) {
                if (!decodeMoves(game, moves, annotations)) return false;
            } else LOGINF << "game " << gameNum << " has no moves!";
        } else if (rv == SQLITE_DONE) {
            DBERROR << "Game " << gameNum << " does not exist";
            return false;
        } else {
            DBERROR << "Failed to select game " << gameNum;
            return false;
        }
    } else {
        DBERROR << "Failed to prepare game select statement";
        return false;
    }

    game.setReadFail(false);
    return true;
}

bool CfdbDatabase::write(unsigned gameNum, const Game &game) {
    //LOGDBG << "gameNum=" << gameNum;

    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access != ACCESS_READWRITE) {
        DBERROR << "Cannot write to this database";
        return false;
    }

    bool inserting = gameNum == 0 || !gameExists(gameNum);
    Blob timeControl, partial, moves, annotations;

    if (game.timeControl().isValid())
        if (!game.timeControl().blob(timeControl))
            return false;

    if (game.isPartialGame())
        if (!game.startPosition().blob(partial))
            return false;

    if (!encodeMoves(game, moves, annotations))
        return false;

#if DEBUG_BLOBS
    LOGDBG << "timeControl " << timeControl;
    LOGDBG << "partial " << partial;
    LOGDBG << "moves " << moves;
    LOGDBG << "annotations " << annotations;
#endif // DEBUG_BLOBS

    SqliteStatement stmt(m_db);

    if (!stmt.beginTransaction()) {
        setDbErrorMsg("Failed to begin transaction");
        return false;
    }

    unsigned whitePlayerId = 0, blackPlayerId = 0, eventId = 0, siteId = 0, annotatorId = 0;

    if (game.white().hasName()) {
        whitePlayerId = selectPlayer(game.white());

        if (whitePlayerId == 0) {
            whitePlayerId = insertPlayer(game.white());

            if (whitePlayerId == 0) {
                stmt.rollback();
                return false;
            }
        }
    }

    if (game.black().hasName()) {
        blackPlayerId = selectPlayer(game.black());

        if (blackPlayerId == 0) {
            blackPlayerId = insertPlayer(game.black());

            if (blackPlayerId == 0) {
                stmt.rollback();
                return false;
            }
        }
    }

    if (game.hasEvent()) {
        eventId = selectEvent(game.event());

        if (eventId == 0) {
            eventId = insertEvent(game.event());

            if (eventId == 0) {
                stmt.rollback();
                return false;
            }
        }
    }

    if (game.hasSite()) {
        siteId = selectSite(game.site());

        if (siteId == 0) {
            siteId = insertSite(game.site());

            if (siteId == 0) {
                stmt.rollback();
                return false;
            }
        }
    }

    if (game.hasAnnotator()) {
        annotatorId = selectAnnotator(game.annotator());

        if (annotatorId == 0) {
            annotatorId = insertAnnotator(game.annotator());

            if (annotatorId == 0) {
                stmt.rollback();
                return false;
            }
        }
    }

    bool retval = false;
    int rv;

    if (inserting) {
        // Insert
        if (gameNum == 0) gameNum = lastGameNum() + 1;

        if (stmt.prepare(
                "INSERT INTO game (game_id, white_player_id, black_player_id, event_id, site_id, "
                "date, round_major, round_minor, result, annotator_id, eco, white_elo, black_elo, "
                "time_control, halfmoves, partial, moves, annotations) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)") &&
            stmt.bind(1, (int)gameNum) &&
            stmt.bind(2, (int)whitePlayerId) &&
            stmt.bind(3, (int)blackPlayerId) &&
            stmt.bind(4, (int)eventId) &&
            stmt.bind(5, (int)siteId) &&
            stmt.bind(6, (int)(game.year() * 10000 + game.month() * 100 + game.day())) &&
            stmt.bind(7, (int)game.roundMajor()) &&
            stmt.bind(8, (int)game.roundMinor()) &&
            stmt.bind(9, (int)game.result()) &&
            stmt.bind(10, (int)annotatorId) &&
            stmt.bind(11, game.eco()) &&
            stmt.bind(12, (int)game.white().elo()) &&
            stmt.bind(13, (int)game.black().elo()) &&
            stmt.bind(14, timeControl) &&
            stmt.bind(15, (int)game.countMainline()) &&
            stmt.bind(16, partial) &&
            stmt.bind(17, moves) &&
            stmt.bind(18, annotations)) {
            rv = stmt.step();

            if (rv == SQLITE_DONE) retval = true;
            //LOGDBG << "Inserted game " << gameNum;
            else setDbErrorMsg("Failed to insert game %u", gameNum);
        } else setDbErrorMsg("Failed to prepare game insert statement");
    } else { // !inserting
        if (stmt.prepare(
                "UPDATE game SET white_player_id = ?, black_player_id = ?, event_id = ?, "
                "site_id = ?, date = ?, round_major = ?, round_minor = ?, result = ?, annotator_id = ?, "
                "eco = ?, white_elo = ?, black_elo = ?, time_control = ?, halfmoves = ?, partial = ?, moves = ?, annotations = ? "
                "WHERE game_id = ?") &&
            stmt.bind(1, (int)whitePlayerId) &&
            stmt.bind(2, (int)blackPlayerId) &&
            stmt.bind(3, (int)eventId) &&
            stmt.bind(4, (int)siteId) &&
            stmt.bind(5, (int)(game.year() * 10000 + game.month() * 100 + game.day())) &&
            stmt.bind(6, (int)game.roundMajor()) &&
            stmt.bind(7, (int)game.roundMinor()) &&
            stmt.bind(8, (int)game.result()) &&
            stmt.bind(9, (int)annotatorId) &&
            stmt.bind(10, game.eco()) &&
            stmt.bind(11, (int)game.white().elo()) &&
            stmt.bind(12, (int)game.black().elo()) &&
            stmt.bind(13, timeControl) &&
            stmt.bind(14, (int)game.countMainline()) &&
            stmt.bind(15, partial) &&
            stmt.bind(16, moves) &&
            stmt.bind(17, annotations) &&
            stmt.bind(18, (int)gameNum)) {
            rv = stmt.step();

            if (rv == SQLITE_DONE) {
                retval = true;
                LOGDBG << "Updated game " << gameNum;
            } else setDbErrorMsg("Failed to insert game %u", gameNum);
        } else setDbErrorMsg("Failed to prepare game update statement");
    }

    if (retval) {
        stmt.commit();
        LOGVERBOSE << "Committed transaction";
    } else {
        stmt.rollback();
        LOGVERBOSE << "Rolled-back transaction";
    }

    return retval;
}

bool CfdbDatabase::buildOpeningTree(unsigned gameNum, unsigned depth, DATABASE_CALLBACK_FUNC callback,
                                    void *contextInfo) {
    //LOGDBG << "gameNum=" << gameNum << ", depth=" << depth;

    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access != ACCESS_READWRITE) {
        DBERROR << "Cannot write to this database";
        return false;
    }

    // Delete the existing entries
    int rv;
    SqliteStatement stmt(m_db);

    if (gameNum > 0) {
        if (stmt.prepare("DELETE FROM optree WHERE game_id = ?") &&
            stmt.bind(1, (int)gameNum)) {
            rv = stmt.step();

            if (rv != SQLITE_DONE) {
                setDbErrorMsg("Failed to delete optree entries for game %u", gameNum);
                return false;
            }
        } else {
            setDbErrorMsg("Failed to prepare optree delete statement for game %u", gameNum);
            return false;
        }
    } else {
        if (stmt.prepare("DELETE FROM optree")) {
            rv = stmt.step();

            if (rv != SQLITE_DONE) {
                setDbErrorMsg("Failed to delete optree entries");
                return false;
            }
        } else {
            setDbErrorMsg("Failed to prepare optree delete statement");
            return false;
        }
    }

    unsigned first, last, count;
    int score;
    Game game;
    Position pos, prevPos;
    UnmakeMoveInfo umi;
    OpeningTreeEntry entry;
    const AnnotMove *move;

    if (gameNum > 0) {
        first = gameNum;
        last = gameNum;
    } else {
        first = firstGameNum();
        last = lastGameNum();
    }

    LOGINF << "Building Opening Tree in database '" << filename() << "' for games " << first << " to "
           << last;

    for (unsigned i = first; i <= last; i++) {
        if (!gameExists(i)) {
            LOGDBG << "Ignoring game " << i << " as it does not exist";
            continue;
        }

        if (!read(i, game)) {
            LOGERR << "Failed to read game " << i;
            return false;
        }

        if (!game.startPosition().isStarting()) {
            LOGDBG << "Ignoring game " << i << " as it starts from a set position";
            continue;
        }

        if (game.mainline() == 0) {
            LOGDBG << "Ignoring game " << i << " as it has no moves";
            continue;
        }

        game.setPositionToStart();
        pos = game.position();

        switch (game.result()) {
        case GameHeader::WHITE_WIN:
            score = +1;
            break;

        case GameHeader::BLACK_WIN:
            score = -1;
            break;

        default:
            score = 0;
            break;
        }

        for (move = game.mainline(), count = 0;
             move && count < depth;
             move = move->next(), count++) {
            entry.init();
            prevPos = pos;

            if (!pos.makeMove(move, umi)) {
                DBERROR << "Error making move '" << move->dump(false) << "'";
                return false;
            }

            entry.setHashKey(pos.hashKey());
            entry.setMove(move->move());
            entry.setScore(score);
            entry.setLastMove(move->next() == 0);
            entry.setGameNum(i);

            // Write the opening tree entry to the database

            if (stmt.prepare(
                    "INSERT INTO optree (pos, move, score, last_move, game_id) "
                    "VALUES (?, ?, ?, ?, ?)") &&
                stmt.bind(1, entry.hashKey()) &&
                stmt.bind(2, (int)entry.move().intValue()) &&
                stmt.bind(3, entry.score()) &&
                stmt.bind(4, entry.lastMove()) &&
                stmt.bind(5, (int)entry.gameNum())) {
                rv = stmt.step();

                if (rv == SQLITE_DONE) {
                    LOGDBG << "Inserted optree entry for game " << i << ", depth " << count;

                    if (callback) {
                        float complete = static_cast<float> ((i * 100) / last);

                        if (!callback(i, complete, contextInfo)) {
                            DBERROR << "User cancelled operation";
                            return false;
                        }
                    }
                } else {
                    setDbErrorMsg("Failed to insert optree entry for game %u, depth %u", i, count);
                    return false;
                }
            } else {
                setDbErrorMsg("Failed to prepare optree insert statement for game %u, depth %u", i, count);
                return false;
            }
        }
    }

    return true;
}

bool CfdbDatabase::searchOpeningTree(uint64_t hashKey, bool lastMoveOnly, vector<OpeningTreeEntry> &entries) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    //logdbg("hashKey=0x%016llx, current=%s", hashKey, current ? "true" : "false");

    int rv;
    string sql = "SELECT move, score, last_move, game_id FROM optree WHERE pos = ?";

    if (lastMoveOnly) sql += " AND last_move <> 0";

    SqliteStatement stmt(m_db);

    if (!stmt.prepare(sql) ||
        !stmt.bind(1, hashKey)) {
        setDbErrorMsg("Failed to prepare optree select statement");
        return false;
    }

    OpeningTreeEntry entry;

    while ((rv = stmt.step()) == SQLITE_ROW) {
        entry.setHashKey(hashKey);
        entry.setMove((uint32_t)stmt.columnInt(0));
        entry.setScore(stmt.columnInt(1));
        entry.setLastMove(stmt.columnBool(2));
        entry.setGameNum((unsigned)stmt.columnInt(3));

        LOGVERBOSE << "Matched " << entry.dump();

        entries.push_back(entry);
    }

    if (rv != SQLITE_DONE) {
        setDbErrorMsg("Failed to select optree row");
        return false;
    }

    LOGDBG << entries.size() << " matches";

    return true;
}

bool CfdbDatabase::countInOpeningTree(uint64_t hashKey, unsigned &count) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    count = 0;

    int rv;
    bool retval = false;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT COUNT(*) FROM optree WHERE pos = ?") &&
        stmt.bind(1, hashKey)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            count = (unsigned)stmt.columnInt(0);
            retval = true;
        } else setDbErrorMsg("Failed to select count from optree");
    } else setDbErrorMsg("Failed to prepare optree select count statement");

    LOGVERBOSE << "hashKey=0x" << hex << setw(16) << setfill('0') << hashKey <<
        ", count=" << dec << setw(0) << count;

    return retval;
}

bool CfdbDatabase::countLongestLine(unsigned &count) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    count = 0;

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT MAX(halfmoves) FROM game")) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            count = (unsigned)stmt.columnInt(0);
            retval = true;
        } else setDbErrorMsg("Failed to select from game");
    } else setDbErrorMsg("Failed to prepare SELECT MAX statement");

    return retval;
}

bool CfdbDatabase::search(const DatabaseSearchCriteria &searchCriteria, const DatabaseSortCriteria &sortCriteria,
                          DATABASE_CALLBACK_FUNC callback, void *contextInfo, int offset /*=0*/, int limit /*=0*/) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    if (callback == 0) {
        DBERROR << "No callback function provided";
        return false;
    }

    vector<string> binds;
    set<string> joins;
    ostringstream query, where, orderBy;

    for (auto it = searchCriteria.begin(); it != searchCriteria.end(); ++it) {
        const DatabaseSearchDescriptor &descriptor = *it;

        bool caseInsensitive = databaseComparisonCaseInsensitive(descriptor.comparison);

        if (!binds.empty()) where << " AND";

        switch (descriptor.field) {
        case DATABASE_FIELD_WHITEPLAYER:
            where << (caseInsensitive ? " UPPER(whiteplayer.last_name)" : " whiteplayer.last_name") <<
                comparisonString(descriptor.comparison);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            joins.insert("white_player_id");
            break;

        case DATABASE_FIELD_BLACKPLAYER:
            where << (caseInsensitive ? " UPPER(blackplayer.last_name)" : " blackplayer.last_name") <<
                comparisonString(descriptor.comparison);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            joins.insert("black_player_id");
            break;

        case DATABASE_FIELD_PLAYER:
            where <<
                " (" <<
                (caseInsensitive ? "UPPER(whiteplayer.last_name)" : "whiteplayer.last_name") <<
                comparisonString(descriptor.comparison) << " OR " <<
                (caseInsensitive ? "UPPER(blackplayer.last_name)" : "blackplayer.last_name") <<
                comparisonString(descriptor.comparison) << ")";
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            joins.insert("white_player_id");
            joins.insert("black_player_id");
            break;

        case DATABASE_FIELD_EVENT:
            where << (caseInsensitive ? " UPPER(event.name)" : " event.name") <<
                comparisonString(descriptor.comparison);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            joins.insert("event_id");
            break;

        case DATABASE_FIELD_SITE:
            where << (caseInsensitive ? " UPPER(site.name)" : " site.name") <<
                comparisonString(descriptor.comparison);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            joins.insert("site_id");
            break;

        case DATABASE_FIELD_DATE:

            if (databaseComparisonNoFlags(descriptor.comparison) == DATABASE_COMPARE_EQUALS) {
                // This could probably use bind variables as well, but it's (wrongly )assumed that all
                // bind variables are strings, so that would need changing first...
                int year = 0, month = 0, day = 0;

                if (sscanf(descriptor.value.c_str(), "%04d%02d%02d", &year, &month, &day) == 3)
                    // Compare with exact year/month/day
                    where << " game.date = " << Util::format("%04d%02d%02d", year, month, day);
                else if (sscanf(descriptor.value.c_str(), "%04d%02d", &year, &month) == 2)
                    // Compare with this month and end of month
                    where << " (game.date >= " << Util::format("%04d%02d00", year, month) <<
                        " AND game.date <= " << Util::format("%04d%02d31", year, month) << ")";
                else if (sscanf(descriptor.value.c_str(), "%04d", &year) == 1)
                    // Compare with this year and end of year
                    where << " (game.date >= " << Util::format("%04d0000", year) <<
                        " AND game.date <= " << Util::format("%04d1231", year) << ")";
                else {
                    DBERROR << "Cannot search for date using invalid value '" << descriptor.value << "'";
                    return false;
                }
            } else LOGWRN << "Ignoring to request to search for date as comparison is not \"equals\"";

            break;

        case DATABASE_FIELD_ECO:
            where << (caseInsensitive ? " UPPER(game.eco)" : " game.eco") <<
                comparisonString(descriptor.comparison);
            binds.push_back(caseInsensitive ? Util::toupper(descriptor.value) : descriptor.value);
            break;

        default:
            DBERROR << "Field " << descriptor.field << " cannot be used for searching";
            return false;
        }
    }

    if (sortCriteria.empty()) orderBy << "game.game_id ASC";
    else
        for (auto it = sortCriteria.begin(); it != sortCriteria.end(); ++it) {
            const DatabaseSortDescriptor &descriptor = *it;

            if (orderBy.tellp() > 0) orderBy << ", ";

            switch (descriptor.field) {
            case DATABASE_FIELD_GAME_NUM:
                orderBy << "game.game_id" << orderString(descriptor.order);
                break;

            case DATABASE_FIELD_WHITEPLAYER:
                orderBy << "whiteplayer.last_name" << orderString(descriptor.order);
                joins.insert("white_player_id");
                break;

            case DATABASE_FIELD_BLACKPLAYER:
                orderBy << "blackplayer.last_name" << orderString(descriptor.order);
                joins.insert("black_player_id");
                break;

            case DATABASE_FIELD_EVENT:
                orderBy << "event.name" << orderString(descriptor.order);
                joins.insert("event_id");
                break;

            case DATABASE_FIELD_SITE:
                orderBy << "site.name" << orderString(descriptor.order);
                joins.insert("site_id");
                break;

            case DATABASE_FIELD_ROUND:
                orderBy << "game.round_major" << orderString(descriptor.order) <<
                    ", game.round_minor" << orderString(descriptor.order);
                break;

            case DATABASE_FIELD_DATE:
                orderBy << "game.date" << orderString(descriptor.order);
                break;

            case DATABASE_FIELD_ECO:
                orderBy << "game.eco" << orderString(descriptor.order);
                break;

            case DATABASE_FIELD_RESULT:
                orderBy << "game.result" << orderString(descriptor.order);
                break;

            default:
                DBERROR << "Field " << descriptor.field << " cannot be used for sorting";
                return false;
            }
        }

    query << "SELECT game.game_id FROM game";

    bool whereEmpty = where.str().empty();

    if (!whereEmpty && joins.size() > 0)
        where << " AND (";

    for (auto it = joins.begin(); it != joins.end(); ++it) {
        const string &column = *it;

        if (it != joins.begin()) where << " AND";

        if (column == "white_player_id") {
            query << ", player whiteplayer";
            where << " game.white_player_id = whiteplayer.player_id";
        } else if (column == "black_player_id") {
            query << ", player blackplayer";
            where << " game.black_player_id = blackplayer.player_id";
        } else if (column == "event_id") {
            query << ", event";
            where << " game.event_id = event.event_id";
        } else if (column == "site_id") {
            query << ", site";
            where << " game.site_id = site.site_id";
        } else ASSERT(false);
    }

    if (!whereEmpty && joins.size() > 0) where << ")";

    if (where.tellp() > 0) query << " WHERE" << where.str();

    query << " GROUP BY game.game_id";
    query << " ORDER BY " << orderBy.str();

    if (limit > 0) query << " LIMIT " << limit;

    if (offset > 0) query << " OFFSET " << offset - 1;

    LOGDBG << "Query: " << query.str();

    SqliteStatement stmt(m_db);

    if (stmt.prepare(query.str())) {
        int bindColumn = 1, rv;

        for (auto it = binds.begin(); it != binds.end(); ++it) {
            const string &value = *it;

            if (!stmt.bind(bindColumn, value)) {
                LOGERR << "Failed to bind column " << bindColumn << " with value '" << value << "'";
                return false;
            }

            bindColumn++;
        }

        while ((rv = stmt.step()) == SQLITE_ROW) {
            unsigned gameNum = (unsigned)stmt.columnInt(0);

            if (!callback(gameNum, 0.0f, contextInfo)) {
                LOGINF << "User terminated search";
                return true;
            }
        }

        if (rv != SQLITE_DONE) {
            setDbErrorMsg("Failed to search games");
            return false;
        }
    } else {
        setDbErrorMsg("Failed to prepare search statement");
        return false;
    }

    return true;
}

unsigned CfdbDatabase::numGames() {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return 0;
    }

    unsigned count = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT COUNT(*) FROM game")) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            count = (unsigned)stmt.columnInt(0);
            LOGDBG << "Database '" << m_filename << "' contains " << count << " games";
        } else if (rv == SQLITE_DONE) DBERROR << "Failed to count games";
        else setDbErrorMsg("Failed to count games");
    } else setDbErrorMsg("Failed to prepare SELECT COUNT statement");

    return count;
}

unsigned CfdbDatabase::firstGameNum() {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return 0;
    }

    unsigned gameNum = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT MIN(game_id) FROM game")) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) gameNum = (unsigned)stmt.columnInt(0);
        else setDbErrorMsg("Failed to get first game number");
    } else setDbErrorMsg("Failed to prepare SELECT MIN statement");

    return gameNum;
}

unsigned CfdbDatabase::lastGameNum() {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return 0;
    }

    unsigned gameNum = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT MAX(game_id) FROM game")) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) gameNum = (unsigned)stmt.columnInt(0);
        else setDbErrorMsg("Failed to get last game number");
    } else setDbErrorMsg("Failed to prepare SELECT MAX statement");

    return gameNum;
}

bool CfdbDatabase::gameExists(unsigned gameNum) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    unsigned count = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT COUNT(*) FROM game WHERE game_id = ?") &&
        stmt.bind(1, (int)gameNum)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) count = (unsigned)stmt.columnInt(0);
        else DBERROR << "Failed to count game where game_id=" << gameNum;
    } else setDbErrorMsg("Failed to prepare game exists statement");

    return count == 1;
}

bool CfdbDatabase::createSchema() {
    clearErrorMsg();

    SqliteStatement stmt(m_db);

    LOGDBG << "Creating table 'metadata'";

    if (!stmt.prepare(
            "CREATE TABLE metadata ("
            "name TEXT PRIMARY KEY, "
            "val TEXT)") || stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'metadata'");
        return false;
    }

    LOGDBG << "Creating table 'game'";

    if (!stmt.prepare(
            "CREATE TABLE game ("
            "game_id INTEGER PRIMARY KEY, "
            "white_player_id INTEGER, "
            "black_player_id INTEGER, "
            "event_id INTEGER, "
            "site_id INTEGER, "
            "date INTEGER, "
            "round_major INTEGER, "
            "round_minor INTEGER, "
            "result INTEGER, "
            "annotator_id INTEGER, "
            "eco TEXT, "
            "white_elo INTEGER, "
            "black_elo INTEGER, "
            "time_control BLOB, "
            "halfmoves INTEGER, "
            "partial BLOB, "
            "moves BLOB, "
            "annotations BLOB)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'game'");
        return false;
    }

    LOGDBG << "Creating index 'game_index'";

    if (!stmt.prepare(
            "CREATE UNIQUE INDEX game_index ON game (game_id)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'game_index'");
        return false;
    }

    LOGDBG << "Creating table 'player'";

    if (!stmt.prepare(
            "CREATE TABLE player ("
            "player_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "last_name TEXT, "
            "first_names TEXT, "
            "country_code TEXT)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'player'");
        return false;
    }

    LOGDBG << "Creating index 'player_index'";

    if (!stmt.prepare("CREATE UNIQUE INDEX player_index ON player (player_id)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'player_index'");
        return false;
    }

    LOGDBG << "Creating index 'player_last_name_index'";

    if (!stmt.prepare(
            "CREATE INDEX player_last_name_index ON player (last_name)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'player_last_name_index'");
        return false;
    }

    LOGDBG << "Creating table 'event'";

    if (!stmt.prepare(
            "CREATE TABLE event ("
            "event_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'event'");
        return false;
    }

    LOGDBG << "Creating index 'event_index'";

    if (!stmt.prepare("CREATE UNIQUE INDEX event_index ON event (event_id)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'event_index'");
        return false;
    }

    LOGDBG << "Creating index 'event_name_index'";

    if (!stmt.prepare("CREATE INDEX event_name_index ON event (name)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'event_name_index'");
        return false;
    }

    LOGDBG << "Creating table 'site'";

    if (!stmt.prepare(
            "CREATE TABLE site ("
            "site_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'site'");
        return false;
    }

    LOGDBG << "Creating index 'site_index'";

    if (!stmt.prepare("CREATE UNIQUE INDEX site_index ON site (site_id)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'site_index'");
        return false;
    }

    LOGDBG << "Creating index 'site_name_index'";

    if (!stmt.prepare("CREATE INDEX site_name_index ON site (name)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'site_name_index'");
        return false;
    }

    LOGDBG << "Creating table 'annotator'";

    if (!stmt.prepare(
            "CREATE TABLE annotator ("
            "annotator_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT)") ||
        stmt.step() != SQLITE_DONE) setDbErrorMsg("Failed to create table 'annotator'");

    LOGDBG << "Creating index 'annotator_index'";

    if (!stmt.prepare(
            "CREATE UNIQUE INDEX annotator_index ON annotator (annotator_id)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'annotator_index'");
        return false;
    }

    LOGDBG << "Creating index 'annotator_name_index'";

    if (!stmt.prepare(
            "CREATE INDEX annotator_name_index ON annotator (name)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'annotator_name_index'");
        return false;
    }

    LOGDBG << "Creating table 'optree'";

    if (!stmt.prepare(
            "CREATE TABLE optree ("
            "pos UNSIGNED BIG INT, "
            "move INTEGER, "
            "score TINYINT, "
            "last_move TINYINT, "
            "game_id INTEGER)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create table 'optree'");
        return false;
    }

    LOGDBG << "Creating index 'optree_pos_index'";

    if (!stmt.prepare(
            "CREATE INDEX optree_pos_index ON optree (pos)") ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to create index 'optree_pos_index'");
        return false;
    }

    LOGDBG << "Populating 'metadata' table";

    if (!stmt.prepare("INSERT INTO metadata (name, val) VALUES (?, ?)") ||
        !stmt.bind(1, "schema_version") ||
        !stmt.bind(2, CURRENT_SCHEMA_VERSION) ||
        stmt.step() != SQLITE_DONE) {
        setDbErrorMsg("Failed to set schema version");
        return false;
    }

    return true;
}

bool CfdbDatabase::checkSchema() {
    clearErrorMsg();

    SqliteStatement stmt(m_db);

    int stepval = SQLITE_OK;

    if (!stmt.prepare("SELECT val FROM metadata WHERE name = ?") ||
        !stmt.bind(1, "schema_version") ||
        (stepval = stmt.step()) != SQLITE_ROW) {
        if (stepval == SQLITE_DONE)
            DBERROR << "Schema version is not set in the database";
        else
            setDbErrorMsg("Failed to select schema_version");

        return false;
    }

    int schemaVersion = stmt.columnInt(0);
    LOGDBG << "Schema version is " << schemaVersion;

    if (schemaVersion != CURRENT_SCHEMA_VERSION) {
        DBERROR << "Database is using an unsupported schema version (" << schemaVersion << ")";
        return false;
    }

    return true;
}

bool CfdbDatabase::decodeMoves(Game &game, const Blob &moves, const Blob &annotations) {
    clearErrorMsg();

    ASSERT(moves.data());
    ASSERT(moves.length());

    Bitstream moveBitstream(moves);
    const uint8_t *pannot = annotations.data();
    AnnotMove *lastMove = 0;

    while (moveBitstream.readOffset() < moves.length()) {
        uint32_t encodedMove;

        if (!moveBitstream.read(encodedMove, ENCMOVE_TYPE_BITSIZE)) {
            DBERROR << "Failed to read from move bitstream at offset " << moveBitstream.readOffset();
            return false;
        }

        if (encodedMove == ENCMOVE_TYPE_VARSTART) {
            if (!game.startVariation()) {
                DBERROR << "Failed to start variation after move '" <<
                    (lastMove ? lastMove->dump() : "none") << "'";
                return false;
            }

            lastMove = 0;
        } else if (encodedMove == ENCMOVE_TYPE_VAREND) {
            if (!game.endVariation()) {
                DBERROR << "Failed to end variation after move '" <<
                    (lastMove ? lastMove->dump() : "none") << "'";
                return false;
            }

            lastMove = 0;
        } else { // Move
            unsigned bitcount;

            if (encodedMove == ENCMOVE_TYPE_MOVE) bitcount = ENCMOVE_MOVE_BITSIZE;
            else // encodedMove == ENCMOVE_TYPE_ANNOTMOVE
                bitcount = ENCMOVE_ANNOTMOVE_BITSIZE;

            if (!moveBitstream.read(encodedMove, bitcount)) {
                DBERROR << "Failed to read from move bitstream at offset " << moveBitstream.readOffset();
                return false;
            }

            // Is this the end-of-game marker?
            if (bitcount == ENCMOVE_ANNOTMOVE_BITSIZE && encodedMove == 0)
                return true;

            // It is necessary to ask the for the 'game over' condition when making the
            // move in order to get the returned move to have the right flags (e.g. mate).
            Game::GameOver gameOver = Game::GAMEOVER_NOT;
            lastMove = game.makeMove(encodedMove & ENCMOVE_MOVE_INDEX_MASK, 0, 0, false, &gameOver, 0);

            if (lastMove) {
                if (encodedMove & ENCMOVE_PRE_ANNOT_BIT) {
                    ASSERT(pannot < annotations.end());
                    lastMove->setPreAnnot((const char *)pannot);
                    pannot += strlen((const char *)pannot) + 1;
                }

                if (encodedMove & ENCMOVE_POST_ANNOT_BIT) {
                    ASSERT(pannot < annotations.end());
                    lastMove->setPostAnnot((const char *)pannot);
                    pannot += strlen((const char *)pannot) + 1;
                }

                if (encodedMove & ENCMOVE_NAGS_BIT) {
                    ASSERT(pannot < annotations.end());

                    while (*pannot != NAG_NONE)
                        lastMove->addNag((Nag) * pannot++);

                    pannot++; // Skip terminating NAG_NONE
                }
            } else {
                DBERROR << "Failed to make move " << (unsigned)(encodedMove & ENCMOVE_MOVE_INDEX_MASK);
                return false;
            }
        }
    }

    DBERROR << "End of blob encountered before end-of-marker found";
    return false;
}

bool CfdbDatabase::encodeMoves(const Game &game, Blob &moves, Blob &annotations) {
    clearErrorMsg();

    moves.free();
    annotations.free();

    if (game.mainline() == 0)
        return true; // Nothing to do

    // Reserve enough space in the blobs to hold the moves and annotations
    // (assumes that each move will take 2 bytes, but it won't)
    unsigned moveCount = 0, variationCount = 0, symbolCount = 0, annotationsLength = 0;
    AnnotMove::count(game.mainline(), moveCount, variationCount, symbolCount, annotationsLength);

    if (moveCount > 0 && !moves.reserve((moveCount * 2) + 2)) {
        DBERROR << "Failed to reserve space in the moves blob";
        return false;
    }

    if (annotationsLength + symbolCount > 0 && !annotations.reserve(annotationsLength + symbolCount)) {
        DBERROR << "Failed to reserve space in the annotations blob";
        return false;
    }

    Bitstream moveBitstream(moves);

    if (!encodeMoves(game.mainline(), moveBitstream, annotations, false))
        return false;

    // Add the end-of-game marker (an annotmove with all bits clear)
    if (!moveBitstream.write(ENCMOVE_TYPE_ANNOTMOVE, ENCMOVE_TYPE_BITSIZE) ||
        !moveBitstream.write(0, ENCMOVE_ANNOTMOVE_BITSIZE)) {
        DBERROR << "Failed to write to move bitstream at offset " << moveBitstream.writeOffset();
        return false;
    }

    return true;
}

bool CfdbDatabase::encodeMoves(const AnnotMove *amove, Bitstream &moveBitstream, Blob &annotations, bool isVariation) {
    Move moves[256];

    ASSERT(amove);

    // There must be a priorPosition available for this line
    const AnnotMove *m = amove;

    while (m->mainline())
        m = m->mainline();

    ASSERT(m->priorPosition());
    Position pos(m->priorPosition());

    if (isVariation)
        if (!moveBitstream.write(ENCMOVE_TYPE_VARSTART, ENCMOVE_TYPE_BITSIZE)) {
            DBERROR << "Failed to write to move bitstream at offset " << moveBitstream.writeOffset();
            return false;
        }

    while (amove) {
        // Get the index of the move
        unsigned moveIndex;
        unsigned numMoves = pos.genMoves(moves);

        for (moveIndex = 0; moveIndex < numMoves; moveIndex++)
            if (amove->equals(moves[moveIndex])) break;

        if (moveIndex == numMoves) {
            DBERROR << "Failed to get index of move '" << amove->dump(false) << "'";
            return false;
        }

        uint32_t encodedMove = (uint32_t)moveIndex;
        bool isAnnotMove = false;

        const string &preAnnot = amove->preAnnot();

        if (!preAnnot.empty()) {
            if (!annotations.add((const uint8_t *)preAnnot.c_str(), (unsigned)preAnnot.length() + 1)) {
                DBERROR << "Failed to add pre-annotation to blob";
                return false;
            }

            encodedMove |= ENCMOVE_PRE_ANNOT_BIT;
            isAnnotMove = true;
        }

        const string &postAnnot = amove->postAnnot();

        if (!postAnnot.empty()) {
            if (!annotations.add((const uint8_t *)postAnnot.c_str(), (unsigned)postAnnot.length() + 1)) {
                DBERROR << "Failed to add post-annotation to blob";
                return false;
            }

            encodedMove |= ENCMOVE_POST_ANNOT_BIT;
            isAnnotMove = true;
        }

        if (amove->nagCount() > 0) {
            // Build and add symbols, ensuring there are no gaps
            Nag nags[STORED_NAGS + 1];
            unsigned numNags = amove->nags(nags);
            ASSERT(numNags == amove->nagCount());
            nags[numNags++] = NAG_NONE;

            if (!annotations.add((const uint8_t *)nags, numNags)) {
                DBERROR << "Failed to add NAGs to blob";
                return false;
            }

            encodedMove |= ENCMOVE_NAGS_BIT;
            isAnnotMove = true;
        }

        uint32_t moveType;
        unsigned bitsize;

        if (isAnnotMove) {
            moveType = ENCMOVE_TYPE_ANNOTMOVE;
            bitsize = ENCMOVE_ANNOTMOVE_BITSIZE;
        } else {
            moveType = ENCMOVE_TYPE_MOVE;
            bitsize = ENCMOVE_MOVE_BITSIZE;
        }

        // Write move type
        if (!moveBitstream.write(moveType, ENCMOVE_TYPE_BITSIZE) ||
            !moveBitstream.write(encodedMove, bitsize)) {
            DBERROR << "Failed to write to move bitstream at offset " << moveBitstream.writeOffset();
            return false;
        }

        // Make the move in the position
        UnmakeMoveInfo umi;

        if (!pos.makeMove(amove, umi)) {
            DBERROR << "Failed to make move '" << amove->dump(false) << "'";
            return false;
        }

        if (amove->variation())
            if (amove->mainline() == 0)
                // Top of variation tree
                for (m = amove->variation(); m; m = m->variation())
                    if (!encodeMoves(m, moveBitstream, annotations, true)) return false;


        amove = amove->next();
    }

    if (isVariation)
        if (!moveBitstream.write(ENCMOVE_TYPE_VAREND, ENCMOVE_TYPE_BITSIZE)) {
            DBERROR << "Failed to write to move bitstream at offset " << moveBitstream.writeOffset();
            return false;
        }

    return true;
}

unsigned CfdbDatabase::selectPlayer(const Player &player) {
    clearErrorMsg();

    if (!player.hasName()) {
        DBERROR << "Cannot find player without name";
        return 0;
    }

    unsigned playerId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    // Build dynamic statement
    unsigned bindCount = 1, lastNameBind = 0, firstNamesBind = 0, countryCodeBind = 0;
    ostringstream sql;
    sql << "SELECT player_id FROM player WHERE";

    if (!player.lastName().empty()) {
        sql << " last_name = ?";
        lastNameBind = bindCount++;
    }

    if (!player.firstNames().empty()) {
        if (bindCount > 1) sql << " AND";

        sql << " first_names = ?";
        firstNamesBind = bindCount++;
    }

    if (!player.countryCode().empty()) {
        if (bindCount > 1) sql << " AND";

        sql << " country_code = ?";
        countryCodeBind = bindCount++;
    }

    if (!stmt.prepare(sql.str())) {
        setDbErrorMsg("Failed to prepare select player statement");
        return 0;
    }

    bool bindOK = true;

    if (lastNameBind > 0) bindOK = bindOK && stmt.bind(lastNameBind, player.lastName());

    if (firstNamesBind > 0) bindOK = bindOK && stmt.bind(firstNamesBind, player.firstNames());

    if (countryCodeBind > 0) bindOK = bindOK && stmt.bind(countryCodeBind, player.countryCode());

    if (!bindOK) {
        setDbErrorMsg("Failed to bind select player parameters");
        return 0;
    }

    rv = stmt.step();

    if (rv == SQLITE_ROW) {
        playerId = (unsigned)stmt.columnInt(0);
        LOGVERBOSE << "Player '" << player << "' has player_id " << playerId;
    } else if (rv == SQLITE_DONE) LOGVERBOSE << "Player '" << player << "' does not exist";
    else setDbErrorMsg("Failed to select player '%s'", player.formattedName().c_str());

    return playerId;
}

bool CfdbDatabase::selectPlayer(unsigned id, Player &player) {
    clearErrorMsg();

    player.initPlayer();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT last_name, first_names, country_code FROM player WHERE player_id = ?") &&
        stmt.bind(1, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            player.setLastName(stmt.columnString(0));
            player.setFirstNames(stmt.columnString(1));
            player.setCountryCode(stmt.columnString(2));
            retval = true;
            LOGVERBOSE << "Player " << id << " is '" << player << "'";
        } else if (rv == SQLITE_DONE) LOGVERBOSE << "Player " << id << " does not exist";
        else setDbErrorMsg("Failed to select player %u", id);
    } else setDbErrorMsg("Failed to prepare player select statement");

    return retval;
}

unsigned CfdbDatabase::insertPlayer(const Player &player) {
    clearErrorMsg();

    unsigned playerId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("INSERT INTO player (last_name, first_names, country_code) VALUES (?, ?, ?)") &&
        stmt.bind(1, player.lastName()) &&
        stmt.bind(2, player.firstNames()) &&
        stmt.bind(3, player.countryCode())) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) {
            playerId = stmt.lastInsertId();
            LOGVERBOSE << "Player '" << player << "' has player_id " << playerId;
        } else setDbErrorMsg("Failed to insert player '%s'", player.formattedName().c_str());
    } else setDbErrorMsg("Failed to prepare player insert statement");

    return playerId;
}

bool CfdbDatabase::updatePlayer(unsigned id, const Player &player) {
    clearErrorMsg();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("UPDATE player SET last_name = ?, first_names = ?, country_code = ? WHERE player_id = ?")
        &&
        stmt.bind(1, player.lastName()) &&
        stmt.bind(2, player.firstNames()) &&
        stmt.bind(3, player.countryCode()) &&
        stmt.bind(4, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) retval = true;
        else setDbErrorMsg("Failed to update player '%s' (%u)", player.formattedName().c_str(), id);
    } else setDbErrorMsg("Failed to prepare player update statement");

    return retval;
}

unsigned CfdbDatabase::selectEvent(const string &name) {
    clearErrorMsg();

    if (name.empty()) {
        DBERROR << "Cannot find event without name";
        return 0;
    }

    unsigned eventId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (!stmt.prepare("SELECT event_id FROM event WHERE name = ?") ||
        !stmt.bind(1, name)) {
        setDbErrorMsg("Failed to prepare select event statement");
        return 0;
    }

    rv = stmt.step();

    if (rv == SQLITE_ROW) {
        eventId = (unsigned)stmt.columnInt(0);
        LOGVERBOSE << "Event '" << name << "' has event_id " << eventId;
    } else if (rv == SQLITE_DONE) LOGVERBOSE << "Event '" << name << "' does not exist";
    else
        setDbErrorMsg("Failed to select event name='%s'",
                      name.c_str());

    return eventId;
}

bool CfdbDatabase::selectEvent(unsigned id, string &name) {
    clearErrorMsg();

    name.clear();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT name FROM event WHERE event_id = ?") &&
        stmt.bind(1, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            stmt.columnString(0, name);
            retval = true;
            LOGVERBOSE << "Event " << id << " is '" << name << "'";
        } else if (rv == SQLITE_DONE) LOGVERBOSE << "Event " << id << " does not exist";
        else setDbErrorMsg("Failed to select event %u", id);
    } else setDbErrorMsg("Failed to prepare event select statement");

    return retval;
}

unsigned CfdbDatabase::insertEvent(const string &name) {
    clearErrorMsg();

    unsigned eventId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("INSERT INTO event (name) VALUES (?)") &&
        stmt.bind(1, name)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) {
            eventId = stmt.lastInsertId();
            LOGVERBOSE << "Event '" << name << "' has event_id " << eventId;
        } else setDbErrorMsg("Failed to insert event '%s'", name.c_str());
    } else setDbErrorMsg("Failed to prepare event insert statement");

    return eventId;
}

bool CfdbDatabase::updateEvent(unsigned id, const string &name) {
    clearErrorMsg();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("UPDATE event SET name = ? WHERE event_id = ?") &&
        stmt.bind(1, name) &&
        stmt.bind(2, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) retval = true;
        else setDbErrorMsg("Failed to update event %u", id);
    } else setDbErrorMsg("Failed to prepare event update statement");

    return retval;
}

unsigned CfdbDatabase::selectSite(const string &name) {
    clearErrorMsg();

    if (name.empty()) {
        DBERROR << "Cannot find site without name";
        return 0;
    }

    unsigned siteId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (!stmt.prepare("SELECT site_id FROM site WHERE name = ?") ||
        !stmt.bind(1, name)) {
        setDbErrorMsg("Failed to prepare select site statement");
        return 0;
    }

    rv = stmt.step();

    if (rv == SQLITE_ROW) {
        siteId = (unsigned)stmt.columnInt(0);
        LOGVERBOSE << "Site '" << name << "' has site_id " << siteId;
    } else if (rv == SQLITE_DONE) LOGVERBOSE << "Site '" << name << "' does not exist";
    else
        setDbErrorMsg("Failed to select site name='%s'",
                      name.c_str());

    return siteId;
}

bool CfdbDatabase::selectSite(unsigned id, string &name) {
    clearErrorMsg();

    name.clear();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT name FROM site WHERE site_id = ?") &&
        stmt.bind(1, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            stmt.columnString(0, name);
            retval = true;
            LOGVERBOSE << "Site " << id << " is '" << name << "'";
        } else if (rv == SQLITE_DONE) LOGVERBOSE << "Site " << id << " does not exist";
        else setDbErrorMsg("Failed to select site %u", id);
    } else setDbErrorMsg("Failed to prepare site select statement");

    return retval;
}

unsigned CfdbDatabase::insertSite(const string &name) {
    clearErrorMsg();

    unsigned siteId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("INSERT INTO site (name) VALUES (?)") &&
        stmt.bind(1, name)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) {
            siteId = stmt.lastInsertId();
            LOGVERBOSE << "Site '" << name << "' has site_id " << siteId;
        } else setDbErrorMsg("Failed to insert site '%s'", name.c_str());
    } else setDbErrorMsg("Failed to prepare site insert statement");

    return siteId;
}

bool CfdbDatabase::updateSite(unsigned id, const string &name) {
    clearErrorMsg();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("UPDATE site SET name = ? WHERE site_id = ?") &&
        stmt.bind(1, name) &&
        stmt.bind(2, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) retval = true;
        else setDbErrorMsg("Failed to update site %u", id);
    } else setDbErrorMsg("Failed to prepare site update statement");

    return retval;
}

unsigned CfdbDatabase::selectAnnotator(const string &name) {
    clearErrorMsg();

    if (name.empty()) {
        DBERROR << "Cannot find annotator without name";
        return 0;
    }

    unsigned annotatorId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (!stmt.prepare("SELECT annotator_id FROM annotator WHERE name = ?") ||
        !stmt.bind(1, name)) {
        setDbErrorMsg("Failed to prepare select annotator statement");
        return 0;
    }

    rv = stmt.step();

    if (rv == SQLITE_ROW) {
        annotatorId = (unsigned)stmt.columnInt(0);
        LOGVERBOSE << "Annotator '" << name << "' has annotator_id " << annotatorId;
    } else if (rv == SQLITE_DONE) LOGVERBOSE << "Annotator '" << name << "' does not exist";
    else
        setDbErrorMsg("Failed to select annotator name='%s'",
                      name.c_str());

    return annotatorId;
}

bool CfdbDatabase::selectAnnotator(unsigned id, string &name) {
    clearErrorMsg();

    name.clear();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("SELECT name FROM annotator WHERE annotator_id = ?") &&
        stmt.bind(1, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_ROW) {
            stmt.columnString(0, name);
            retval = true;
            LOGVERBOSE << "Annotator " << id << " is '" << name << "'";
        } else if (rv == SQLITE_DONE) LOGVERBOSE << "Annotator " << id << " does not exist";
        else setDbErrorMsg("Failed to select annotator %u", id);
    } else setDbErrorMsg("Failed to prepare annotator select statement");

    return retval;
}

unsigned CfdbDatabase::insertAnnotator(const string &name) {
    clearErrorMsg();

    unsigned annotatorId = 0;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("INSERT INTO annotator (name) VALUES (?)") &&
        stmt.bind(1, name)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) {
            annotatorId = stmt.lastInsertId();
            LOGVERBOSE << "Annotator '" << name << "' has annotator_id " << annotatorId;
        } else setDbErrorMsg("Failed to insert annotator '%s'", name.c_str());
    } else setDbErrorMsg("Failed to prepare annotator insert statement");

    return annotatorId;
}

bool CfdbDatabase::updateAnnotator(unsigned id, const string &name) {
    clearErrorMsg();

    bool retval = false;
    int rv;
    SqliteStatement stmt(m_db);

    if (stmt.prepare("UPDATE annotator SET name = ? WHERE annotator_id = ?") &&
        stmt.bind(1, name) &&
        stmt.bind(2, (int)id)) {
        rv = stmt.step();

        if (rv == SQLITE_DONE) retval = true;
        else setDbErrorMsg("Failed to update annotator %u", id);
    } else setDbErrorMsg("Failed to prepare annotator update statement");

    return retval;
}

void CfdbDatabase::setDbErrorMsg(const char *message, ...) {
    char buffer[1024];
    va_list va;

    va_start(va, message);
    vsprintf(buffer, message, va);
    va_end(va);

    if (m_isOpen) {
        DBERROR << buffer << ": " << sqlite3_errmsg(m_db);
        LOGERR << buffer << ": " << sqlite3_errmsg(m_db);
    } else {
        DBERROR << buffer;
        LOGERR << buffer;
    }
}

string CfdbDatabase::comparisonString(DatabaseComparison comparison) {
    switch (databaseComparisonNoFlags(comparison)) {
    case DATABASE_COMPARE_EQUALS:
        return " = ?";

    case DATABASE_COMPARE_STARTSWITH:
        return " LIKE ?||'%'";

    case DATABASE_COMPARE_CONTAINS:
        return " LIKE '%'||?||'%'";

    default:
        break;
    }

    ASSERT(false);
    return "";
}

string CfdbDatabase::orderString(DatabaseOrder order) {
    switch (order) {
    case DATABASE_ORDER_ASCENDING:
        return " ASC";

    case DATABASE_ORDER_DESCENDING:
        return " DESC";

    default:
        break;
    }

    ASSERT(false);
    return "";
}
}   // namespace ChessCore
