#include <ChessCore/Position.h>
#include <ChessCore/Move.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Log.h>
#include <ChessCore/Rand64.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

TEST(MoveTest, basic) {
	EXPECT_EQ(sizeof(Move), 4);
}

static testing::AssertionResult testMove(const Position &pos, const Move m) {

	string san = m.san(pos);
	Move newm;
	if (!newm.parse(pos, san)) {
		return testing::AssertionFailure() << "Move::parse(\"" << san << "\") returned false";
	}

	if (newm.from() != m.from()) {
		return testing::AssertionFailure() << "Move \"" << san << "\" has different from: " << newm.from();
	}

	if (newm.to() != m.to()) {
		return testing::AssertionFailure() << "Move \"" << san << "\" has different to: " << newm.to();
	}

	if (newm.prom() != m.prom()) {
		return testing::AssertionFailure() << "Move \"" << san << "\" has different prom: " << newm.prom();
	}

	if (newm.flags() != m.flags()) {
		return testing::AssertionFailure() << "Move \"" << san << "\" has different flag: 0x" << hex << newm.from();
	}

	return testing::AssertionSuccess();
}

TEST(MoveTest, parse) {
	Position pos;
	Move m, moves[256];

	Rand64::init();
	pos.setStarting();
	for (unsigned i = 0; i < 20; i++) {
		unsigned numMoves = pos.genMoves(moves);
		for (unsigned j = 0; j < numMoves; j++) {
			testMove(pos, moves[j]);
		}

		unsigned moveNum = Rand64::rand() % numMoves;
		UnmakeMoveInfo umi;
		ASSERT_TRUE(pos.makeMove(moves[moveNum], umi));
	}
}