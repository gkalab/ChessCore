//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// CbhDatabase.cpp: ChessBase CBH Database class implementation.
//

#define VERBOSE_LOGGING 1

#include <ChessCore/CbhDatabase.h>
#include <ChessCore/Log.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <iomanip>

using namespace std;

namespace ChessCore {

// Database Factory
static shared_ptr<Database> databaseFactory(const string &dburl, bool readOnly) {
    shared_ptr<Database> db;
    if (Util::endsWith(dburl, ".cbh", false)) {
        db.reset(new CbhDatabase(dburl, readOnly));
    }
    return db;
}

static bool registered = Database::registerFactory(databaseFactory);

// ==========================================================================
// struct CbhHeader
// ==========================================================================
string CbhHeader::dump() const {
    ostringstream oss;

    oss <<
        "numGames=" << numGames;
    return oss.str();
}

// ==========================================================================
// struct CbhRecord
// ==========================================================================

string CbhRecord::dump() const {
    ostringstream oss;

    oss <<
        "flags=0x" << hex << (int)flags << dec <<
        ", cbgIndex=" << cbgIndex <<
        ", cbaIndex=" << cbaIndex <<
        ", cbpWhiteIndex=" << cbpWhiteIndex <<
        ", cbpBlackIndex=" << cbpBlackIndex <<
        ", cbtIndex=" << cbtIndex <<
        ", cbcIndex=" << cbcIndex <<
        ", cbsIndex=" << cbsIndex <<
        ", day/month/year=" << (int)day << "/" << (int)month << "/" << year <<
        ", result=0x" << hex << (int)result << dec <<
        ", roundMajor=" << (int)roundMajor <<
        ", roundMinor=" << (int)roundMinor <<
        ", whileElo=" << whiteElo <<
        ", blackElo=" << blackElo <<
        ", eco=" << eco <<
        ", partialGame=" << boolalpha << partialGame;
    return oss.str();
}

// ==========================================================================
// struct CbTreeHeader
// ==========================================================================

string CbTreeHeader::dump() const {
    ostringstream oss;

    oss <<
        "numRecords=" << numRecords <<
        ", rootRecord=0x" << hex << rootRecord << dec <<
        ", recordSize=" << recordSize <<
        ", firstDeleted=" << (int)firstDeleted <<
        ", existingRecords=" << existingRecords;
    return oss.str();
}

// ==========================================================================
// struct CbpRecord
// ==========================================================================

string CbpRecord::dump() const {
    ostringstream oss;

    oss <<
        "leftChild=" << leftChild <<
        ", rightChild=" << rightChild <<
        ", height=" << (int)height <<
        ", lastName='" << lastName << "'" <<
        ", firstName='" << firstName << "'" <<
        ", numGames=" << numGames <<
        ", firstGameIndex=" << firstGameIndex;
    return oss.str();
}

// ==========================================================================
// class CbhDatabase
// ==========================================================================

const char *CbhDatabase::m_classname = "CbhDatabase";

CbhDatabase::CbhDatabase():Database(), m_numGames(0), m_filename() {
}

CbhDatabase::CbhDatabase(const string &filename, bool readOnly) :
    Database(),
    m_numGames(0),
    m_filename() {
    open(filename, readOnly);
}

CbhDatabase::~CbhDatabase() {
    close();
}

bool CbhDatabase::open(const string &filename, bool readOnly) {
    clearErrorMsg();

    if (!Util::fileExists(filename)) {
        DBERROR << "Database does not exist";
        return false;
    }

    if (m_isOpen)
        close();

    ios_base::openmode mode = ios::binary | ios::in;     // Read-only support only

    m_cbhFile.open(filename.c_str(), mode);

    if (!m_cbhFile.is_open()) {
        DBERROR << "Failed to open CBH file '" << filename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    string otherFilename;
    Util::replaceExt(filename, ".cbh", otherFilename, ".cbg");
    m_cbgFile.open(otherFilename.c_str(), mode);

    if (!m_cbgFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    Util::replaceExt(filename, ".cbh", otherFilename, ".cba");
    m_cbaFile.open(otherFilename.c_str(), mode);

    if (!m_cbaFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    Util::replaceExt(filename, ".cbh", otherFilename, ".cbp");
    m_cbpFile.open(otherFilename.c_str(), mode);

    if (!m_cbpFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    Util::replaceExt(filename, ".cbh", otherFilename, ".cbt");
    m_cbtFile.open(otherFilename.c_str(), mode);

    if (!m_cbtFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    Util::replaceExt(filename, ".cbh", otherFilename, ".cbc");
    m_cbcFile.open(otherFilename.c_str(), mode);

    if (!m_cbcFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    Util::replaceExt(filename, ".cbh", otherFilename, ".cbs");
    m_cbsFile.open(otherFilename.c_str(), mode);

    if (!m_cbsFile.is_open()) {
        DBERROR << "Failed to open file '" << otherFilename << "': " << strerror(
            errno) << " (" << errno << ")";
        close();
        return false;
    }

    // Read the CBH header
    CbhHeader cbhHeader;

    if (!cbhReadHeader(cbhHeader)) {
        close();
        return false;
    }

    LOGVERBOSE << "cbhHeader: " << cbhHeader.dump();

    m_numGames = cbhHeader.numGames;

    // Read the CBP header
    if (!cbpReadHeader(m_cbpHeader)) {
        close();
        return false;
    }

    LOGVERBOSE << "cbpHeader: " << m_cbpHeader.dump();

    if (m_cbpHeader.recordSize != 58) {
        DBERROR << "Unsupported CBP record size " << m_cbpHeader.recordSize;
        close();
        return false;
    }

    m_filename = filename;
    m_isOpen = true;
    m_access = ACCESS_READONLY;

    return m_isOpen;
}

bool CbhDatabase::close() {
    m_cbhFile.close();
    m_cbgFile.close();
    m_cbaFile.close();
    m_cbpFile.close();
    m_cbtFile.close();
    m_cbcFile.close();
    m_cbsFile.close();

    m_filename.clear();

    m_isOpen = false;
    m_access = ACCESS_READONLY;
    return true;
}

bool CbhDatabase::readHeader(unsigned gameNum, GameHeader &gameHeader) {
    LOGDBG << "gameNum=" << gameNum;

    // Note: no gameHeader.initHeader() as the caller will have already done that!

    clearErrorMsg();

    CbhRecord cbhRecord;

    if (!cbhReadRecord(gameNum, cbhRecord))
        return false;

    LOGVERBOSE << "cbhRecord: " << cbhRecord.dump();

    CbpRecord cbpWhite, cbpBlack;

    if (!cbpReadRecord(cbhRecord.cbpWhiteIndex, cbpWhite) ||
        !cbpReadRecord(cbhRecord.cbpBlackIndex, cbpBlack))
        return false;

    LOGVERBOSE << "cbpWhite: " << cbpWhite.dump();
    LOGVERBOSE << "cbpBlack: " << cbpBlack.dump();

    return true;
}

bool CbhDatabase::read(unsigned gameNum, Game &game) {
    LOGDBG << "gameNum=" << gameNum;

    clearErrorMsg();

    game.init();

    if (!readHeader(gameNum, game))
        return false;

    return true;
}

bool CbhDatabase::write(unsigned gameNum, const Game &game) {
    DBERROR << "Writing to CBH databases is not supported";
    return false;
}

unsigned CbhDatabase::numGames() {
    return m_numGames;
}

unsigned CbhDatabase::firstGameNum() {
    return m_numGames > 0 ? 1 : 0;
}

unsigned CbhDatabase::lastGameNum() {
    return m_numGames;
}

bool CbhDatabase::gameExists(unsigned gameNum) {
    CbhRecord cbhRecord;

    if (!cbhReadRecord(gameNum, cbhRecord))
        return false;

    // Record is a game and it's not deleted
    return (cbhRecord.flags & 0x01) && (cbhRecord.flags & 0x08) == 0;
}

// CBH header (external)
#pragma pack(push,1)
struct CbhHeaderExt {
    uint8_t header[6]; // header
    uint8_t numGames[4]; // Number of games+1
    uint8_t dummy1[2]; // ?
    uint8_t dummy2[4]; // Can be 2
    uint8_t dummy3[4]; // Can be 3
    uint8_t dummy4[4]; // Can be 3
    uint8_t dummy5[16]; // ?
    uint8_t dummy6[4]; // Either number of games+1 (?), or 0 (?)
    uint8_t dummy7[2]; // ?
};
#pragma pack(pop)

bool CbhDatabase::cbhReadHeader(CbhHeader &cbhHeader) {
    ASSERT(sizeof(CbhHeaderExt) == 46);
    CbhHeaderExt ext;

    if (!readFile(m_cbhFile, "CBH", 0, &ext, sizeof(ext)))
        return false;

    cbhHeader.numGames = PackUtil<uint32_t>::big(ext.numGames, sizeof(ext.numGames)) - 1;

    return true;
}

// CBH record (external)
#pragma pack(push,1)
struct CbhRecordExt {
    uint8_t flags;            // flags
    uint8_t cbgIndex[4];      // Position in .cbg file where the game data starts (or guiding text)
    uint8_t cbaIndex[4];      // Position in .cba file where annotations start (0 = no annotations)
    uint8_t cbpWhiteIndex[3]; // White player number (0 = first player in .cbp file)
    uint8_t cbpBlackIndex[3]; // Black player number (0 = first player in .cbp file)
    uint8_t cbtIndex[3];      // Tournament number (0 = first tournament in .cbt file)
    uint8_t cbcIndex[3];      // Annotator number (0 = first annotator in .cbc file)
    uint8_t cbsIndex[3];      // Source number (0 = first source in .cbs file)
    uint8_t date[3];          // year & month & day
    uint8_t result[2];        // result
    uint8_t round;            // round
    uint8_t subround;         // subround
    uint8_t whiteElo[2];      // White ELO
    uint8_t blackElo[2];      // Black ELO
    uint8_t eco[2];           // ECO and Sub-ECO
    uint8_t medals[2];
    uint8_t critOrCorr;       // Contains critical position or correspondane header
    uint8_t multimediaFlags;
    uint8_t dummy1;           // ?
    uint8_t partialGameFlags; // Partial game
    uint8_t manyFlags;        // Contains many variations, etc.
    uint8_t mainlineMoves;    // Number of mainline moves
    uint8_t guidingTextInfo;  // Stuff about 'guiding text'
};
#pragma pack(pop)

bool CbhDatabase::cbhReadRecord(unsigned gameNum, CbhRecord &cbhRecord) {
    ASSERT(sizeof(CbhRecordExt) == 46);
    CbhRecordExt ext;

    if (!readFile(m_cbhFile, "CBH",
                  gameNum * sizeof(CbhRecordExt), &ext, sizeof(ext)))
        return false;

    cbhRecord.flags = ext.flags;

    cbhRecord.cbgIndex = PackUtil<uint32_t>::big(ext.cbgIndex, sizeof(ext.cbgIndex));
    cbhRecord.cbaIndex = PackUtil<uint32_t>::big(ext.cbaIndex, sizeof(ext.cbaIndex));
    cbhRecord.cbpWhiteIndex = PackUtil<uint32_t>::big(ext.cbpWhiteIndex, sizeof(ext.cbpWhiteIndex));
    cbhRecord.cbpBlackIndex = PackUtil<uint32_t>::big(ext.cbpBlackIndex, sizeof(ext.cbpBlackIndex));
    cbhRecord.cbtIndex = PackUtil<uint32_t>::big(ext.cbtIndex, sizeof(ext.cbtIndex));
    cbhRecord.cbcIndex = PackUtil<uint32_t>::big(ext.cbcIndex, sizeof(ext.cbcIndex));
    cbhRecord.cbsIndex = PackUtil<uint32_t>::big(ext.cbsIndex, sizeof(ext.cbsIndex));

    uint32_t date = PackUtil<uint32_t>::big(ext.date, sizeof(ext.date));
    cbhRecord.day = date & 0x1f;
    cbhRecord.month = (date >> 5) & 0x0f;
    cbhRecord.year = date >> 9;

    cbhRecord.result = PackUtil<uint16_t>::big(ext.result, sizeof(ext.result));
    cbhRecord.roundMajor = ext.round;
    cbhRecord.roundMinor = ext.subround;
    cbhRecord.whiteElo = PackUtil<uint16_t>::big(ext.whiteElo, sizeof(ext.whiteElo));
    cbhRecord.blackElo = PackUtil<uint16_t>::big(ext.blackElo, sizeof(ext.blackElo));
    cbhRecord.eco = PackUtil<uint16_t>::big(ext.eco, sizeof(ext.eco));
    cbhRecord.partialGame = (ext.partialGameFlags & 0x01) != 0;

    return true;
}

// Binary tree header (external)
#pragma pack(push,1)
struct CbTreeHeaderExt {
    uint8_t numRecords[4];
    uint8_t rootRecord[4];
    uint8_t dummy1[4];
    uint8_t recordSize[4];
    uint8_t firstDeleted[4];
    uint8_t existingRecords[4];
    uint8_t dummy2[4];
};
#pragma pack(pop)

bool CbhDatabase::cbpReadHeader(CbTreeHeader &cbpHeader) {
    ASSERT(sizeof(CbTreeHeaderExt) == 28);
    CbTreeHeaderExt ext;

    if (!readFile(m_cbpFile, "CBP", 0, &ext, sizeof(ext)))
        return false;

    cbpHeader.numRecords = PackUtil<uint32_t>::little(ext.numRecords, sizeof(ext.numRecords));
    cbpHeader.rootRecord = PackUtil<uint32_t>::little(ext.rootRecord, sizeof(ext.rootRecord));
    cbpHeader.recordSize = PackUtil<uint32_t>::little(ext.recordSize, sizeof(ext.recordSize));
    cbpHeader.firstDeleted = PackUtil<uint32_t>::little(ext.firstDeleted, sizeof(ext.firstDeleted));
    cbpHeader.existingRecords = PackUtil<uint32_t>::little(ext.existingRecords,
                                                           sizeof(ext.existingRecords));

    return true;
}

// CBP record (external)
#pragma pack(push,1)
struct CbpRecordExt {
    uint8_t leftChild[4];
    uint8_t rightChild[4];
    uint8_t height;
    uint8_t lastName[30];
    uint8_t firstName[20];
    uint8_t numGames[4];
    uint8_t firstGameIndex[4];
};
#pragma pack(pop)

bool CbhDatabase::cbpReadRecord(unsigned index, CbpRecord &cbpRecord) {
    ASSERT(sizeof(CbpRecordExt) == 67);
    CbpRecordExt ext;

    if (!readFile(m_cbpFile, "CBP",
                  sizeof(CbTreeHeaderExt) + (index * sizeof(CbpRecordExt)), &ext, sizeof(ext)))
        return false;

    cbpRecord.leftChild = PackUtil<uint32_t>::little(ext.leftChild, sizeof(ext.leftChild));
    cbpRecord.rightChild = PackUtil<uint32_t>::little(ext.rightChild, sizeof(ext.rightChild));
    cbpRecord.height = ext.height;
    cbpRecord.lastName.assign((const char *)ext.lastName,
                              strnlen((const char *)ext.lastName, sizeof(ext.lastName)));
    cbpRecord.firstName.assign((const char *)ext.firstName,
                               strnlen((const char *)ext.firstName, sizeof(ext.firstName)));
    cbpRecord.numGames = PackUtil<uint32_t>::little(ext.numGames, sizeof(ext.numGames));
    cbpRecord.firstGameIndex = PackUtil<uint32_t>::little(ext.firstGameIndex,
                                                          sizeof(ext.firstGameIndex));

    return true;
}

bool CbhDatabase::readFile(fstream &file, const char *filetype, unsigned offset, void *buffer, unsigned len) {
    file.seekg(offset, ios::beg);

    if (!file.good()) {
        DBERROR << "Failed to seek to offset 0x" << hex << offset <<
            " in " << filetype << " file of database '" << m_filename << "': " <<
            strerror(errno) << " (" << errno << ")";
        return false;
    }

    file.read((char *)buffer, len);

    if (!file.good()) {
        DBERROR << "Failed to read " << dec << len << " bytes from offset 0x" <<
            hex << offset << " of " << filetype << " file of database '" << m_filename << "': " <<
            strerror(errno) << " (" << errno << ")";
        return false;
    }

    return true;
}
}   // namespace ChessCore
