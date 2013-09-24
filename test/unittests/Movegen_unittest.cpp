#include <ChessCore/Position.h>
#include <ChessCore/Move.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Log.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

#define TEST_DEPTH_5 1

//
// From: http://chessprogramming.wikispaces.com/Perft+Results
//
static const char *fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static const char *fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
static const char *fen3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
static const char *fen4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
static const char *fen5 = "rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6";

// Recursive perft function
static uint64_t perft(const Position &pos, unsigned depth) {
    if (depth == 0)
        return 1ULL;

    Move moves[256];
    uint64_t totalMoves = 0;
    unsigned numMoves = pos.genMoves(moves);
    Position posTemp(pos);
    for (unsigned i = 0; i < numMoves; i++) {
        UnmakeMoveInfo umi;
        if (!posTemp.makeMove(moves[i], umi))  {
            throw ChessCoreException("Failed to make move %s in position\n%s",
                moves[i].dump().c_str(), posTemp.dump().c_str());
        }
        totalMoves += perft(posTemp, depth - 1);
        if (!posTemp.unmakeMove(umi)) {
            throw ChessCoreException("Failed to unmake move %s in position\n%s",
                moves[i].dump().c_str(), posTemp.dump().c_str());
        }
    }
    return totalMoves;
}

//
// These tests could be one giant test, however they are slow so it's better to
// see some progress allowing the quicker ones to run first.
//
static void testPerft(const char *fen, unsigned depth, uint64_t expected) {
    Position pos;
    EXPECT_TRUE(pos.setFromFen(fen) == Position::LEGAL);
    EXPECT_EQ(expected, perft(pos, depth));
}

TEST(MovegenTest, perft1a) {
    testPerft(fen1, 1, 20);
}

TEST(MovegenTest, perft1b) {
    testPerft(fen1, 2, 400);
}

TEST(MovegenTest, perft1c) {
    testPerft(fen1, 3, 8902);
}

TEST(MovegenTest, perft1d) {
    testPerft(fen1, 4, 197281);
}

#if TEST_DEPTH_5
TEST(MovegenTest, perft1e) {
    testPerft(fen1, 5, 4865609);
}
#endif

TEST(MovegenTest, perft2a) {
    testPerft(fen2, 1, 48);
}

TEST(MovegenTest, perft2b) {
    testPerft(fen2, 2, 2039);
}

TEST(MovegenTest, perft2c) {
    testPerft(fen2, 3, 97862);
}

TEST(MovegenTest, perft2d) {
    testPerft(fen2, 4, 4085603);
}

#if TEST_DEPTH_5
TEST(MovegenTest, perft2e) {
    testPerft(fen2, 5, 193690690);
}
#endif

TEST(MovegenTest, perft3a) {
    testPerft(fen3, 1, 14);
}

TEST(MovegenTest, perft3b) {
    testPerft(fen3, 2, 191);
}

TEST(MovegenTest, perft3c) {
    testPerft(fen3, 3, 2812);
}

TEST(MovegenTest, perft3d) {
    testPerft(fen3, 4, 43238);
}

#if TEST_DEPTH_5
TEST(MovegenTest, perft3e) {
    testPerft(fen3, 5, 674624);
}
#endif

TEST(MovegenTest, perft4a) {
    testPerft(fen4, 1, 6);
}

TEST(MovegenTest, perft4b) {
    testPerft(fen4, 2, 264);
}

TEST(MovegenTest, perft4c) {
    testPerft(fen4, 3, 9467);
}

TEST(MovegenTest, perft4d) {
    testPerft(fen4, 4, 422333);
}

#if TEST_DEPTH_5
TEST(MovegenTest, perft4e) {
    testPerft(fen4, 5, 15833292);
}
#endif

TEST(MovegenTest, perft5a) {
    testPerft(fen5, 1, 42);
}

TEST(MovegenTest, perft5b) {
    testPerft(fen5, 2, 1352);
}

TEST(MovegenTest, perft5c) {
    testPerft(fen5, 3, 53392);
}

TEST(MovegenTest, epCapPinned) {
    // The move generator must not generate the en-passant captures when they are pinned to the king
    Position pos;
    Move moves[256];
    pos.setFromFen("8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - e3 0 1");
    unsigned numMoves = pos.genMoves(moves);
    for (unsigned i = 0; i < numMoves; i++) {
        EXPECT_FALSE(moves[i].from() == F4 && moves[i].to() == E3);
    }
}

TEST(MovegenTest, bug1) {
    // Move generator bug
    testPerft("Q7/p7/8/k7/6K1/8/8/8 b - - 0 1", 1, 6);
}

TEST(MovegenTest, bug2) {
    // Move generator bug
    testPerft("8/2p5/3p4/KP6/R1r2pPk/4P3/8/8 b - g3 0 3", 1, 19);
}

