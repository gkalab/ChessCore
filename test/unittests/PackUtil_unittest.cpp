#include <ChessCore/Util.h>
#include <gtest/gtest.h>

using namespace std;
using namespace ChessCore;

static testing::AssertionResult testPackUtil() {
    const uint16_t u16_2 = 0x1234;
    const uint32_t u32_3 = 0x123456;
    const uint32_t u32_4 = 0x12345678;
    const uint64_t u64_5 = 0x123456789aULL;
    const uint64_t u64_6 = 0x123456789abcULL;
    const uint64_t u64_7 = 0x123456789abcdeULL;
    const uint64_t u64_8 = 0x123456789abcdef0ULL;
    uint8_t buffer[sizeof(uint64_t)];

#define PACKUTIL_TEST(type, var, size, endian) \
    PackUtil<type>::endian(var, buffer, size); \
    type var ## size ## endian = PackUtil<type>::endian(buffer, size); \
    if (var ## size ## endian != var) { \
        return testing::AssertionFailure() << "PackUtil " #type " " #size " " #endian " test failed: 0x" << hex << var ## size ## endian << \
            " != 0x" << hex << var; \
    }

    PACKUTIL_TEST(uint16_t, u16_2, 2, little);
    PACKUTIL_TEST(uint16_t, u16_2, 2, big);
    PACKUTIL_TEST(uint32_t, u32_3, 3, little);
    PACKUTIL_TEST(uint32_t, u32_3, 3, big);
    PACKUTIL_TEST(uint32_t, u32_4, 4, little);
    PACKUTIL_TEST(uint32_t, u32_4, 4, big);
    PACKUTIL_TEST(uint64_t, u64_5, 5, little);
    PACKUTIL_TEST(uint64_t, u64_5, 5, big);
    PACKUTIL_TEST(uint64_t, u64_6, 6, little);
    PACKUTIL_TEST(uint64_t, u64_6, 6, big);
    PACKUTIL_TEST(uint64_t, u64_7, 7, little);
    PACKUTIL_TEST(uint64_t, u64_7, 7, big);
    PACKUTIL_TEST(uint64_t, u64_8, 8, little);
    PACKUTIL_TEST(uint64_t, u64_8, 8, big);

    return testing::AssertionSuccess();
}

TEST(PackUtilTest, test) {
    EXPECT_TRUE(testPackUtil());
}
