#include <ChessCore/Util.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

//
// Test popcnt().
//
// Call with EXPECT_TRUE().
//
static testing::AssertionResult testPopcnt() {
    uint64_t bb;
    int i;
    unsigned j, k;

    bb = 0ULL;

    for (i = 63, k = 1; i >= 0; i--, k++) {
        bb |= 1ULL << i;
        j = popcnt(bb);
        if (j != k) {
            return testing::AssertionFailure() << "popcnt(" << OUT_UINT64(bb) << ") returned " << dec << j << " and not " << dec << k;
        }
    }

    bb = 0ULL;

    for (i = 0, k = 1; i < 64; i++, k++) {
        bb |= 1ULL << i;
        j = popcnt(bb);
        if (j != k) {
            return testing::AssertionFailure() << "popcnt(" << OUT_UINT64(bb) << ") returned " << dec << j << " and not " << dec << k;
        }
    }

    return testing::AssertionSuccess();
}

//
// Test lsb() and lsb2().
//
// Call with EXPECT_TRUE().
//
static testing::AssertionResult testLsb() {
    uint64_t bb, bb2, bit;
    int i;
    unsigned j, k;

    bb = 0xffffffffffffffffULL;

    for (i = 0, k = 0; i < 64; i++, k++) {
        bb2 = bb;
        bit = 0;
        j = lsb(bb2);
        if (j != k) {
            return testing::AssertionFailure() << "lsb(" << OUT_UINT64(bb) << ") #1 returned " << dec << j << " and not " << dec << k;
        }

        j = lsb2(bb2, bit);
        if (j != k) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #1 returned " << dec << j << " and not " << dec << k;
        } else if (offsetBit(k) != bit) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #1 returned wrong bit (" << OUT_UINT64(bit) << ")";
        } else if ((bb & ~bit) != bb2) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #1 didn't clear bit (bb2=" << OUT_UINT64(bb2) <<
                ", bit=" << OUT_UINT64(bit) << ")";
        }

        bb <<= 1;
    }

    bb = 1ULL;

    for (i = 0, k = 0; i < 64; i++, k++) {
        bb2 = bb;
        bit = 0;
        j = lsb(bb2);
        if (j != k) {
            return testing::AssertionFailure() << "lsb(" << OUT_UINT64(bb) << ") #2 returned " << dec << j << " and not " << dec << k;
        }

        j = lsb2(bb2, bit);
        if (j != k) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #2 returned " << dec << j << " and not " << dec << k;
        } else if (offsetBit(k) != bit) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #2 returned wrong bit (" << OUT_UINT64(bit) << ")";
        } else if ((bb & ~bit) != bb2) {
            return testing::AssertionFailure() << "lsb2(" << OUT_UINT64(bb) << ") #2 didn't clear bit (bb2=" << OUT_UINT64(bb2) <<
                ", bit=" << OUT_UINT64(bit) << ")";
        }

        bb <<= 1;
    }

    return testing::AssertionSuccess();
}

//
// Test bswap16(), bswap32(), bswap64().
//
// Simply call.
//
static void testBSwap() {

    //
    // 16-bit
    //
    union {
        uint16_t u;
        uint8_t b[sizeof(uint16_t)];
    } u16;

    u16.b[0] = 0x01;
    u16.b[1] = 0x02;
    u16.u = bswap16(u16.u);
    EXPECT_EQ(0x02, u16.b[0]);
    EXPECT_EQ(0x01, u16.b[1]);

    //
    // 32-bit
    //
    union {
        uint32_t u;
        uint8_t b[sizeof(uint32_t)];
    } u32;

    u32.b[0] = 0x01;
    u32.b[1] = 0x02;
    u32.b[2] = 0x03;
    u32.b[3] = 0x04;
    u32.u = bswap32(u32.u);
    EXPECT_EQ(0x04, u32.b[0]);
    EXPECT_EQ(0x03, u32.b[1]);
    EXPECT_EQ(0x02, u32.b[2]);
    EXPECT_EQ(0x01, u32.b[3]);

    //
    // 64-bit
    //
    union {
        uint64_t u;
        uint8_t b[sizeof(uint64_t)];
    } u64;

    u64.b[0] = 0x01;
    u64.b[1] = 0x02;
    u64.b[2] = 0x03;
    u64.b[3] = 0x04;
    u64.b[4] = 0x05;
    u64.b[5] = 0x06;
    u64.b[6] = 0x07;
    u64.b[7] = 0x08;
    u64.u = bswap64(u64.u);
    EXPECT_EQ(0x08, u64.b[0]);
    EXPECT_EQ(0x07, u64.b[1]);
    EXPECT_EQ(0x06, u64.b[2]);
    EXPECT_EQ(0x05, u64.b[3]);
    EXPECT_EQ(0x04, u64.b[4]);
    EXPECT_EQ(0x03, u64.b[5]);
    EXPECT_EQ(0x02, u64.b[6]);
    EXPECT_EQ(0x01, u64.b[7]);
}

TEST(LowlevelTest, popcnt) {
    EXPECT_TRUE(testPopcnt());
}

TEST(LowlevelTest, lsb) {
    EXPECT_TRUE(testLsb());
}

TEST(LowlevelTest, bswap) {
    testBSwap();
}
