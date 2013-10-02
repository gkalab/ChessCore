//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Functions.cpp: Special-purpose functions.
//

#include "ccore.h"
#include <ChessCore/Database.h>
#include <ChessCore/PgnDatabase.h>
#include <ChessCore/OpeningTree.h>
#include <ChessCore/Epd.h>
#include <ChessCore/Log.h>
#include <ChessCore/Rand64.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>


using namespace std;
using namespace ChessCore;

static uint64_t perft(const Position &pos, unsigned depth, bool printMoves);
static void posDump(const Position &pos, unsigned depth);
static bool indexCallback(unsigned gameNum, float percentComplete, void *contextInfo);
static bool treeCallback(unsigned gameNum, float percentComplete, void *contextInfo);
static bool searchCallback(unsigned gameNum, float percentComplete, void *contextInfo);

static const char *m_classname = "";

//
// Generate random numbers.
//
bool funcRandom(bool cstyle) {
    unsigned column = 0;
    int i, j;
    unique_ptr<uint64_t []> numbers(new uint64_t[g_optNumber1]);

    for (i = 0; i < g_optNumber1; i++) {
        uint64_t r = Rand64::rand();

        for (j = 0; j < i; j++)
            if (r == numbers[j]) {
                cerr << "Duplicate random number generated!" << endl;
                return false;
            }

        numbers[i] = r;
    }

    if (cstyle)
        cout << "uint64_t random_numbers[" << g_optNumber1 << "] =" << endl << "{" << endl;

    for (i = 0; i < g_optNumber1; i++) {
        if (cstyle) {
            if (column == 0)
                cout << "    ";

            cout << "0x" << hex << setfill('0') << setw(16) << numbers[i];

            if (i < g_optNumber1 - 1)
                cout << ", ";

            if (++column == 4) {
                cout << endl;
                column = 0;
            }
        } else {
            cout << dec << numbers[i] << endl;
        }
    }

    if (cstyle)
        cout << endl << "};" << endl;

    return true;
}

//
// Generate random positions
//
bool funcRandomPositions() {
    int num = 1;
    if (g_optNumber1 > 0)
        num = g_optNumber1;
    Position pos;
    for (int i = 0; i < num; i++) {
        pos.setRandom();
        string fen = pos.fen();
        cout << fen << endl;
    }
    return true;
}

//
// Generate EPD file from a PGN file.
//
bool funcMakeEpd() {
    Game game;
    Position pos;
    UnmakeMoveInfo umi;
    int epdsWritten = 0;
    unsigned gameNum, lastGameNum;
    ofstream epdout;
    string san;
    bool retval = true;

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    if (g_optEpdFile.empty()) {
        cerr << "No EPD output file specified" << endl;
        return false;
    }

    cout << "Generating EPD file '" << g_optEpdFile << "'" << endl;

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << indb->errorMsg() << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << indb->errorMsg() << endl;
        return false;
    }

    epdout.open(g_optEpdFile.c_str(), ios::out);
    if (epdout.is_open()) {
        cerr << "Failed to open EPD file '" << g_optEpdFile << endl;
        return false;
    }

    gameNum = indb->firstGameNum();
    lastGameNum = indb->lastGameNum();

    while (gameNum <= lastGameNum && retval && !g_quitFlag) {
        if (!indb->gameExists(gameNum)) {
            cout << "Game " << gameNum << " does not exist" << endl;
            gameNum++;
            continue;
        }

        retval = indb->read(gameNum, game);

        if (!retval) {
            cerr << "Failed to read game " << gameNum << ": " << indb->errorMsg() << endl;
            retval = false;
            break;
        }

        if ((gameNum % 1000) == 0)
            cout << "Read game " << gameNum << endl;

        pos = game.startPosition();

        for (const AnnotMove *amove = game.mainline(); amove; amove = amove->next()) {
            EpdOp::Eval eval;
            Nag nags[4];
            amove->nags(nags);

            for (unsigned i = 0; i < 4 && nags[i] != NAG_NONE; i++) {
                switch (nags[i]) {
                case NAG_WHITE_SLIGHT_ADV:
                    eval = EpdOp::EVAL_W_SLIGHT_ADV;
                    break;
                case NAG_BLACK_SLIGHT_ADV:
                    eval = EpdOp::EVAL_B_SLIGHT_ADV;
                    break;
                case NAG_WHITE_ADV:
                    eval = EpdOp::EVAL_W_CLEAR_ADV;
                    break;
                case NAG_EVEN:
                case NAG_UNCLEAR:
                    eval = EpdOp::EVAL_EQUAL;
                    break;
                case NAG_BLACK_ADV:
                    eval = EpdOp::EVAL_B_CLEAR_ADV;
                    break;
                case NAG_WHITE_DECISIVE_ADV:
                    eval = EpdOp::EVAL_W_DECISIVE_ADV;
                    break;
                case NAG_BLACK_DECISIVE_ADV:
                    eval = EpdOp::EVAL_B_DECISIVE_ADV;
                    break;
                default:
                    eval = EpdOp::EVAL_NONE;
                    break;
                }

                if (eval != EpdOp::EVAL_NONE) {
                    // We want this position
                    san = pos.moveNumber() + amove->san(pos);

                    if (!pos.makeMove(amove->move(), umi)) {
                        cerr << "Failed to make move " << amove << endl;
                        retval = false;
                    }

                    epdout << pos.fen(true);
                    epdout << " eval ";
                    epdout << EpdOp::formatEval(eval);
                    epdout << "; id \"";

                    if (game.white().hasName() && game.black().hasName()) {
                        // Use details from game
                        epdout << game.white().formattedName() << "-" << game.black().formattedName();

                        if (game.hasEvent())
                            epdout << "," << game.event();

                        if (game.hasSite())
                            epdout << "," << game.site();

                        if (game.year() > 0)
                            epdout << "," << dec << game.year();
                    } else {
                        // Use input file details
                        epdout << g_optInputDb << ":" << gameNum;
                    }

                    if (game.hasAnnotator() && game.annotator() != "RR")
                        epdout << " [" << game.annotator() << "]";

                    epdout << " after " << san << "\";";
                    epdout << endl;
                    epdsWritten++;
                } else if (!pos.makeMove(amove->move(), umi)) {
                    cerr << "Failed to make move" << amove << endl;
                    retval = false;
                }
            }
        }

        gameNum++;
    }

    epdout.close();
    indb->close();

    if (retval)
        cout << "Created EPD file '" << g_optEpdFile << "' successfully (Number of EPDs: "
             << epdsWritten << ")" << endl;
    else
        cerr << "Failed to create EPD file '" << g_optEpdFile << "' see logfile" << endl;

    return retval;
}

//
// Validate a database.
//
bool funcValidateDb() {
    Game game;
    unsigned gameNum, firstGame = 0, lastGame = 0, dotFileIndex = 1;
    unsigned gameCount = 0, startTime, endTime;

    bool retval = true;

    PgnDatabase::setRelaxedParsing(true);

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    cout << "Validating database '" << g_optInputDb << "' games " << firstGame << "-" << lastGame <<
        endl;

    startTime = Util::getTickCount();

    for (gameNum = firstGame; gameNum <= lastGame && retval && !g_quitFlag; gameNum++) {
        if (!indb->gameExists(gameNum)) {
            cout << "Ignoring game " << gameNum << " as it does not exist" << endl;
            continue;
        }

        retval = indb->read(gameNum, game);

        if (retval) {
            if ((gameNum % 1000) == 0)
                cout << "Read game " << gameNum << endl;

            // Dump the final game tree to .dot file
            if (!g_optDotDir.empty()) {
                string dotFileName = Util::format("%s/game_%08u.dot", g_optDotDir.c_str(), dotFileIndex++);

                if (!AnnotMove::writeToDotFile(game.mainline(), dotFileName))
                    cerr << "Failed to write game tree to file '" << dotFileName << "'" << endl;
            }
        } else {
            cerr << "Failed to read game " << gameNum << ": " << indb->errorMsg() << endl;
        }

        gameCount++;
    }

    endTime = Util::getTickCount();

    indb->close();

    if (g_quitFlag)
        cout << "Validation aborted" << endl;
    else {
        if (retval) {
            unsigned elapsed = endTime - startTime;
            cout << "Database is valid. " << gameCount << " games in " << elapsed << "mS";
            // Write to logfile as well, for the benefit of test/scripts/dbtest.py
            LOGINF << "Database is valid. " << gameCount << " games in " << elapsed << "mS";

            if (elapsed)
                cout << " (" << (gameCount * 1000) / elapsed << " games/s)";

            cout << endl;
        } else {
            cout << "Database is invalid" << endl;
        }
    }

    return retval;
}

//
// Copy a database.
//
bool funcCopyDb() {
    Game game;
    unsigned inGameNum, outGameNum, firstGame = 0, lastGame = 0, dotFileIndex = 1;
    unsigned gameCount = 0, startTime, endTime;
    bool retval = true;

    PgnDatabase::setRelaxedParsing(true);

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    if (g_optOutputDb.empty()) {
        cerr << "No output database specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    shared_ptr<Database> outdb = Database::openDatabase(g_optOutputDb, false);
    if (!outdb) {
        cerr << "Don't know how to create database '" << g_optOutputDb << "'" << endl;
        return false;
    } else if (!outdb->isOpen()) {
        cerr << "Failed to create '" << g_optOutputDb << "': " << outdb->errorMsg() << endl;
        return false;
    }

    if (outdb->needsIndexing() &&
        !outdb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optOutputDb << "': " << outdb->errorMsg() << endl;
        return false;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    cout << "Copying database '" << g_optInputDb << "' games " << firstGame << "-" << lastGame <<
        " to database '" << g_optOutputDb << "' (which already contains " << outdb->numGames() << " games)"
         << endl;

    startTime = Util::getTickCount();

    for (inGameNum = firstGame, outGameNum = outdb->lastGameNum() + 1;
         inGameNum <= lastGame && retval && !g_quitFlag;
         inGameNum++, outGameNum++) {
        if (!indb->gameExists(inGameNum)) {
            cout << "Game " << inGameNum << " does not exist" << endl;
            continue;
        }

        retval = indb->read(inGameNum, game);

        if (retval) {
            retval = outdb->write(outGameNum, game);

            if (retval) {

                if ((inGameNum % 1000) == 0)
                    cout << "Copied game " << inGameNum << endl;

                // Dump the final game tree to .dot file
                if (!g_optDotDir.empty()) {
                    string dotFileName = Util::format("%s/game_%08u.dot", g_optDotDir.c_str(), dotFileIndex++);

                    if (!AnnotMove::writeToDotFile(game.mainline(), dotFileName))
                        cerr << "Failed to write game tree to file '" << dotFileName << "'" << endl;
                }
            } else {
                cerr << "Failed to write game " << outGameNum << ": " << outdb->errorMsg() << endl;

                if (g_optRelaxed)
                    retval = true;    // Ignore it

            }
        } else {
            cerr << "Failed to read game " << inGameNum << ": " << indb->errorMsg() << endl;

            if (g_optRelaxed)
                retval = true;    // Ignore it

        }

        gameCount++;
    }

    endTime = Util::getTickCount();

    if (g_quitFlag)
        cout << "Copying aborted" << endl;
    else {
        if (retval) {
            unsigned elapsed = endTime - startTime;
            cout << "Successfully copied database. " << gameCount << " games in " << elapsed << "mS";
            // Write to logfile as well, for the benefit of test/scripts/dbtest.py
            LOGINF << "Successfully copied database. " << gameCount << " games in " << elapsed << "mS";

            if (elapsed)
                cout << " (" << (gameCount * 1000) / elapsed << " games/s)";

            cout << endl;
        } else {
            cout << "Failed to copy database" << endl;
        }
    }

    outdb->close();
    indb->close();

    return retval;
}

//
// Build the opening tree in a database
//
bool funcBuildOpeningTree() {
    Game game;
    unsigned gameNum, firstGame = 0, lastGame = 0;
    unsigned gameCount = 0, startTime, endTime;

    bool retval = true;

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (!indb->supportsOpeningTree()) {
        cerr << "Database '" << g_optInputDb << "' does not support opening trees" << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    if (g_optDepth <= 0)
        g_optDepth = 50;

    if (g_optNumber1 == 0) {
        cout << "Building opening tree for whole database '" << g_optInputDb << "'" << endl;
        startTime = Util::getTickCount();

        if (!indb->buildOpeningTree(0, g_optDepth, treeCallback, NULL)) {
            cerr << "Failed to build opening tree: " << indb->errorMsg() << endl;
            return false;
        }

        endTime = Util::getTickCount();

        cout << "Opening Tree successfully built in " << endTime - startTime << "mS" << endl;

        return true;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    cout << "Building opening tree for database '" << g_optInputDb << "' games " << firstGame << "-" <<
        lastGame << endl;

    startTime = Util::getTickCount();

    for (gameNum = firstGame; gameNum <= lastGame && retval && !g_quitFlag; gameNum++) {
        if (!indb->gameExists(gameNum)) {
            cout << "Game " << gameNum << " does not exist" << endl;
            continue;
        }

        retval = indb->buildOpeningTree(gameNum, g_optDepth, NULL, NULL);

        if (retval) {
            // Call the callback function ourselves
            float complete = static_cast<float> ((gameNum * 100) / lastGame);
            treeCallback(gameNum, complete, NULL);
        } else {
            cerr << "Failed to build the Opening Tree for game " << gameNum << ": " << indb->errorMsg() << endl;
        }

        gameCount++;
    }

    endTime = Util::getTickCount();

    indb->close();

    if (g_quitFlag)
        cout << "Opening Tree building aborted" << endl;
    else {
        if (retval) {
            unsigned elapsed = endTime - startTime;
            cout << "Opening Tree successfully built. " << gameCount << " games in " << elapsed << "mS";

            if (elapsed)
                cout << " (" << (gameCount * 1000) / elapsed << " games/s)";

            cout << endl;
        } else {
            cout << "Error building opening tree" << endl;
        }
    }

    return retval;
}

//
// Classify games by ECO code
//
bool funcClassify() {
    Game game;
    unsigned gameNum, firstGame = 0, lastGame = 0;
    unsigned gameCount = 0, startTime, endTime;
    unsigned match = 0, mismatch = 0;

    bool retval = true;

    PgnDatabase::setRelaxedParsing(true);

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    if (g_optEcoFile.empty()) {
        cerr << "No ECO classification file specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    OpeningTree optree(g_optEcoFile);

    if (!optree.isOpen()) {
        cerr << "Failed to open ECO Classification file '" << g_optEcoFile << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    cout << "Classifying database '" << g_optInputDb << "' games " << firstGame << "-" << lastGame <<
        endl;

    startTime = Util::getTickCount();

    for (gameNum = firstGame; gameNum <= lastGame && retval && !g_quitFlag; gameNum++) {
        if (!indb->gameExists(gameNum)) {
            cout << "Game " << gameNum << " does not exist" << endl;
            continue;
        }

        retval = indb->read(gameNum, game);

        if (retval) {
            if (game.isPartialGame() || game.mainline() == 0)
                continue;    // Cannot classify

            if ((gameNum % 1000) == 0)
                cout << "Read game " << gameNum << endl;

            string eco, opening, variation;

            if (optree.classify(game, eco, opening, variation)) {
                if (eco == game.eco()) {
                    cout << "Game " << gameNum << " is " << eco << " (match) " << opening << " " << variation << endl;
                    match++;
                } else {
                    cout << "Game " << gameNum << " is " << eco << " (mismatch " << game.eco() << ") " << opening << " "
                         << variation << endl;
                    mismatch++;
                }

                // Update the game in the database if possible
                if (indb->access() == Database::ACCESS_READWRITE &&
                    !indb->write(gameNum, game)) {
                    cerr << "Failed to update game " << gameNum << ": " << indb->errorMsg() << endl;
                    retval = false;
                }
            } else {
                cerr << "Failed to classify game " << gameNum << endl;
            }
        } else {
            cerr << "Failed to read game " << gameNum << ": " << indb->errorMsg() << endl;
            retval = false;
        }

        gameCount++;
    }

    endTime = Util::getTickCount();

    indb->close();

    if (g_quitFlag)
        cout << "Classification aborted" << endl;
    else {
        if (retval) {
            unsigned elapsed = endTime - startTime;
            cout << "Classification succeeded. " << gameCount << " games in " << elapsed << "mS";

            if (elapsed)
                cout << " (" << (gameCount * 1000) / elapsed << " games/s)";

            cout << endl;
            cout << match << " matches, " << mismatch << " mis-matches" << endl;
        } else {
            cout << "Failed to classify database" << endl;
        }
    }

    return retval;
}

//
// Get PGN index values (offsets and line numbers).
//
bool funcPgnIndex() {
    Game game;
    unsigned gameNum, firstGame = 0, lastGame = 0;
    bool retval = true;

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }


    if (!indb->needsIndexing()) {
        cerr << "This database doesn't support indexing" << endl;
        return false;
    }

    if (!indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    if (g_optNumber1 <= 0)
        firstGame = indb->firstGameNum();
    else
        firstGame = (unsigned)g_optNumber1;

    if (g_optNumber2 <= 0)
        lastGame = indb->lastGameNum();
    else
        lastGame = g_optNumber2;

    if (firstGame > lastGame) {
        cerr << "Invalid game numbers specified" << endl;
        return false;
    }

    PgnDatabase *pgnDb = dynamic_cast<PgnDatabase *>(indb.get());
    for (gameNum = firstGame; gameNum <= lastGame && retval && !g_quitFlag; gameNum++) {

        if (!indb->gameExists(gameNum)) {
            cout << "Game " << gameNum << " does not exist" << endl;
            continue;
        }

        uint64_t offset;
        uint32_t linenum;

        if (pgnDb->readIndex(gameNum, offset, linenum)) {
            cout << "game " << dec << gameNum << " offset=0x" << hex << offset << " linenum=" << dec << linenum
                 << endl;
        } else {
            cerr << "Failed to get index info for game " << dec << gameNum << ": " << indb->errorMsg() << endl;
            retval = false;
            break;
        }
    }

    indb->close();

    return retval;
}

//
// Search a database.
//
bool funcSearchDb() {
    PgnDatabase::setRelaxedParsing(true);

    if (g_optInputDb.empty()) {
        cerr << "No input database specified" << endl;
        return false;
    }

    shared_ptr<Database> indb = Database::openDatabase(g_optInputDb, false);
    if (!indb) {
        cerr << "Don't know how to open database '" << g_optInputDb << "'" << endl;
        return false;
    } else if (!indb->isOpen()) {
        cerr << "Failed to open database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }


    if (!indb->supportsSearching()) {
        cerr << "Database '" << g_optInputDb << "' doesn't support searching" << endl;
        return false;
    }

    if (indb->needsIndexing() &&
        !indb->index(indexCallback, NULL)) {
        cerr << "Failed to index database '" << g_optInputDb << "': " << indb->errorMsg() << endl;
        return false;
    }

    if (indb->numGames() == 0) {
        cerr << "Database '" << g_optInputDb << "' is empty" << endl;
        return false;
    }

    DatabaseSearchCriteria searchCriteria;
    DatabaseSortCriteria sortCriteria;
    unsigned offset, limit;

    while (!g_quitFlag && !cin.eof()) {
        searchCriteria.clear();
        sortCriteria.clear();
        offset = 0;
        limit = 0;

        cout << "----------------------------------------------------------------------------------------------------"
             << endl;

        cout << "Enter search criteria and finish with a blank line (or 'quit' to end)" << endl;
        cout << "Search criteria: field comparison value" << endl;
        cout << "Where field is 'whiteplayer', 'blackplayer', 'player', 'event', 'site', 'date' or 'eco'" <<
            endl;
        cout << "      comparison is 'equals', 'startswith' or 'contains' " << endl;
        cout << "      (add 'ci_' prefix for case-insensitive match)" << endl;

        string line;
        vector<string> parts;

        while (!g_quitFlag && !cin.eof()) {
            getline(cin, line);

            if (line.empty())
                break;
            else if (line == "quit")
                return true;

            Util::splitLine(line, parts);

            if (parts.size() != 3) {
                cerr << "Enter 'field comparison value', a blank line or 'quit'" << endl;
                continue;
            }

            DatabaseSearchDescriptor descriptor;

            if (parts[0] == "whiteplayer")
                descriptor.field = DATABASE_FIELD_WHITEPLAYER;
            else if (parts[0] == "blackplayer")
                descriptor.field = DATABASE_FIELD_BLACKPLAYER;
            else if (parts[0] == "player")
                descriptor.field = DATABASE_FIELD_PLAYER;
            else if (parts[0] == "event")
                descriptor.field = DATABASE_FIELD_EVENT;
            else if (parts[0] == "site")
                descriptor.field = DATABASE_FIELD_SITE;
            else if (parts[0] == "date")
                descriptor.field = DATABASE_FIELD_DATE;
            else if (parts[0] == "eco")
                descriptor.field = DATABASE_FIELD_ECO;
            else {
                cerr << "Invalid field '" << parts[0] << "'" << endl;
                continue;
            }

            if (parts[1] == "equals")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_EQUALS, DATABASE_COMPARE_NONE);
            else if (parts[1] == "startswith")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_STARTSWITH, DATABASE_COMPARE_NONE);
            else if (parts[1] == "contains")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_CONTAINS, DATABASE_COMPARE_NONE);
            else if (parts[1] == "ci_equals")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_EQUALS,
                                                           DATABASE_COMPARE_CASE_INSENSITIVE);
            else if (parts[1] == "ci_startswith")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_STARTSWITH,
                                                           DATABASE_COMPARE_CASE_INSENSITIVE);
            else if (parts[1] == "ci_contains")
                descriptor.comparison = databaseComparison(DATABASE_COMPARE_CONTAINS,
                                                           DATABASE_COMPARE_CASE_INSENSITIVE);
            else {
                cerr << "Invalid comparison '" << parts[1] << "'" << endl;
                continue;
            }

            if (parts[2].empty()) {
                cerr << "Empty value" << endl;
                continue;
            }

            descriptor.value = parts[2];
            searchCriteria.push_back(descriptor);
        }

        if (cin.eof())
            return true;

        cout << "Enter sort criteria and finish with a blank line (or 'quit' to end)" << endl;
        cout << "Sort Criterial: field order" << endl;
        cout << "Where field is 'gamenum', 'whiteplayer', 'blackplayer', 'event', 'site', 'round', 'date' or 'result'"
             << endl;
        cout << "      order is 'asc' or 'desc'" << endl;

        while (g_quitFlag && !cin.eof()) {
            getline(cin, line);

            if (line.empty())
                break;
            else if (line == "quit")
                return true;

            Util::splitLine(line, parts);

            if (parts.size() != 2) {
                cerr << "Entry 'field order', a blank line or 'quit'" << endl;
                continue;
            }

            DatabaseSortDescriptor descriptor;

            if (parts[0] == "gamenum")
                descriptor.field = DATABASE_FIELD_GAME_NUM;
            else if (parts[0] == "whiteplayer")
                descriptor.field = DATABASE_FIELD_WHITEPLAYER;
            else if (parts[0] == "blackplayer")
                descriptor.field = DATABASE_FIELD_BLACKPLAYER;
            else if (parts[0] == "event")
                descriptor.field = DATABASE_FIELD_EVENT;
            else if (parts[0] == "site")
                descriptor.field = DATABASE_FIELD_SITE;
            else if (parts[0] == "round")
                descriptor.field = DATABASE_FIELD_ROUND;
            else if (parts[0] == "date")
                descriptor.field = DATABASE_FIELD_DATE;
            else if (parts[0] == "result")
                descriptor.field = DATABASE_FIELD_RESULT;
            else {
                cerr << "Invalid field '" << parts[0] << "'" << endl;
                continue;
            }

            if (parts[1] == "asc")
                descriptor.order = DATABASE_ORDER_ASCENDING;
            else if (parts[1] == "desc")
                descriptor.order = DATABASE_ORDER_DESCENDING;
            else {
                cerr << "Invalid order '" << parts[1] << "'" << endl;
                continue;
            }

            sortCriteria.push_back(descriptor);
        }

        if (cin.eof())
            return true;

        cout << "Enter offset/limit criteria and finish with a blank line (or 'quit' to end)" << endl;

        while (!g_quitFlag && !cin.eof()) {
            getline(cin, line);

            if (line.empty())
                break;
            else if (line == "quit")
                return true;

            Util::splitLine(line, parts);

            if (parts.size() != 2) {
                cerr << "Entry 'order/limit <number>', a blank line or 'quit'" << endl;
                continue;
            }

            if (parts[0] == "offset") {
                if (!Util::parse(parts[1], offset)) {
                    cerr << "Invalid number '" << parts[1] << "'" << endl;
                    continue;
                }
            } else if (parts[0] == "limit") {
                if (!Util::parse(parts[1], limit)) {
                    cerr << "Invalid number '" << parts[1] << "'" << endl;
                    continue;
                }
            } else {
                cerr << "Invalid keyword '" << parts[0] << "'" << endl;
                continue;
            }
        }

        if (cin.eof())
            return true;

        unsigned startTime = Util::getTickCount();

        if (indb->search(searchCriteria, sortCriteria, searchCallback, indb.get(), offset, limit)) {
            unsigned endTime = Util::getTickCount();
            cout << "Search completed in " << (endTime - startTime) << "mS" << endl;
        } else {
            cerr << "Search failed: " << indb->errorMsg() << endl;
        }

    }

    return false;
}

//
// Perftdiv
//
bool funcPerftdiv() {
    if (g_optFen.empty()) {
        cerr << "No FEN specified" << endl;
        return false;
    }

    if (g_optDepth < 1 || g_optNumber1 > 10) {
        cerr << "Depth out-of-range or unspecified" << endl;
        return false;
    }

    Position pos;
    if (pos.setFromFen(g_optFen.c_str()) != Position::LEGAL) {
        cerr << "Failed to set position; invalid FEN" << endl;
        return false;
    }

    if (!g_optQuiet)
        cout << pos.dump() << endl;

    perft(pos, g_optDepth, true);
    return true;
}

//
// RecursivePosDump
//
bool funcRecursivePosDump() {
    if (g_optFen.empty()) {
        cerr << "No FEN specified" << endl;
        return false;
    }

    if (g_optDepth < 1 || g_optNumber1 > 10) {
        cerr << "Depth out-of-range or unspecified" << endl;
        return false;
    }

    Position pos;
    if (pos.setFromFen(g_optFen.c_str()) != Position::LEGAL) {
        cerr << "Failed to set position; invalid FEN" << endl;
        return false;
    }

    if (!g_optQuiet)
        cout << pos.dump() << endl;

    posDump(pos, g_optDepth);

    return true;
}

//
// Interactive perftdiv.  This was written simply to be quicker to control via
// tools/find_buggy_pos.py.
//
bool funcFindBuggyPos() {

    if (!g_optQuiet) {
        cout << "The following commands are enabled:\n";
        cout << "randompos:           Generates a random position\n";
        cout << "setboard <fen>:      Sets the position to the given FEN.\n";
        cout << "perftdiv <depth>:    Performs a perftdiv of the current position.\n";
        cout << "quit:                Exits.\n";
    }

    string line;
    vector<string> parts;
    Position pos;

    while (!g_quitFlag && !cin.eof()) {
        getline(cin, line);
        if (line.empty())
            break;

        Util::splitLine(line, parts);

        if (parts.size() == 0)
            return false;
        if (parts[0] == "randompos" && parts.size() == 1) {
            pos.setRandom();
            cout << pos.fen() << endl;
        } else if (parts[0] == "setboard" && parts.size() == 7) {
            pos.init();
            if (pos.setFromFen(parts[1].c_str(), parts[2].c_str(), parts[3].c_str(),
                               parts[4].c_str(), parts[5].c_str(), parts[6].c_str()) != Position::LEGAL) {
                cerr << "Invalid FEN: " << Util::concat(parts, 1, 7) << endl;
                return false;
            }
        } else if (parts[0] == "perftdiv" && parts.size() == 2) {
            unsigned depth;
            if (!Util::parse(parts[1], depth)) {
                cerr << "Invalid depth value: " << parts[1] << endl;
                return false;
            }
            perft(pos, depth, true);
        } else if (parts[0] == "quit" && parts.size() == 1) {
            g_quitFlag = true;
        } else {
            cerr << "Unknown command: " << line << endl;
            return false;
        }
    }
    return true;
}

bool funcTestPopCnt() {
    cout << "Timing " << dec << g_optNumber1 << " popcnt operations" << endl;

    unsigned startTime = Util::getTickCount();

    for (int count = 0; count < g_optNumber1 && !g_quitFlag; count++)
        if (popcnt(0xaaaaaaaaaaaaaaaaULL) != 32) {
            cerr << "popcnt() didn't return 32" << endl;
            return false;
        }

    unsigned elapsed = Util::getTickCount() - startTime;

    if (elapsed > 0)
        cout << "popcnt time: " << Util::formatElapsed(elapsed) << " ("
             << unsigned(uint64_t(g_optNumber1) * 1000ULL) / elapsed << " popcnt/s)" << endl;
    else
        cout << "popcnt time: " << Util::formatElapsed(elapsed) << " (inf popcnt/s)" << endl;

    return true;
}

static uint64_t perft(const Position &pos, unsigned depth, bool printMoves) {
    if (depth == 0)
        return 1ULL;

    Move moves[256];
    uint64_t totalNodes = 0;
    unsigned numMoves = pos.genMoves(moves);
    Position posTemp(pos);
    for (unsigned i = 0; i < numMoves; i++) {
        UnmakeMoveInfo umi;
        if (!posTemp.makeMove(moves[i], umi))  {
            throw ChessCoreException("Failed to make move %s in position\n%s",
                moves[i].dump().c_str(), posTemp.dump().c_str());
        }
        uint64_t nodes = perft(posTemp, depth - 1, false);
        totalNodes += nodes;
        if (!g_optQuiet && printMoves)
            cout << setw(14) << (moves[i].san(pos) + ": ") << setw(12) << nodes << endl;
        if (!posTemp.unmakeMove(umi)) {
            throw ChessCoreException("Failed to unmake move %s in position\n%s",
                moves[i].dump().c_str(), posTemp.dump().c_str());
        }
    }

    if (printMoves) {
        if (!g_optQuiet)
            cout << setw(14) << "Total nodes: " << setw(12) << totalNodes << endl;
        else
            cout << totalNodes << endl;
    }
    return totalNodes;
}

static void posDump(const Position &pos, unsigned depth) {
    if (depth == 0)
        return;

    Move moves[256];
    unsigned numMoves = pos.genMoves(moves);
    Position posTemp(pos);
    for (unsigned i = 0; i < numMoves; i++) {
        UnmakeMoveInfo umi;
        if (!posTemp.makeMove(moves[i], umi))  {
            throw ChessCoreException("Failed to make move %s in position\n%s",
                                     moves[i].dump().c_str(), posTemp.dump().c_str());
        }
        cout << posTemp.fen() << endl;
        posDump(posTemp, depth - 1);
        if (!posTemp.unmakeMove(umi)) {
            throw ChessCoreException("Failed to unmake move %s in position\n%s",
                                     moves[i].dump().c_str(), posTemp.dump().c_str());
        }
    }
}

static bool indexCallback(unsigned gameNum, float percentComplete, void *contextInfo) {
    if ((gameNum % 1000) == 0)
        cout << gameNum << " (" << percentComplete << "%)" << endl;

    return !g_quitFlag;
}

static bool treeCallback(unsigned gameNum, float percentComplete, void *contextInfo) {
    if ((gameNum % 1000) == 0)
        cout << gameNum << " (" << percentComplete << "%)" << endl;

    return !g_quitFlag;
}

static bool searchCallback(unsigned gameNum, float percentComplete, void *contextInfo) {
    Database *database = (Database *)contextInfo;
    GameHeader header;

    if (database->readHeader(gameNum, header)) {
        string formatted;
        header.format(formatted, "Unknown", false);
        cout << gameNum << ": " << formatted << endl;
    } else {
        cerr << "Failed to read game header " << gameNum << ": " << database->errorMsg() << endl;
        return false;
    }

    return !g_quitFlag;
}
