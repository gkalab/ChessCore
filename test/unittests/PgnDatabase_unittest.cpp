#include <ChessCore/PgnDatabase.h>
#include <ChessCore/Game.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

#define HEADER "[Event \"?\"]\n[Site \"?\"]\n[Date \"????.??.??\"]\n[Round \"?\"]\n[White \"?\"]\n[Black \"?\"]\n[Result \"*\"]\n\n"

//
// PGN Parser tests
//

// Test normal Null move "--"
TEST(PgnDatabaseTest, parseNullMove) {
    PgnDatabase pgnDb;
    Game game;
    pgnDb.readFromString("1. d4 Nf6 2. c4 c5 3. d5 b5 4. cxb5 a6 5. bxa6 Bxa6 6. -- d6 *", game);
    string movesStr = game.mainline()->dumpLine();
    EXPECT_EQ("Pd2d4 Ng8f6 Pc2c4 Pc7c5 Pd4d5 Pb7b5 Pc4b5 Pa7a6 Pb5a6 Bc8a6 null Pd7d6", movesStr);
}

// Test Null move "Z0"
TEST(PgnDatabaseTest, parseNullMoveWithZ0AsNull) {
    PgnDatabase pgnDb;
    Game game;
    pgnDb.readFromString("1. e4 e5 2. Nf3 Nc6 3. Bc4 Bc5 4. Nc3 Z0 (4... Nh6 5. d3 d6) *", game);
    string movesStr = game.mainline()->dumpLine();
    EXPECT_EQ("Pe2e4 Pe7e5 Ng1f3 Nb8c6 Bf1c4 Bf8c5 Nb1c3 null (Ng8h6 Pd2d3 Pd7d6)", movesStr);
}

TEST(PgnDatabaseTest, nullMoveOutputsAsTwoDashes) {
    PgnDatabase pgnDb;
    Game game;
    pgnDb.readFromString("1. e4 e5 2. Nf3 Nc6 3. Bc4 Bc5 4. Nc3 -- (4... Nh6 5. d3 d6) *", game);
    string movesStr;
    game.get(movesStr);
    EXPECT_EQ(HEADER "1. e4 e5 2. Nf3 Nc6 3. Bc4 Bc5 4. Nc3 -- (4... Nh6 5. d3 d6) *\n", movesStr);
}

