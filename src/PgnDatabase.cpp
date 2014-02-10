//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// PgnDatabase.cpp: PGN Database class implementation.
//

#define VERBOSE_LOGGING 0

#include <ChessCore/PgnDatabase.h>
#include <ChessCore/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>

using namespace std;

namespace ChessCore {
#define EMBEDDED_VARIATIONS 0

// Database Factory
static shared_ptr<Database> databaseFactory(const string &dburl, bool readOnly) {
    shared_ptr<Database> db;
    if (Util::endsWith(dburl, ".pgn", false)) {
        db.reset(new PgnDatabase(dburl, readOnly));
    }
    return db;
}

static bool registered = Database::registerFactory(databaseFactory);

//
// IMPORTANT: The PgnScanner class will throw ChessCoreExceptons if a fatal error
// error occurs.  Any access to functions in the PgnScanner file must be surrounded
// try-catch blocks!
//

const char *PgnDatabase::m_classname = "PgnDatabase";
bool PgnDatabase::m_relaxedParsing = false;
IndexManager PgnDatabase::m_indexManager;

Nag PgnDatabase::m_nagMap[NUM_PGN_NAGS] = {
    NAG_NONE,           // 0     null annotation
    NAG_GOOD_MOVE,      // 1     good move (traditional "!")
    NAG_MISTAKE,        // 2     poor move (traditional "?")
    NAG_EXCELLENT_MOVE, // 3     very good move (traditional "!!")
    NAG_BLUNDER,        // 4     very poor move (traditional "??")
    NAG_INTERESTING_MOVE, // 5     speculative move (traditional "!?")
    NAG_DUBIOUS_MOVE,   // 6     questionable move (traditional "?!")
    NAG_ONLY_MOVE,      // 7     forced move (all others lose quickly)
    NAG_ONLY_MOVE,      // 8     singular move (no reasonable alternatives)
    NAG_BLUNDER,        // 9     worst move
    NAG_EVEN,           // 10    drawish position
    NAG_EVEN,           // 11    equal chances, quiet position
    NAG_EVEN,           // 12    equal chances, active position
    NAG_UNCLEAR,        // 13    unclear position
    NAG_WHITE_SLIGHT_ADV, // 14    White has a slight advantage
    NAG_BLACK_SLIGHT_ADV, // 15    Black has a slight advantage
    NAG_WHITE_ADV,      // 16    White has a moderate advantage
    NAG_BLACK_ADV,      // 17    Black has a moderate advantage
    NAG_WHITE_DECISIVE_ADV, // 18    White has a decisive advantage
    NAG_BLACK_DECISIVE_ADV, // 19    Black has a decisive advantage
    NAG_WHITE_DECISIVE_ADV, // 20    White has a crushing advantage (Black should resign)
    NAG_BLACK_DECISIVE_ADV, // 21    Black has a crushing advantage (White should resign)
    NAG_ZUGZWANG,       // 22    White is in zugzwang
    NAG_ZUGZWANG,       // 23    Black is in zugzwang
    NAG_SPACE_ADV,      // 24    White has a slight space advantage
    NAG_SPACE_ADV,      // 25    Black has a slight space advantage
    NAG_SPACE_ADV,      // 26    White has a moderate space advantage
    NAG_SPACE_ADV,      // 27    Black has a moderate space advantage
    NAG_SPACE_ADV,      // 28    White has a decisive space advantage
    NAG_SPACE_ADV,      // 29    Black has a decisive space advantage
    NAG_DEVELOPMENT_ADV, // 30    White has a slight time (development) advantage
    NAG_DEVELOPMENT_ADV, // 31    Black has a slight time (development) advantage
    NAG_DEVELOPMENT_ADV, // 32    White has a moderate time (development) advantage
    NAG_DEVELOPMENT_ADV, // 33    Black has a moderate time (development) advantage
    NAG_DEVELOPMENT_ADV, // 34    White has a decisive time (development) advantage
    NAG_DEVELOPMENT_ADV, // 35    Black has a decisive time (development) advantage
    NAG_WITH_INITIATIVE, // 36    White has the initiative
    NAG_WITH_INITIATIVE, // 37    Black has the initiative
    NAG_WITH_INITIATIVE, // 38    White has a lasting initiative
    NAG_WITH_INITIATIVE, // 39    Black has a lasting initiative
    NAG_WITH_ATTACK,    // 40    White has the attack
    NAG_WITH_ATTACK,    // 41    Black has the attack
    NAG_NONE,           // 42    White has insufficient compensation for material deficit
    NAG_NONE,           // 43    Black has insufficient compensation for material deficit
    NAG_COMP_FOR_MATERIAL, // 44    White has sufficient compensation for material deficit
    NAG_COMP_FOR_MATERIAL, // 45    Black has sufficient compensation for material deficit
    NAG_COMP_FOR_MATERIAL, // 46    White has more than adequate compensation for material deficit
    NAG_COMP_FOR_MATERIAL, // 47    Black has more than adequate compensation for material deficit
    NAG_CENTRE,         // 48    White has a slight center control advantage
    NAG_CENTRE,         // 49    Black has a slight center control advantage
    NAG_CENTRE,         // 50    White has a moderate center control advantage
    NAG_CENTRE,         // 51    Black has a moderate center control advantage
    NAG_CENTRE,         // 52    White has a decisive center control advantage
    NAG_CENTRE,         // 53    Black has a decisive center control advantage
    NAG_KINGSIDE,       // 54    White has a slight kingside control advantage
    NAG_KINGSIDE,       // 55    Black has a slight kingside control advantage
    NAG_KINGSIDE,       // 56    White has a moderate kingside control advantage
    NAG_KINGSIDE,       // 57    Black has a moderate kingside control advantage
    NAG_KINGSIDE,       // 58    White has a decisive kingside control advantage
    NAG_KINGSIDE,       // 59    Black has a decisive kingside control advantage
    NAG_QUEENSIDE,      // 60    White has a slight queenside control advantage
    NAG_QUEENSIDE,      // 61    Black has a slight queenside control advantage
    NAG_QUEENSIDE,      // 62    White has a moderate queenside control advantage
    NAG_QUEENSIDE,      // 63    Black has a moderate queenside control advantage
    NAG_QUEENSIDE,      // 64    White has a decisive queenside control advantage
    NAG_QUEENSIDE,      // 65    Black has a decisive queenside control advantage
    NAG_NONE,           // 66    White has a vulnerable first rank
    NAG_NONE,           // 67    Black has a vulnerable first rank
    NAG_NONE,           // 68    White has a well protected first rank
    NAG_NONE,           // 69    Black has a well protected first rank
    NAG_NONE,           // 70    White has a poorly protected king
    NAG_NONE,           // 71    Black has a poorly protected king
    NAG_NONE,           // 72    White has a well protected king
    NAG_NONE,           // 73    Black has a well protected king
    NAG_NONE,           // 74    White has a poorly placed king
    NAG_NONE,           // 75    Black has a poorly placed king
    NAG_NONE,           // 76    White has a well placed king
    NAG_NONE,           // 77    Black has a well placed king
    NAG_NONE,           // 78    White has a very weak pawn structure
    NAG_NONE,           // 79    Black has a very weak pawn structure
    NAG_NONE,           // 80    White has a moderately weak pawn structure
    NAG_NONE,           // 81    Black has a moderately weak pawn structure
    NAG_NONE,           // 82    White has a moderately strong pawn structure
    NAG_NONE,           // 83    Black has a moderately strong pawn structure
    NAG_NONE,           // 84    White has a very strong pawn structure
    NAG_NONE,           // 85    Black has a very strong pawn structure
    NAG_NONE,           // 86    White has poor knight placement
    NAG_NONE,           // 87    Black has poor knight placement
    NAG_NONE,           // 88    White has good knight placement
    NAG_NONE,           // 89    Black has good knight placement
    NAG_NONE,           // 90    White has poor bishop placement
    NAG_NONE,           // 91    Black has poor bishop placement
    NAG_NONE,           // 92    White has good bishop placement
    NAG_NONE,           // 93    Black has good bishop placement
    NAG_NONE,           // 84    White has poor rook placement
    NAG_NONE,           // 85    Black has poor rook placement
    NAG_NONE,           // 86    White has good rook placement
    NAG_NONE,           // 87    Black has good rook placement
    NAG_NONE,           // 98    White has poor queen placement
    NAG_NONE,           // 99    Black has poor queen placement
    NAG_NONE,           // 100   White has good queen placement
    NAG_NONE,           // 101   Black has good queen placement
    NAG_NONE,           // 102   White has poor piece coordination
    NAG_NONE,           // 103   Black has poor piece coordination
    NAG_NONE,           // 104   White has good piece coordination
    NAG_NONE,           // 105   Black has good piece coordination
    NAG_NONE,           // 106   White has played the opening very poorly
    NAG_NONE,           // 107   Black has played the opening very poorly
    NAG_NONE,           // 108   White has played the opening poorly
    NAG_NONE,           // 109   Black has played the opening poorly
    NAG_NONE,           // 110   White has played the opening well
    NAG_NONE,           // 111   Black has played the opening well
    NAG_NONE,           // 112   White has played the opening very well
    NAG_NONE,           // 113   Black has played the opening very well
    NAG_NONE,           // 114   White has played the middlegame very poorly
    NAG_NONE,           // 115   Black has played the middlegame very poorly
    NAG_NONE,           // 116   White has played the middlegame poorly
    NAG_NONE,           // 117   Black has played the middlegame poorly
    NAG_NONE,           // 118   White has played the middlegame well
    NAG_NONE,           // 119   Black has played the middlegame well
    NAG_NONE,           // 120   White has played the middlegame very well
    NAG_NONE,           // 121   Black has played the middlegame very well
    NAG_NONE,           // 122   White has played the ending very poorly
    NAG_NONE,           // 123   Black has played the ending very poorly
    NAG_NONE,           // 124   White has played the ending poorly
    NAG_NONE,           // 125   Black has played the ending poorly
    NAG_NONE,           // 126   White has played the ending well
    NAG_NONE,           // 127   Black has played the ending well
    NAG_NONE,           // 128   White has played the ending very well
    NAG_NONE,           // 129   Black has played the ending very well
    NAG_WITH_COUNTER_PLAY, // 130   White has slight counterplay
    NAG_WITH_COUNTER_PLAY, // 131   Black has slight counterplay
    NAG_WITH_COUNTER_PLAY, // 132   White has moderate counterplay
    NAG_WITH_COUNTER_PLAY, // 133   Black has moderate counterplay
    NAG_WITH_COUNTER_PLAY, // 134   White has decisive counterplay
    NAG_WITH_COUNTER_PLAY, // 135   Black has decisive counterplay
    NAG_TIME,           // 136   White has moderate time control pressure
    NAG_TIME,           // 137   Black has moderate time control pressure
    NAG_TIME,           // 138   White has severe time control pressure
    NAG_TIME,           // 139   Black has severe time control pressure
    // Non-standard NAGs (but quite commonly used)
    NAG_WITH_THE_IDEA,  // 140   With the idea
    NAG_NONE,           // 141   Aimed against
    NAG_BETTER_IS,      // 142   Better is
    NAG_WORSE_IS,       // 143   Worse is
    NAG_NONE,           // 144   Equivalent is
    NAG_EDITORIAL_COMMENT, // 145   Editorial Comment
    NAG_NOVELTY,        // 146   Novelty
    NAG_NONE,           // 147
    NAG_NONE,           // 148
    NAG_NONE,           // 149
    NAG_NONE,           // 150
    NAG_NONE,           // 151
    NAG_NONE,           // 152
    NAG_NONE,           // 153
    NAG_NONE,           // 154
    NAG_NONE,           // 155
    NAG_NONE,           // 156
    NAG_NONE,           // 157
    NAG_NONE,           // 158
    NAG_NONE,           // 159
    NAG_NONE,           // 160
    NAG_NONE,           // 161
    NAG_NONE,           // 162
    NAG_NONE,           // 163
    NAG_NONE,           // 164
    NAG_NONE,           // 165
    NAG_NONE,           // 166
    NAG_NONE,           // 167
    NAG_NONE,           // 168
    NAG_NONE,           // 169
    NAG_NONE,           // 170
    NAG_NONE,           // 171
    NAG_NONE,           // 172
    NAG_NONE,           // 173
    NAG_NONE,           // 174
    NAG_NONE,           // 175
    NAG_NONE,           // 176
    NAG_NONE,           // 177
    NAG_NONE,           // 178
    NAG_NONE,           // 179
    NAG_NONE,           // 180
    NAG_NONE,           // 181
    NAG_NONE,           // 182
    NAG_NONE,           // 183
    NAG_NONE,           // 184
    NAG_NONE,           // 185
    NAG_NONE,           // 186
    NAG_NONE,           // 187
    NAG_NONE,           // 188
    NAG_NONE,           // 189
    NAG_NONE,           // 190
    NAG_NONE,           // 191
    NAG_NONE,           // 192
    NAG_NONE,           // 193
    NAG_NONE,           // 194
    NAG_NONE,           // 195
    NAG_NONE,           // 196
    NAG_NONE,           // 197
    NAG_NONE,           // 198
    NAG_NONE,           // 199
    NAG_NONE,           // 200
    NAG_NONE,           // 201
    NAG_NONE,           // 202
    NAG_NONE,           // 203
    NAG_NONE,           // 204
    NAG_NONE,           // 205
    NAG_NONE,           // 206
    NAG_NONE,           // 207
    NAG_NONE,           // 208
    NAG_NONE,           // 209
    NAG_NONE,           // 210
    NAG_NONE,           // 211
    NAG_NONE,           // 212
    NAG_NONE,           // 213
    NAG_NONE,           // 214
    NAG_NONE,           // 215
    NAG_NONE,           // 216
    NAG_NONE,           // 217
    NAG_NONE,           // 218
    NAG_NONE,           // 219
    NAG_DIAGRAM,        // 220   Diagram
    NAG_DIAGRAM_FLIPPED, // 221   Diagram (flipped)
    NAG_NONE,           // 222
    NAG_NONE,           // 223
    NAG_NONE,           // 224
    NAG_NONE,           // 225
    NAG_NONE,           // 226
    NAG_NONE,           // 227
    NAG_NONE,           // 228
    NAG_NONE,           // 229
    NAG_NONE,           // 230
    NAG_NONE,           // 231
    NAG_NONE,           // 232
    NAG_NONE,           // 233
    NAG_NONE,           // 234
    NAG_NONE,           // 235
    NAG_NONE,           // 236
    NAG_NONE,           // 237
    NAG_SPACE_ADV,      // 238   Space Advantage
    NAG_FILE,           // 239   File
    NAG_DIAGONAL,       // 240   Diagonal
    NAG_CENTRE,         // 241   Centre
    NAG_KINGSIDE,       // 242   Kingside
    NAG_QUEENSIDE,      // 243   Queenside
    NAG_WEAK_POINT,     // 244   Weak Point
    NAG_ENDING,         // 245   Ending
    NAG_BISHOP_PAIR,    // 246   Bishop Pair
    NAG_OPP_COLOURED_BISHOP_PAIR, // 247   Opposite-coloured Bishop Pair
    NAG_SAME_COLOURED_BISHOP_PAIR, // 248   Same-coloured Bishop Pair
    NAG_UNITED_PAWNS,   // 249   Connected Pawns
    NAG_SEPARATED_PAWNS, // 250   Isolated Pawns
    NAG_DOUBLED_PAWNS,  // 251   Doubled Pawns
    NAG_PASSED_PAWN,    // 252   Passed Pawn
    NAG_PAWN_ADV,       // 253   Pawn Advantage
    NAG_WITH,           // 254   With
    NAG_WITHOUT         // 255   Without
};

Nag PgnDatabase::fromPgnNag(unsigned nag) {
    if (nag >= NUM_PGN_NAGS) {
        LOGWRN << "Nag value " << nag << " is out-of-range";
        return NAG_NONE;
    }

    return m_nagMap[nag];
}

unsigned PgnDatabase::toPgnNag(Nag nag) {
    for (unsigned i = 0; i < NUM_PGN_NAGS; i++)
        if (m_nagMap[i] == nag)
            return i;
    return 0;
}

bool PgnDatabase::initIndexManager() {
    if (m_indexManager.rootDir().empty())
        if (!m_indexManager.setRootDir(g_tempDir + PATHSEP + "pgnindex"))
            return false;

    return true;
}

PgnDatabase::PgnDatabase() :
    Database(),
    m_pgnFilename(),
    m_pgnFile(),
    m_indexFilename(),
    m_indexFile(),
    m_context(m_pgnFile),
    m_numGames(0) {

    if (!initIndexManager())
        throw ChessCoreException("Failed to initialise the index manager");
}

PgnDatabase::PgnDatabase(const string &filename, bool readOnly) :
    Database(),
    m_pgnFilename(),
    m_pgnFile(),
    m_indexFilename(),
    m_indexFile(),
    m_context(m_pgnFile),
    m_numGames(0) {

    if (!initIndexManager())
        throw ChessCoreException("Failed to initialise the index manager");

    open(filename, false);
}

PgnDatabase::~PgnDatabase() {
    close();
}

bool PgnDatabase::open(const string &filename, bool readOnly) {
    clearErrorMsg();

    if (m_isOpen)
        close();

    m_access = ACCESS_NONE;
    m_pgnFilename = filename;

    ios_base::openmode mode;
    bool exists = Util::fileExists(filename);
    bool canWrite = !readOnly && Util::canWrite(filename);

    if (exists) {
        if (canWrite) {
            mode = ios::binary | ios::in | ios::out | ios::app;
            m_access = ACCESS_READWRITE;
        } else {
            mode = ios::binary | ios::in;
            m_access = ACCESS_READONLY;
        }
    } else {
        if (readOnly) {
            DBERROR << "Database file does not exist";
            return false;
        }

        mode = ios::binary | ios::in | ios::out | ios::app;
        m_access = ACCESS_READWRITE;
    }

    m_pgnFile.open(m_pgnFilename.c_str(), mode);

    if (m_pgnFile.is_open()) {
        m_isOpen = true;
        m_context.restart();
        m_context.setLineNumber(1);
    } else {
        DBERROR << "Failed to open PGN database file: " << strerror(errno);
        m_access = ACCESS_NONE;
    }

    return m_isOpen;
}

bool PgnDatabase::close() {
    m_pgnFilename.clear();
    m_pgnFile.close();
    m_indexFilename.clear();
    m_indexFile.close();
    m_isOpen = false;
    m_access = ACCESS_NONE;
    return true;
}

bool PgnDatabase::hasValidIndex() {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return true;
    }

    if (!m_indexFile.is_open()) {
        if (!m_indexManager.getIndexFile(m_pgnFilename, m_indexFile, m_indexFilename)) {
            DBERROR << "Failed to get an index file for database";
            return false;
        }

        ASSERT(m_indexFile.is_open());
        LOGDBG << "PGN database '" << m_pgnFilename << "' is using index file '" << m_indexFilename << "'";
    }

    bool retval = false;
    m_numGames = 0;

    // The index file might already be valid
    if (Util::size(m_pgnFile) > 0 &&
        Util::size(m_indexFile) > 0 &&
        Util::modifyTime(m_indexFilename) >= Util::modifyTime(m_pgnFilename)) {
        // Determine number of games in database based on size of index file
        m_numGames = (unsigned)(Util::size(m_indexFile) / (uint64_t)(sizeof(uint64_t) + sizeof(uint32_t)));
        LOGINF << "PGN database '" << m_pgnFilename << "' already has a valid index file";
        retval = true;
    }

    return retval;
}

bool PgnDatabase::index(DATABASE_CALLBACK_FUNC callback, void *contextInfo) {
    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    // hasValidIndex() will set m_numGames and open index file if it succeeds
    if (hasValidIndex())
        return true;

    ASSERT(m_indexFile.is_open());

    bool retval = true;
    uint64_t totalSize = Util::size(m_pgnFile);
    m_pgnFile.seekg(0, ios::beg);

    if (!m_pgnFile.fail() && !m_pgnFile.bad()) {
        unsigned gameNum = 0;
        uint64_t offset;
        uint32_t linenum = 0;
        bool inHeader = false;

        while (retval) {
            offset = m_pgnFile.tellg();
            string line;
            getline(m_pgnFile, line);
            linenum++;

            if (!line.empty()) {
                if (line[0] == '[') {
                    if (!inHeader) {
                        // First header of game
                        gameNum++;
                        retval = writeIndex(gameNum, offset, linenum);

                        if (!retval)
                            break;

                        inHeader = true;
                        m_numGames++;

                        if (callback) {
                            float complete = static_cast<float> ((offset * 100) / totalSize);

                            if (!callback(m_numGames, complete, contextInfo)) {
                                DBERROR << "User cancelled indexing";
                                retval = false;
                            }
                        }
                    }

                    // else still in header section
                } else if (inHeader)
                    inHeader = false;

            } else if (m_pgnFile.eof()) {
                m_pgnFile.clear();
                break;
            }
        }
    } else {
        DBERROR << "Failed to seek to start of database file";
        retval = false;
    }

    if (retval) {
        LOGINF << "Database '" << m_pgnFilename << "' contains " << m_numGames << " games";
    } else {
        m_indexManager.deleteIndexFile(m_pgnFilename);
    }

    return retval;
}

bool PgnDatabase::readHeader(unsigned gameNum, GameHeader &gameHeader) {
    bool retval = true;
    string str;

    //LOGDBG << "gameNum=" << gameNum << ", m_numGames=" << m_numGames;

    clearErrorMsg();

    gameHeader.initHeader();
    gameHeader.setReadFail(true);

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    try {
        if (!isOpen()) {
            DBERROR << "PGN Database is not open";
            return false;
        }

        if (m_indexFile.is_open()) {
            // Using random-access
            if (gameNum < 1) {
                DBERROR << "Game number " << gameNum << " is out-of-range";
                return false;
            } else if (gameNum > m_numGames) {
                DBERROR << "Cannot read game header " << gameNum << " as there are only " <<
                    m_numGames << " games in the database";
                return false;
            }

            uint32_t linenum;

            if (!seekGameNum(gameNum, linenum))
                return false;

            m_context.flush();
            m_context.setLineNumber(linenum);
        }

        int token, tokenCount = 0;

        while ((token = m_context.lex()) > 0 && retval) {
            LOGVERBOSE << "linenum=" << m_context.lineNumber() << ", token=" << token;

            tokenCount++;

            if (IS_PGN_HEADER(token))
                retval = readRoster(m_context, token, gameHeader, m_errorMsg);
            else if (token != A_PGN_FEN)
                break;
        }

        if (tokenCount == 0)
            retval = false;
    } catch(ChessCoreException &e) {
        logerr("ChessCoreException: %s", e.what());
        DBERROR << e.what();
        retval = false;
    }

    gameHeader.setReadFail(!retval);

    return retval;
}

bool PgnDatabase::read(unsigned gameNum, Game &game) {
    bool retval = true;
    string str;

    //LOGDBG << "gameNum=" << gameNum << ", m_numGames=" << m_numGames;

    game.setReadFail(true);

    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access == ACCESS_NONE) {
        DBERROR << "Cannot read from this database";
        return false;
    }

    try {
        if (m_indexFile.is_open()) {
            // Random access
            if (gameNum < 1) {
                DBERROR << "Game number " << gameNum << " is out-of-range";
                return false;
            } else if (gameNum > m_numGames) {
                DBERROR << "Cannot read game " << gameNum << " as there are only " <<
                    m_numGames << " games in the database";
                return false;
            }

            uint32_t linenum;

            if (!seekGameNum(gameNum, linenum))
                return false;

            m_context.flush();
            m_context.setLineNumber(linenum);
        }

        retval = read(m_context, game, m_errorMsg);
    } catch(ChessCoreException &e) {
        logerr("ChessCoreException while reading game: %s", e.what());
        DBERROR << e.what();
        retval = false;
    }

    game.setReadFail(!retval);

    return retval;
}

bool PgnDatabase::write(unsigned gameNum, const Game &game) {
    uint64_t offset = m_pgnFile.tellg();

    //LOGDBG << "gameNum=" << gameNum << ", m_numGames=" << m_numGames;

    clearErrorMsg();

    if (!m_isOpen) {
        DBERROR << "Database is not open";
        return false;
    }

    if (m_access != ACCESS_READWRITE) {
        DBERROR << "Cannot write to this database";
        return false;
    }

    if (gameNum < 1) {
        DBERROR << "Game number " << gameNum << " is out-of-range";
        return false;
    }

    if (m_indexFile.is_open()) {
        // Random access
        if (gameNum != m_numGames + 1) {
            // Oh dear; we are going to need to move the file around lots in order
            // to insert the new game.
#if 0
            // Write the game into a temporary in-memory stream so we know its size
            stringstream ss;

            if (!write(ss, game, m_errorMsg)) {
                DBERROR << "Error writing to database: " << m_errorMsg;
                return false;
            }

            ss << endl;
            uint64_t gameSize = Util::size(ss); // Size of the game
            ss.seekg(0, ios::beg);      // Rewind game stream

            // Move the current games down
            m_pgnFile.sync();
            uint64_t databaseSize = Util::size(m_pgnFile);
            uint64_t gameOffset;
            uint32_t lineNumber;

            if (!readIndex(gameNum + 1, gameOffset, lineNumber))
                return false;

            uint64_t offset = databaseSize - gameSize;
            vector<char> buffer(gameSize);

            while (offset > gameOffset && !m_pgnFile.fail()) {
                uint64_t len = max(offset - gameOffset, gameSize);
                m_pgnFile.seekg(offset, ios::beg);
                m_pgnFile.read(&buffer[0], len);
                m_pgnFile.write(&buffer[0], len);
                offset -= len;
            }

            if (m_pgnFile.fail()) {
                DBERROR << "Failed to re-arrange PGN file (it is probably corrupt now): " << strerror(errno);
                return false;
            }

            // Write the new game
            m_pgnFile.seekg(gameOffset, ios::beg);
            m_pgnFile.write(ss.str().c_str(), gameSize);

            if (m_pgnFile.fail()) {
                DBERROR << "Failed to write to database: " << strerror(errno);
                return false;
            }

            // Move the index entries down
#else // !0
            DBERROR << "Not implemented";
            return false;
#endif // 0
        }
    } else {
        // Sequential access
        if (gameNum != m_numGames + 1) {
            DBERROR << "Games can only be written to the end of this database";
            return false;
        }
    }

    if (gameNum > 1) {
        // Add a blank line to separate this game from the previous one
        m_pgnFile << endl;
    }

    if (!write(m_pgnFile, game, m_errorMsg)) {
        DBERROR << "Error writing game: " << m_errorMsg;
        return false;
    }

    m_pgnFile.flush();

    // Write the offset of this game to the index file
    if (m_indexFile.is_open())
        if (!writeIndex(gameNum, offset, m_context.lineNumber()))
            return false;

    m_numGames++;
    return true;
}

bool PgnDatabase::readFromString(const string &input, Game &game) {
    istringstream iss(input);
    PgnScannerContext context(iss);
    string errorMsg;
    bool retval = read(context, game, errorMsg);

    if (!retval)
        LOGERR << "Failed to read game from string: " << errorMsg;

    return retval;
}

unsigned PgnDatabase::readMultiFromString(const string &input,
                                          vector<shared_ptr<Game> > &games,
                                          DATABASE_CALLBACK_FUNC callback, void *contextInfo) {
    istringstream iss(input);
    PgnScannerContext context(iss);

    string errorMsg;
    size_t totalSize = input.size();

    games.clear();
    unsigned numGames = 0;

    bool done = false;
    shared_ptr<Game> game;

    do {
        game.reset(new Game);

        if (read(context, *game, errorMsg)) {
            games.push_back(game);
            game = 0;
            numGames++;
            unsigned offset = (unsigned)iss.tellg();

            if (callback) {
                float complete = static_cast<float>((offset * 100) / totalSize);
                if (!callback(numGames, complete, contextInfo)) {
                    LOGERR << "User cancelled reading";
                    done = true;
                }
            }
        } else {
            done = true;
            LOGERR << "Failed to read game from string: " << errorMsg;
        }
    } while (!done);

    return (unsigned)games.size();
}

bool PgnDatabase::writeToString(const Game &game, string &output) {
    stringstream outss;
    string errorMsg;
    bool retval = write(outss, game, errorMsg);

    if (retval) {
        output = outss.str();
    } else {
        LOGERR << "Failed to write game to string: " << errorMsg;
    }

    return retval;
}

bool PgnDatabase::read(PgnScannerContext &context, Game &game, string &errorMsg) {
    bool retval = true;
    int token, tokenCount = 0;
    AnnotMove *lastMove = 0;
    string annotation, str;

    try {
        game.init();

        while ((token = context.lex()) > 0 && retval) {
            LOGVERBOSE << "linenum=" << context.lineNumber() << ", token=" << token;

            tokenCount++;

            if (token == A_PGN_FEN) {
                // This is in the class IS_PGN_HEADER, however the value must be stored
                // in a Game object, not a GameHeader object (which is all readRoster()
                // can work on).  It must therefore be handled specially.
                string data = getTagString(context, errorMsg);

                if (!data.empty()) {
                    if (game.setStartPosition(data) == Position::LEGAL) {
                        game.setPositionToStart();
                    } else {
                        errorMsg = Util::format("line %u: invalid FEN in header: '%s'",
                                                context.lineNumber(), data.c_str());
                        retval = false;
                    }
                } else {
                    errorMsg = Util::format("line %u: invalid FEN header", context.lineNumber());
                    retval = false;
                }
            } else if (IS_PGN_HEADER(token)) {
                retval = readRoster(context, token, game, errorMsg);
            } else if (IS_PGN_MOVENUM(token)) {
                unsigned moveNum = (unsigned)atoi(context.text());
                moveNum = toHalfMove(moveNum, token == A_WHITE_MOVENUM ? WHITE : BLACK);

                if (moveNum != game.nextPly()) {
                    errorMsg = Util::format("line %u: invalid move number '%s'; expected %u%s",
                                            context.lineNumber(), context.text(), toMove(game.nextPly()),
                                            (toColour(game.nextPly()) == WHITE ? "." : "..."));
                    retval = false;
                }
            } else if (IS_PGN_MOVE(token)) {
                // It is necessary to ask the for the 'game over' condition when making the
                // move in order to get the returned move to have the right flags (e.g mate).
                Game::GameOver gameOver = Game::GAMEOVER_NOT;
                lastMove = game.makeMove(context.text(), 0, 0, false, &gameOver, 0);

                if (lastMove == 0) {
                    errorMsg = Util::format("line %u: failed to make move '%s'",
                                            context.lineNumber(), context.text());
                    retval = false;
                } else if (!annotation.empty()) {
                    lastMove->setPreAnnot(annotation);
                    annotation.clear();
                }
            } else if (IS_PGN_RESULT(token)) {
                Game::Result rslt = Game::UNFINISHED;

                if (strcmp(context.text(), "1-0") == 0) {
                    rslt = Game::WHITE_WIN;
                } else if (strcmp(context.text(), "0-1") == 0) {
                    rslt = Game::BLACK_WIN;
                } else if (strcmp(context.text(), "1/2-1/2") == 0) {
                    rslt = Game::DRAW;
                } else if (strcmp(context.text(), "*") == 0) {
                    rslt = Game::UNFINISHED;
                } else {
                    errorMsg = Util::format("line %u: invalid result", context.lineNumber());
                    retval = false;
                }

                if (retval && rslt != game.result()) {
                    errorMsg = Util::format("line %u: result does not match result in header",
                                            context.lineNumber());
                    retval = false;
                }

                break; // End of game
            } else if (token == A_COMMENT || token == A_ROL_COMMENT) {
                const char *comment = context.text();

                if (token == A_ROL_COMMENT)
                    if (*comment == ';')
                        comment++;

                annotation = comment;
                Util::trim(annotation);

                if (lastMove) {
                    lastMove->setPostAnnot(annotation);
                    annotation.clear();
                } else {
                    //LOGDBG << "Latched annotation: '" << annotation << "'";
                }
            } else if (IS_PGN_EVAL(token)) {
                if (lastMove) {
                    Nag nag = NAG_NONE;
                    unsigned nagValue;

                    switch (token) {
                    case A_CHECK:
                        // Nothing
                        break;

                    case A_MATE:
                        // Nothing
                        break;

                    case A_GOOD_MOVE:
                        nag = NAG_GOOD_MOVE;
                        break;

                    case A_BAD_MOVE:
                        nag = NAG_MISTAKE;
                        break;

                    case A_INTERESTING_MOVE:
                        nag = NAG_INTERESTING_MOVE;
                        break;

                    case A_DUBIOUS_MOVE:
                        nag = NAG_DUBIOUS_MOVE;
                        break;

                    case A_BRILLIANT_MOVE:
                        nag = NAG_EXCELLENT_MOVE;
                        break;

                    case A_BLUNDER_MOVE:
                        nag = NAG_BLUNDER;
                        break;

                    case A_NAG:
                        if (Util::parse(context.text() + 1, nagValue)) {
                            nag = fromPgnNag(nagValue);
                        } else {
                            errorMsg = Util::format("line %u: invalid NAG value '%s'",
                                                    context.lineNumber(), context.text() + 1);
                            retval = false;
                        }
                        break;

                    case A_NAG_MATE:
                        // Nothing
                        break;

                    case A_NAG_NOVELTY:
                        nag = NAG_NOVELTY;
                        break;

                    default:
                        ASSERT(false);
                        break;
                    }

                    if (nag != NAG_NONE)
                        lastMove->addNag(nag);
                }
            } else if (token == A_VARSTART) {
                if (!game.startVariation()) {
                    str = Util::format("line %u: failed to start variation after move %s",
                                       context.lineNumber(), (lastMove ? lastMove->dump().c_str() : "none"));

                    if (m_relaxedParsing) {
                        // Consume the invalid variation and carry on
                        LOGWRN << str;
                        int varCount = 1;

                        while (varCount > 0 && (token = context.lex()) > 0) {
                            if (token == A_VARSTART)
                                varCount++;
                            else if (token == A_VAREND)
                                varCount--;
                        }

                        if (token == 0)
                            break;
                    } else {
                        errorMsg = str;
                        retval = false;
                    }
                }

                lastMove = 0;
            } else if (token == A_VAREND) {
                if (!game.endVariation()) {
                    errorMsg = Util::format("line %u: failed to end variation after move '%s'",
                                            context.lineNumber(), (lastMove != 0 ? lastMove->dump().c_str() : "none"));
                    retval = false;
                }
            } else {
                if (token == '{' || token == '}') {
                    str = Util::format("line %u:: broken comment (unmatched braces?)", context.lineNumber());
                } else {
                    if (isprint(token))
                        str = Util::format("line %u: spurious character '%c'", context.lineNumber(), (char)token);
                    else
                        str = Util::format("line %u: spurious character 0x%02x", context.lineNumber(), (unsigned)token);
                }

                if (m_relaxedParsing) {
                    // Keep calm and carry on
                    LOGWRN << str;
                } else {
                    errorMsg = str;
                    return false;
                }
            }
        }

        if (tokenCount == 0)
            retval = false;

    } catch(ChessCoreException &e) {
        logerr("ChessCoreException while reading game: %s", e.what());
        errorMsg = e.what();
        retval = false;
    }

    return retval;
}

bool PgnDatabase::readRoster(PgnScannerContext &context, int token, GameHeader &gameHeader, string &errorMsg) {
    unsigned d, m, y;
    unsigned major, minor;
    unsigned elo;

    errorMsg.clear();

    try {
        string data;

        data = getTagString(context, errorMsg);

        if (data.empty())
            return errorMsg.empty(); // Empty string could be an error

        if (data == "?")
            return true;
        else if (data == "*" && token != A_PGN_RESULT)
            return true;

        switch (token) {
        case A_PGN_EVENT:
            gameHeader.setEvent(data);
            break;

        case A_PGN_SITE:
            gameHeader.setSite(data);
            break;

        case A_PGN_DATE:
            // This is structured like this as a previous sscanf will leave half-parsed values
            // set in y, m or d.
            y = m = d = 0;

            if (sscanf(data.c_str(), "%u.%u.%u", &y, &m, &d) != 3) {
                y = m = d = 0;

                if (sscanf(data.c_str(), "%u.%u.??", &y, &m) != 2) {
                    y = m = d = 0;
                    sscanf(data.c_str(), "%u.??.??", &y);
                    // Ignore anything else
                }
            }

            gameHeader.setDay(d);
            gameHeader.setMonth(m);
            gameHeader.setYear(y);
            break;

        case A_PGN_ROUND:
            // This is structured like this as a previous sscanf will leave half-parsed values
            // set in major or minor.
            major = minor = 0;

            if (sscanf(data.c_str(), "%u.%u", &major, &minor) != 2) {
                major = minor = 0;

                if (sscanf(data.c_str(), "%u.?", &major) != 1) {
                    major = minor = 0;

                    if (sscanf(data.c_str(), "%u", &major) != 1) {
                        major = minor = 0;
                        sscanf(data.c_str(), "?.%u", &minor);
                        // Ignore anything else
                    }
                }
            }

            gameHeader.setRoundMajor(major);
            gameHeader.setRoundMinor(minor);
            break;

        case A_PGN_WHITE:
            gameHeader.white().setFormattedName(data);
            break;

        case A_PGN_BLACK:
            gameHeader.black().setFormattedName(data);
            break;

        case A_PGN_RESULT:
            if (data == "1-0") {
                gameHeader.setResult(Game::WHITE_WIN);
            } else if (data == "0-1") {
                gameHeader.setResult(Game::BLACK_WIN);
            } else if (data == "1/2-1/2") {
                gameHeader.setResult(Game::DRAW);
            } else if (data == "*") {
                gameHeader.setResult(Game::UNFINISHED);
            } else {
                errorMsg = Util::format("line %u: invalid result in header: '%s'",
                                        context.lineNumber(), data.c_str());
                return false;
            }

            break;

        case A_PGN_ANNOTATOR:
            gameHeader.setAnnotator(data);
            break;

        case A_PGN_ECO:
            gameHeader.setEco(data);
            break;

        case A_PGN_WHITEELO:
            elo = 0;

            if (sscanf(data.c_str(), "%u", &elo) != 1) {
                errorMsg = Util::format("line %u: invalid white elo in header: '%s'",
                                        context.lineNumber(), data.c_str());
                return false;
            }

            gameHeader.white().setElo(elo);
            break;

        case A_PGN_BLACKELO:
            elo = 0;

            if (sscanf(data.c_str(), "%u", &elo) != 1) {
                errorMsg = Util::format("line %u: invalid black elo in header: '%s'",
                                        context.lineNumber(), data.c_str());
                return false;
            }

            gameHeader.black().setElo(elo);
            break;

        // A_PGN_OPENING and A_PGN_VARIATION are really hacks used to create the
        // OpeningClassification.cfdb database from the eco.pgn file.
        case A_PGN_OPENING:
            setOpening(gameHeader.white(), data);
            break;

        case A_PGN_VARIATION:
            setOpening(gameHeader.black(), data);
            break;

        case A_PGN_TIMECONTROL:
            // Ignore any errors with time control
            gameHeader.timeControl().set(data, TimeControlPeriod::FORMAT_PGN);
            if (!gameHeader.timeControl().isValid()) {
                LOGWRN << "Line: " << context.lineNumber() << ": Failed to parse time control '" << data << "'";
            }
            break;

        default:
            break;
        }
    } catch(ChessCoreException &e) {
        errorMsg = e.what();
        return false;
    }

    return true;
}

bool PgnDatabase::write(ostream &output, const Game &game, string &errorMsg) {
    bool retval = true;
    string date, result;

    if (game.year() > 0) {
        if (game.month() > 0) {
            if (game.day() > 0)
                date = Util::format("%04u.%02u.%02u", game.year(), game.month(), game.day());
            else
                date = Util::format("%04u.%02u.??", game.year(), game.month());
        } else {
            date = Util::format("%04u.??.??", game.year());
        }
    } else {
        date = "????.??.??";
    }

    switch (game.result()) {
    case Game::WHITE_WIN:
        result = "1-0";
        break;

    case Game::BLACK_WIN:
        result = "0-1";
        break;

    case Game::DRAW:
        result = "1/2-1/2";
        break;

    default:
        result = "*";
        break;
    }

    output << Util::format("[Event \"%s\"]", formatTagString(game.event()).c_str()) << '\n';
    output << Util::format("[Site \"%s\"]", formatTagString(game.site()).c_str()) << '\n';
    output << Util::format("[Date \"%s\"]", date.c_str()) << '\n';

    if (game.roundMajor() && game.roundMinor())
        output << Util::format("[Round \"%u.%u\"]", game.roundMajor(), game.roundMinor()) << '\n';
    else if (game.roundMajor() && game.roundMinor() == 0)
        output << Util::format("[Round \"%u\"]", game.roundMajor()) << '\n';
    else if (game.roundMajor() == 0 && game.roundMinor())
        output << Util::format("[Round \"?.%u\"]", game.roundMinor()) << '\n';
    else
        output << "[Round \"?\"]" << '\n';

    output << Util::format("[White \"%s\"]",
                           formatTagString(game.white().formattedName()).c_str()) << '\n';
    output << Util::format("[Black \"%s\"]",
                           formatTagString(game.black().formattedName()).c_str()) << '\n';

    if (!game.startPosition().isStarting()) {
        output << "[SetUp \"1\"]" << endl;
        output << Util::format("[FEN \"%s\"]", game.startPositionFen().c_str()) << '\n';
    }

    output << Util::format("[Result \"%s\"]", result.c_str()) << '\n';

    if (!game.annotator().empty())
        output << Util::format("[Annotator \"%s\"]", game.annotator().c_str()) << '\n';

    if (!game.eco().empty())
        output << Util::format("[ECO \"%s\"]", game.eco().c_str()) << '\n';

    if (game.white().elo())
        output << Util::format("[WhiteElo \"%u\"]", game.white().elo()) << '\n';

    if (game.black().elo())
        output << Util::format("[BlackElo \"%u\"]", game.black().elo()) << '\n';

    if (game.timeControl().isValid()) {
        output << "[TimeControl \"" << game.timeControl().notation(TimeControlPeriod::FORMAT_PGN) << "\"\n";
    }
    output << endl;

    if (output.bad() || output.fail()) {
        LOGERR << "Failed to write PGN header to stream: " << strerror(errno);
        return false;
    }

    unsigned width = 0;

    if (game.mainline())
        retval = writeMoves(output, game.mainline(), width, errorMsg);

    writeText(output, result, width);
    output << endl;
    return retval;
}

bool PgnDatabase::writeMoves(ostream &output, const AnnotMove *amove, unsigned &width, string &errorMsg) {
    Position pos;
    UnmakeMoveInfo umi;

    vector<string> parts;
    const AnnotMove *m;
    string str;
    bool retval = true, moveNum = true, forceMoveNum = false, firstWord = true;
    unsigned i, numParts;

    ASSERT(amove);

    // There must be a priorPosition available for this line
    m = amove;

    while (m->mainline())
        m = m->mainline();

    ASSERT(m->priorPosition());
    pos.set(m->priorPosition());

    // We can write a 'pre-move annotation' at the start of each line
    if (amove) {
        const string &preAnnot = amove->preAnnot();

        if (!preAnnot.empty()) {
            writeText(output, "{", width, false);
            numParts = Util::splitLine(preAnnot, parts);

            for (i = 0; i < numParts; i++)
                writeText(output, parts[i], width, i > 0);

            writeText(output, "}", width, false);
            forceMoveNum = true;
            firstWord = false;
        }
    }

    while (amove && retval) {
        if (output.bad() || output.fail()) {
            LOGERR << "Failed to write moves to stream: " << strerror(errno);
            retval = false;
            break;
        }

        moveNum = toColour(pos.ply() + 1) == WHITE || amove->mainline();

        if (moveNum || forceMoveNum) {
            str = pos.moveNumber();
            writeText(output, str, width, !firstWord);
            firstWord = false;
        }

        str = amove->san(pos);
        writeText(output, str, width, !firstWord);
        firstWord = false;
        forceMoveNum = false;

        if (amove->nagCount() > 0) {
            Nag nags[STORED_NAGS];
            unsigned numNags = amove->nags(nags);
            ASSERT(numNags == amove->nagCount());

            for (i = 0; i < numNags; i++) {
                unsigned nagValue = toPgnNag(nags[i]);

                if (nagValue) {
                    str = Util::format("$%u", nagValue);
                    writeText(output, str, width);
                    forceMoveNum = true;
                }
            }
        }

        const string &postAnnot = amove->postAnnot();

        if (!postAnnot.empty()) {
            writeText(output, "{", width, true);
            numParts = Util::splitLine(postAnnot, parts);

            for (i = 0; i < numParts; i++)
                writeText(output, parts[i], width, i > 0);

            writeText(output, "}", width, false);
            forceMoveNum = true;
        }

        if (!pos.makeMove(amove->move(), umi)) {
            errorMsg = Util::format("Failed to make move '%s'", amove->dump(false).c_str());
            return false;
        }

        if (amove->variation()) {
            if (EMBEDDED_VARIATIONS) {
                writeText(output, "(", width, true);
                retval = writeMoves(output, amove->variation(), width, errorMsg);
                writeText(output, ")", width, false);
                forceMoveNum = true;
            } else if (amove->mainline() == 0) {
                // Top of variation tree
                for (m = amove->variation(); m; m = m->variation()) {
                    writeText(output, "(", width, true);
                    retval = writeMoves(output, m, width, errorMsg);
                    writeText(output, ")", width, false);
                }

                forceMoveNum = true;
            }
        }

        amove = amove->next();
    }

    return retval;
}

void PgnDatabase::writeText(ostream &output, const string &text, unsigned &width, bool insertSpace /*=true*/) {
    if (insertSpace && width > 0) {
        output << ' ';
        width++;
    }

    unsigned len = (unsigned)text.length();

    if (width + len > 79) {
        output << endl;
        width = 0;
    }

    output << text;
    width += len;
}

string PgnDatabase::getTagString(PgnScannerContext &context, string &errorMsg) {
    stringstream ss;
    char *s = context.text();

    // Find leading quote
    while (*s != '\0' && *s != '"')
        s++;

    if (*s == '\0')
        return "";

    s++;

    // Find end of contents
    while (*s != '\0' && *s != '"') {
        if (*s == '\\') {
            // Escaped quote or backslash (else ignore)
            if (*(s + 1) == '"' || *(s + 1) == '\\')
                ss << *(s + 1);

            s++;
        } else {
            ss << *s;
        }

        s++;
    }

    string tag = ss.str();

    if (*s != '"') {
        // This can occur if the end quote is escaped (\").  We can recover from it
        // by removing the backslash and anything after it
        auto it = tag.rfind("]");

        if (it != string::npos) {
            tag = tag.substr(0, it);
            logwrn("line %u: unmatched quotes in header (recovered)", context.lineNumber());
        } else {
            // Cannot recover from this error
            errorMsg = Util::format("line %u: unmatched quotes in header", context.lineNumber());
            return "";
        }
    }

    return Util::trim(const_cast<const string &> (tag));
}

string PgnDatabase::formatTagString(const string &str) {
    if (str.empty())
        return "?";

    stringstream ss;

    for (string::const_iterator it = str.begin(); it != str.end(); ++it) {
        uint8_t c = *it;

        if (c == '"' || c == '\\')
            ss << '\\';

        ss << c;
    }

    return Util::trim(ss.str());
}

void PgnDatabase::setOpening(Player &player, const string &data) {
    if (!player.firstNames().empty() ||
        !player.lastName().empty()) {
        //LOGDBG << "Ignoring Opening tag as player name is set";
        return;
    }

    // Capitalize words in opening string
    vector<string> words;
    unsigned numWords = Util::splitLine(data, words);

    if (numWords > 0) {
        for (auto it = words.begin(); it < words.end(); ++it) {
            string &word = *it;

            // Don't capitalise if the 1st character is a letter and the 2nd a number
            // (to avoid corrupting '4.e3 e8g8, 5.Nf3, Without ...d5')
            if (word.length() >= 2 && isalpha(word[0]) && isdigit(word[1]))
                continue;

            word[0] = toupper(word[0]);
        }

        string capitalized = Util::concat(words, 0, numWords);
        player.setLastName(capitalized);
    }
}

bool PgnDatabase::seekGameNum(unsigned gameNum, uint32_t &linenum) {
    ASSERT(gameNum > 0);
    uint64_t offset;

    if (!readIndex(gameNum, offset, linenum))
        return false;

    m_pgnFile.seekg(offset, ios::beg);

    if (m_pgnFile.fail() || m_pgnFile.bad()) {
        DBERROR << "Failed to seek to offset 0x" << hex << offset << " in PGN database file: " <<
            strerror(errno);
        return false;
    }

    return true;
}

bool PgnDatabase::readIndex(unsigned gameNum, uint64_t &offset, uint32_t &linenum) {
    clearErrorMsg();

    ASSERT(gameNum > 0);
    uint64_t indexOffset = (gameNum - 1) * (sizeof(uint64_t) + sizeof(uint32_t));
    m_indexFile.seekg(indexOffset, ios::beg);

    if (m_indexFile.fail() || m_indexFile.bad()) {
        DBERROR << "Failed to seek to offset 0x" << hex << indexOffset <<
            " in PGN index file: " << strerror(errno);
        return false;
    }

    uint64_t tempOffset;
    uint32_t tempLinenum;

    if (!StreamUtil<uint64_t>::read(m_indexFile, tempOffset) ||
        !StreamUtil<uint32_t>::read(m_indexFile, tempLinenum)) {
        DBERROR << "Failed to read index for game " << gameNum <<
            " from PGN index file: " << strerror(errno);
        return false;
    }

    offset = le64(tempOffset);
    linenum = le32(tempLinenum);

    if (linenum == 0) {
        DBERROR << "Got line number of 0 from index file for game << " << gameNum;
        return false;
    }

    return true;
}

bool PgnDatabase::writeIndex(unsigned gameNum, uint64_t offset, uint32_t linenum) {
    clearErrorMsg();

    ASSERT(gameNum > 0);
    ASSERT(linenum > 0);

    uint64_t indexOffset = (gameNum - 1) * (sizeof(uint64_t) + sizeof(uint32_t));
    m_indexFile.seekg(indexOffset, ios::beg);

    if (m_indexFile.fail() || m_indexFile.bad()) {
        DBERROR << "Failed to seek to offset 0x" << hex << indexOffset <<
            " in PGN index file: " << strerror(errno);
        return false;
    }

    if (!StreamUtil<uint64_t>::write(m_indexFile, le64(offset)) ||
        !StreamUtil<uint32_t>::write(m_indexFile, le32(linenum))) {
        DBERROR << "Failed to write index for game " << gameNum <<
            " to PGN index file: " << strerror(errno);
        return false;
    }

    m_indexFile.flush();

    return true;
}
    
}   // namespace ChessCore
