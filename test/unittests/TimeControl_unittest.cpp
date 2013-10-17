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
    EXPECT_EQ(0u, periods[0].moves());
    EXPECT_EQ(300u, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("G/5", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_g5) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("300", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0u, periods[0].moves());
    EXPECT_EQ(300u, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("300", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_g5_10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("G/5/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0u, periods[0].moves());
    EXPECT_EQ(300u, periods[0].time());
    EXPECT_EQ(10, periods[0].increment());
    EXPECT_EQ("G/5/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_g5_10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("300+10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[0].type());
    EXPECT_EQ(0u, periods[0].moves());
    EXPECT_EQ(300u, periods[0].time());
    EXPECT_EQ(10, periods[0].increment());
    EXPECT_EQ("300+10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("M/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[0].type());
    EXPECT_EQ(1u, periods[0].moves());
    EXPECT_EQ(10u, periods[0].time());
    EXPECT_EQ(0 ,periods[0].increment());
    EXPECT_EQ("M/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("*10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(1, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[0].type());
    EXPECT_EQ(1u, periods[0].moves());
    EXPECT_EQ(10u, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ("*10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120, G/30", TimeControlPeriod::FORMAT_NORMAL));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[1].type());
    EXPECT_EQ(0u, periods[1].moves());
    EXPECT_EQ(1800u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/120, G/30", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200:1800", TimeControlPeriod::FORMAT_PGN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(0, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[1].type());
    EXPECT_EQ(0u, periods[1].moves());
    EXPECT_EQ(1800u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/7200:1800", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_30_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120/30, M/10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[1].type());
    EXPECT_EQ(1u, periods[1].moves());
    EXPECT_EQ(10u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/120/30, M/10", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_30_m10) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200+30,*10", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(2, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_MOVES_IN, periods[1].type());
    EXPECT_EQ(1u, periods[1].moves());
    EXPECT_EQ(10u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ("40/7200+30:*10", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, normal_40_120_30_20_60_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/120/30, 20/60, G/30", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(3, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[1].type());
    EXPECT_EQ(20u, periods[1].moves());
    EXPECT_EQ(3600u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[2].type());
    EXPECT_EQ(0u, periods[2].moves());
    EXPECT_EQ(1800u, periods[2].time());
    EXPECT_EQ(0, periods[2].increment());
    EXPECT_EQ("40/120/30, 20/60, G/30", timeControl.notation(TimeControlPeriod::FORMAT_NORMAL));
}

TEST(TimeControlTest, pgn_40_120_30_20_60_g30) {
    TimeControl timeControl;
    EXPECT_TRUE(timeControl.set("40/7200+30:20/3600:1800", TimeControlPeriod::FORMAT_UNKNOWN));
    vector<TimeControlPeriod> &periods = timeControl.periods();
    EXPECT_EQ(3, periods.size());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[0].type());
    EXPECT_EQ(40u, periods[0].moves());
    EXPECT_EQ(7200u, periods[0].time());
    EXPECT_EQ(30, periods[0].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_ROLLOVER, periods[1].type());
    EXPECT_EQ(20u, periods[1].moves());
    EXPECT_EQ(3600u, periods[1].time());
    EXPECT_EQ(0, periods[1].increment());
    EXPECT_EQ(TimeControlPeriod::TYPE_GAME_IN, periods[2].type());
    EXPECT_EQ(0u, periods[2].moves());
    EXPECT_EQ(1800u, periods[2].time());
    EXPECT_EQ(0, periods[2].increment());
    EXPECT_EQ("40/7200+30:20/3600:1800", timeControl.notation(TimeControlPeriod::FORMAT_PGN));
}

TEST(TimeControlTest, tracking_g1) {
    TimeControl timeControl;
    TimeTracker timeTracker(timeControl);
    EXPECT_TRUE(timeControl.set("G/1", TimeControlPeriod::FORMAT_UNKNOWN));

    EXPECT_TRUE(timeTracker.reset());
    EXPECT_EQ(0u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(60000u, timeTracker.timeLeft());
    EXPECT_LE(60000u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(1u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(58800u, timeTracker.timeLeft());
    EXPECT_LE(58800u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(2u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(21800u, timeTracker.timeLeft());
    EXPECT_LE(21800u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(3u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(1798u, timeTracker.timeLeft());
    EXPECT_LE(1798u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1798));
    EXPECT_EQ(4, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(0u, timeTracker.timeLeft());
    EXPECT_EQ(0u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1));
    EXPECT_EQ(5u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(0u, timeTracker.timeLeft());
    EXPECT_EQ(0u, timeTracker.runningTimeLeft());
    EXPECT_TRUE(timeTracker.isOutOfTime());
}

TEST(TimeControlTest, tracking_4_1_G1) {
    TimeControl timeControl;
    TimeTracker timeTracker(timeControl);
    EXPECT_TRUE(timeControl.set("4/1,G/1", TimeControlPeriod::FORMAT_UNKNOWN));

    EXPECT_TRUE(timeTracker.reset());
    EXPECT_EQ(0u, timeTracker.numMoves());
    EXPECT_EQ(4u, timeTracker.movesLeft());
    EXPECT_EQ(60000u, timeTracker.timeLeft());
    EXPECT_LE(60000u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(1u, timeTracker.numMoves());
    EXPECT_EQ(3u, timeTracker.movesLeft());
    EXPECT_EQ(58800u, timeTracker.timeLeft());
    EXPECT_LE(58800u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(2u, timeTracker.numMoves());
    EXPECT_EQ(2u, timeTracker.movesLeft());
    EXPECT_EQ(21800u, timeTracker.timeLeft());
    EXPECT_LE(21800u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(3u, timeTracker.numMoves());
    EXPECT_EQ(1u, timeTracker.movesLeft());
    EXPECT_EQ(1798u, timeTracker.timeLeft());
    EXPECT_LE(1798u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1000));
    EXPECT_EQ(4u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(60798u, timeTracker.timeLeft());
    EXPECT_LE(60798u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1200));
    EXPECT_EQ(5u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(59598u, timeTracker.timeLeft());
    EXPECT_LE(59598u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(37000));
    EXPECT_EQ(6u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(22598u, timeTracker.timeLeft());
    EXPECT_LE(22598u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(20002));
    EXPECT_EQ(7u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(2596u, timeTracker.timeLeft());
    EXPECT_LE(2596u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(2596));
    EXPECT_EQ(8u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(0u, timeTracker.timeLeft());
    EXPECT_EQ(0u, timeTracker.runningTimeLeft());
    EXPECT_FALSE(timeTracker.isOutOfTime());

    EXPECT_TRUE(timeTracker.update(1));
    EXPECT_EQ(9u, timeTracker.numMoves());
    EXPECT_EQ(0u, timeTracker.movesLeft());
    EXPECT_EQ(0u, timeTracker.timeLeft());
    EXPECT_LE(0u, timeTracker.runningTimeLeft());
    EXPECT_TRUE(timeTracker.isOutOfTime());
}

TEST(TimeControlTest, tracking_m10) {
    TimeControl timeControl;
    TimeTracker timeTracker(timeControl);
    EXPECT_TRUE(timeControl.set("M/10", TimeControlPeriod::FORMAT_UNKNOWN));
    EXPECT_TRUE(timeTracker.reset());

    for (unsigned i = 1; i <= 1000; i++) {
        EXPECT_TRUE(timeTracker.update(9999));
        EXPECT_EQ(i, timeTracker.numMoves());
        EXPECT_EQ(1, timeTracker.movesLeft());
        EXPECT_EQ(10000u, timeTracker.timeLeft());
        EXPECT_LE(10000u, timeTracker.runningTimeLeft());
        EXPECT_FALSE(timeTracker.isOutOfTime());
    }
}
