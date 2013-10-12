#include <ChessCore/TimeControl.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace std;
using namespace ChessCore;

TEST(TimeControlTest, normal_g5) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("G/5", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0, periods[0].moves());
    EXPECT_EQ(300, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("G/5", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_g5) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("300", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0, periods[0].moves());
    EXPECT_EQ(300, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("300", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_g5_10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("G/5/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0, periods[0].moves());
    EXPECT_EQ(300, periods[0].time());
    EXPECT_EQ(10, periods[0].increment());
    EXPECT_EQ("G/5/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_g5_10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("300+10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0, periods[0].moves());
    EXPECT_EQ(300, periods[0].time());
    EXPECT_EQ(10, periods[0].increment());
    EXPECT_EQ("300+10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("M/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[0].type());
    EXPECT_EQ(1, periods[0].moves());
    EXPECT_EQ(10, periods[0].time());
    EXPECT_EQ(0 ,periods[0].increment());
    EXPECT_EQ("M/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("*10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[0].type());
    EXPECT_EQ(1, periods[0].moves());
    EXPECT_EQ(10, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("*10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120,G/30", TimeControlPeriod::FORMAT_NORMAL));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[1].type());
    EXPECT_EQ(0, periods[1].moves());
    EXPECT_EQ(1800, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/120,G/30", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200:1800", TimeControlPeriod::FORMAT_PGN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[1].type());
    EXPECT_EQ(0, periods[1].moves());
    EXPECT_EQ(1800, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/7200:1800", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_30_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120/30,M/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[1].type());
    EXPECT_EQ(1, periods[1].moves());
    EXPECT_EQ(10, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/120/30,M/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_30_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200+30,*10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[1].type());
    EXPECT_EQ(1, periods[1].moves());
    EXPECT_EQ(10, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/7200+30:*10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_30_20_60_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120/30,20/60,G/30", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(3, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[1].type());
    EXPECT_EQ(20, periods[1].moves());
    EXPECT_EQ(3600, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[2].type());
    EXPECT_EQ(0, periods[2].moves());
    EXPECT_EQ(1800, periods[2].time());
    EXPECT_EQ(0, periods[2].increment());
    EXPECT_EQ("40/120/30,20/60,G/30", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_30_20_60_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200+30:20/3600:1800", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(3, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40, periods[0].moves());
    EXPECT_EQ(7200, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[1].type());
    EXPECT_EQ(20, periods[1].moves());
    EXPECT_EQ(3600, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[2].type());
    EXPECT_EQ(0, periods[2].moves());
    EXPECT_EQ(1800, periods[2].time());
    EXPECT_EQ(0, periods[2].increment());
    EXPECT_EQ("40/7200+30:20/3600:1800", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, tracking_g1) {
    TimeControl timeControl;
    TimeTracker timeTracker(timeControl);
    EXPECT_TRUE(timeControl.set("G/1", TimeControlPeriod::FORMAT_UNKNOWN));

    EXPECT_TRUE(timeTracker.reset());
    EXPECT_EQ(0, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(60000, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(1, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(58800, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(2, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(21800, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(3, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(1798, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1798));
    EXPECT_EQ(4, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(0, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1));
    EXPECT_EQ(5, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(0, timeTracker.timeLeft());
    EXPECT_TRUE(timeTracker.isOutOfTime());
}

TEST(TimeControlTest, tracking_4_1_G1) {
    TimeControl timeControl;
    TimeTracker timeTracker(timeControl);
    EXPECT_TRUE(timeControl.set("4/1,G/1", TimeControlPeriod::FORMAT_UNKNOWN));

    EXPECT_TRUE(timeTracker.reset());
    EXPECT_EQ(0, timeTracker.numMoves());
    EXPECT_EQ(4, timeTracker.movesLeft());
    EXPECT_EQ(60000, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(1, timeTracker.numMoves());
    EXPECT_EQ(3, timeTracker.movesLeft());
    EXPECT_EQ(58800, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(2, timeTracker.numMoves());
    EXPECT_EQ(2, timeTracker.movesLeft());
    EXPECT_EQ(21800, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(3, timeTracker.numMoves());
    EXPECT_EQ(1, timeTracker.movesLeft());
    EXPECT_EQ(1798, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1000));
    EXPECT_EQ(4, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(60798, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(5, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(59598, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(6, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(22598, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(7, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(2596, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(2596));
    EXPECT_EQ(8, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(0, timeTracker.timeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1));
    EXPECT_EQ(9, timeTracker.numMoves());
    EXPECT_EQ(0, timeTracker.movesLeft());
    EXPECT_EQ(0, timeTracker.timeLeft());
    EXPECT_TRUE(timeTracker.isOutOfTime());
}


