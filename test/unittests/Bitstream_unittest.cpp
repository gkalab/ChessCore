#include <ChessCore/Blob.h>
#include <ChessCore/Bitstream.h>
#include <ChessCore/Rand64.h>
#include <gtest/gtest.h>
#include <vector>

using namespace std;
using namespace ChessCore;

static testing::AssertionResult testBitstream() {
    Blob blob;
    Bitstream bitstream(blob);

    // Generate a set of random values and bitsizes
    int i, total = abs((int)(Rand64::rand() % 900ULL)) + 100;
    vector<uint32_t> values(total);
    vector<unsigned> bitsizes(total);
    uint32_t value, expected;

    for (i = 0; i < total; i++) {
        bitsizes[i] = (unsigned)(Rand64::rand() % 31) + 1;
        values[i] = (uint32_t)(Rand64::rand() & 0xffffffffULL);

        if (!bitstream.write(values[i], bitsizes[i])) {
            return testing::AssertionFailure() << "Failed to write value at index " << i;
        }
    }

#if 0   // Dump the test data and blob
    logdbg("Bitstream length=%u", bitstream.length());

    for (i = 0; i < total; i++) {
        expected = values[i] & ((1 << bitsizes[i]) - 1);
        logdbg("index %06u: 0x%x (length %u)", i, expected, bitsizes[i]);
    }

    LOGDBG << "Blob data:";
    LOGDBG << blob.dump();
#endif

    bitstream.reset();

    for (i = 0; i < total; i++) {
        if (!bitstream.read(value, bitsizes[i])) {
            return testing::AssertionFailure() << "Failed to read value at index " << i;
        }

        expected = values[i] & ((1 << bitsizes[i]) - 1);

        if (value != expected) {
            return testing::AssertionFailure() << "Bitstream error at index " << i << "; 0x"
                << hex << value << " != 0x" << hex << expected << " (length " << dec << bitsizes[i] << ")";
        }
    }

    return testing::AssertionSuccess();
}

TEST(BitstreamTest, test) {
    EXPECT_TRUE(testBitstream());
}
