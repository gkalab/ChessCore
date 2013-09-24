#include <ChessCore/Game.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

//
// A general test, which tests:
//
// Move (parsing).
// AnnotMove (game tree).
// 
TEST(GameTest, parseGame) {
    Game game;
    unsigned i;

    // Make the following moves:
    // 1.e4 (1.d4 Nf6 2.e4 Nc6 (2...e5 f4 3...h6)) e5 2.Nc3 (2.Nf3) (2.d4) Nc6 d4
    const char *moves[] = {
        "e4",
        "(",  "d4",    "Nf6",   "e4",  "Nc6",
        "(",  "e5",    "f4",    "h6",  ")",
        ")",
        "e5", "Nc3",
        "(",  "Nf3",   ")",
        "(",  "d4",    ")",
        "Nc6", "d4",    0
    };

    for (i = 0; moves[i] != 0; i++) {
        string movetext = moves[i];

        if (movetext == "(") {
            EXPECT_TRUE(game.startVariation());
        } else if (movetext == ")") {
            EXPECT_TRUE(game.endVariation());
        } else if (!game.makeMove(movetext)) {
            FAIL() << "Failed to make move " << movetext;
        }
    }

    EXPECT_NE(nullptr, game.mainline());
    string movesStr = game.mainline()->dumpLine();
    EXPECT_EQ("Pe2e4 (Pd2d4 Ng8f6 Pe2e4 Nb8c6 (Pe7e5 Pf2f4 Ph7h6)) Pe7e5 Nb1c3 (Ng1f3) (Pd2d4) Nb8c6 Pd2d4",
              movesStr);
}

