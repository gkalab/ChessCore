#include <ChessCore/Util.h>
#include <ChessCore/Blob.h>
#include <ChessCore/Log.h>
#include <gtest/gtest.h>
#include <vector>
#include <fstream>

using namespace std;
using namespace ChessCore;

// in unittest_main.cpp
extern testing::AssertionResult regexMatch(const string &toMatch, const string &regexStr);

static const char *m_classname = "Util_unittests";

TEST(UtilTest, format) {
    string out;

    out = Util::format("Hello %s", "World");
    EXPECT_EQ("Hello World", out);

    out = Util::format("The meaning of life is %d", 42);
    EXPECT_EQ("The meaning of life is 42", out);

    out = Util::format("1.2 + 2.3 = %.2f", 1.2 + 2.3);
    EXPECT_EQ("1.2 + 2.3 = 3.50", out);
}

TEST(UtilTest, formatNPS) {
    string out;

    out = Util::formatNPS(1, 0);
    EXPECT_EQ("INF", out);

    out = Util::formatNPS(143000000ULL, 1500);
    EXPECT_EQ("95.333 Mnps", out);
}

TEST(UtilTest, formatBB) {
    string out;

    out = Util::formatBB(0x0123456789abcdefULL);
    EXPECT_EQ("+---------------+\n"
              "|X|.|.|.|.|.|.|.|\n"
              "|X|X|.|.|.|X|.|.|\n"
              "|X|.|X|.|.|.|X|.|\n"
              "|X|X|X|.|.|X|X|.|\n"
              "|X|.|.|X|.|.|.|X|\n"
              "|X|X|.|X|.|X|.|X|\n"
              "|X|.|X|X|.|.|X|X|\n"
              "|X|X|X|X|.|X|X|X|\n"
              "+---------------+\n",
              out);
}

TEST(UtilTest, formatTime) {
    EXPECT_TRUE(regexMatch(
        Util::formatTime(true, true),
        "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]"
    ));

    EXPECT_TRUE(regexMatch(
        Util::formatTime(true, false),
        "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9]"
    ));

    EXPECT_TRUE(regexMatch(
        Util::formatTime(false, true),
        "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]"
    ));

    EXPECT_TRUE(regexMatch(
        Util::formatTime(false, false),
        "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9]"
    ));
}

TEST(UtilTest, formatDatePGN) {
    EXPECT_TRUE(regexMatch(
        Util::formatDatePGN(),
        "[0-9][0-9][0-9][0-9]\\.[0-9][0-9]\\.[0-9][0-9]"
    ));
}

TEST(UtilTest, formatElapsed) {
    string out;

    out = Util::formatElapsed(12345);
    EXPECT_EQ("12.345", out);

    out = Util::formatElapsed(1392345);
    EXPECT_EQ("23:12.345", out);

    out = Util::formatElapsed(292992345);
    EXPECT_EQ("81:23:12.345", out);
}

TEST(UtilTest, formatMilli) {
    string out;

    out = Util::formatMilli(-12345);
    EXPECT_EQ("-12.345", out);

    out = Util::formatMilli(12345);
    EXPECT_EQ("12.345", out);
}

TEST(UtilTest, formatCenti) {
    string out;

    out = Util::formatCenti(-1923);
    EXPECT_EQ("-19.23", out);

    out = Util::formatCenti(+1923);
    EXPECT_EQ("+19.23", out);
}

TEST(UtilTest, formatData) {
    string out;
    uint8_t data[40] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
        0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28
    };

    out = Util::formatData(data, sizeof(data));
    EXPECT_EQ("length=40 (0x28)\n"
              "00000000: 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 ................\n"
              "00000010: 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 ............... \n"
              "00000020: 21 22 23 24 25 26 27 28                         !\"#$%&'(\n",
              out);
}

TEST(UtilTest, percent) {
    EXPECT_EQ(45, Util::percent(45, 100));
    EXPECT_EQ(99, Util::percent(999, 1000));
}

TEST(UtilTest, parse) {
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    bool b;

    EXPECT_TRUE(Util::parse("12345678", i32));
    EXPECT_EQ(12345678, i32);

    EXPECT_TRUE(Util::parse("-12345678", i32));
    EXPECT_EQ(-12345678, i32);

    EXPECT_TRUE(Util::parse("1234567890123456", i64));
    EXPECT_EQ(1234567890123456LL, i64);

    EXPECT_TRUE(Util::parse("-1234567890123456", i64));
    EXPECT_EQ(-1234567890123456LL, i64);

    EXPECT_TRUE(Util::parse("12345678", u32));
    EXPECT_EQ(12345678, u32);

    EXPECT_TRUE(Util::parse("1234567890123456", u64));
    EXPECT_EQ(1234567890123456LL, u64);

    EXPECT_TRUE(Util::parse("TrUE", b) && b);
    EXPECT_TRUE(Util::parse("oN", b) && b);
    EXPECT_TRUE(Util::parse("yEs", b) && b);
    EXPECT_TRUE(Util::parse("1", b) && b);

    EXPECT_TRUE(Util::parse("fALse", b) && !b);
    EXPECT_TRUE(Util::parse("oFF", b) && !b);
    EXPECT_TRUE(Util::parse("No", b) && !b);
    EXPECT_TRUE(Util::parse("0", b) && !b);
}

TEST(UtilTest, tickCountAndSleep) {
    unsigned start = Util::getTickCount();
    Util::sleep(450);
    unsigned end = Util::getTickCount();
    EXPECT_GE(420u, end - start);			// Have had 437ms under Windows once...
}

TEST(UtilTest, splitLine) {
    const char *str = "Mary had a \"little lamb\" its fleece was 'white as snow' and everywhere";
    const unsigned MAX_PARTS = 32;
    char *parts[MAX_PARTS];
    char strTemp[128];
    unsigned numParts;

    strcpy(strTemp, str);
    numParts = Util::splitLine(strTemp, parts, MAX_PARTS);
    EXPECT_EQ(10, numParts);
    EXPECT_STREQ("Mary",            parts[0]);
    EXPECT_STREQ("had",             parts[1]);
    EXPECT_STREQ("a",               parts[2]);
    EXPECT_STREQ("little lamb",     parts[3]);
    EXPECT_STREQ("its",             parts[4]);
    EXPECT_STREQ("fleece",          parts[5]);
    EXPECT_STREQ("was",             parts[6]);
    EXPECT_STREQ("white as snow",   parts[7]);
    EXPECT_STREQ("and",             parts[8]);
    EXPECT_STREQ("everywhere",      parts[9]);
    EXPECT_EQ(nullptr,              parts[10]);

    vector<string> vparts;
    numParts = Util::splitLine(str, vparts);
    EXPECT_EQ(10, numParts);
    EXPECT_EQ(10, vparts.size());
    EXPECT_STREQ("Mary",           parts[0]);
    EXPECT_STREQ("had",            parts[1]);
    EXPECT_STREQ("a",              parts[2]);
    EXPECT_STREQ("little lamb",    parts[3]);
    EXPECT_STREQ("its",            parts[4]);
    EXPECT_STREQ("fleece",         parts[5]);
    EXPECT_STREQ("was",            parts[6]);
    EXPECT_STREQ("white as snow",  parts[7]);
    EXPECT_STREQ("and",            parts[8]);
    EXPECT_STREQ("everywhere",     parts[9]);
}

TEST(UtilTest, trim) {
    string out = "  hello world   ";
    Util::trim(out);
    EXPECT_EQ("hello world", out);

    out = Util::trim("  hello world   ");
    EXPECT_EQ("hello world", out);
}

TEST(UtilTest, toLowerUpper) {
    string out;

    out = Util::tolower("ARSENAL FC");
    EXPECT_EQ("arsenal fc", out);
    out = Util::toupper(out);
    EXPECT_EQ("ARSENAL FC", out);
}

TEST(UtilTest, concat) {
    string out;
    vector<string> parts;
    parts.push_back("Mary");
    parts.push_back("had");
    parts.push_back("a");
    parts.push_back("little");
    parts.push_back("lamb");

    out = Util::concat(parts, 0, 2);
    EXPECT_EQ("Mary had", out);

    out = Util::concat(parts, 3, 5);
    EXPECT_EQ("little lamb", out);
}

TEST(UtilTest, startsWith) {
    EXPECT_TRUE(Util::startsWith("Apple", "App", true));
    EXPECT_TRUE(Util::startsWith("Apple", "APP", false));
    EXPECT_FALSE(Util::startsWith("Apple", "Apb", true));
    EXPECT_FALSE(Util::startsWith("Apple", "APB", false));
}

TEST(UtilTest, endsWith) {
    EXPECT_TRUE(Util::endsWith("Apple", "pple", true));
    EXPECT_TRUE(Util::endsWith("Apple", "PPLE", false));
    EXPECT_FALSE(Util::endsWith("Apple", "lee", true));
    EXPECT_FALSE(Util::endsWith("Apple", "LEE", false));
}

TEST(UtilTest, fileops) {

    uint64_t startTime = Util::currentTime();

    string filename = Util::tempFilename("UtilTest_fileops");
    EXPECT_TRUE(!filename.empty());
    LOGINF << "Using file '" << filename << "'";

    string dirname = Util::dirName(filename);
    EXPECT_EQ(g_tempDir, dirname);
    EXPECT_EQ(g_tempDir + PATHSEP + Util::baseName(filename), filename);
    EXPECT_TRUE(Util::dirExists(dirname));

    ofstream f(filename);
    EXPECT_TRUE(f.good());
    f << "testing" << endl;
    f.close();

    EXPECT_TRUE(Util::fileExists(filename));
    uint64_t modTime = Util::modifyTime(filename);
    EXPECT_TRUE(modTime - startTime <= 1);     // Give it 1S
    EXPECT_TRUE(Util::deleteFile(filename));
}

// Util::moveData() call back
static bool callback(const string &filename, float percentComplete, void *context) {
    LOGINF << filename << ": " << percentComplete << "%";
    return true;
}

static bool compareBlobAndFile(const Blob &b, FILE *fp) {
    EXPECT_EQ(0, ::fseek(fp, 0, SEEK_END));
    off_t length = (off_t)::ftello(fp);
    EXPECT_EQ(length, b.allocatedLength());
    EXPECT_EQ(0, ::fseek(fp, 0, SEEK_SET));
    for (off_t i = 0; i < length; i++) {
        uint8_t x;
        EXPECT_EQ(1, ::fread(&x, 1, 1, fp));
        EXPECT_EQ(x, *(b.data() + i));
    }

    return true;
}

TEST(UtilTest, moveData) {

    // Write test data
    string filename = Util::tempFilename("UtilTest_moveData");
    EXPECT_TRUE(!filename.empty());
    LOGINF << "Using file '" << filename << "'";
    ofstream f(filename);
    EXPECT_TRUE(f.good());
    for (unsigned i = 0; i < 1000; i++) {
        for (unsigned j = 0; j < 100; j++) {
            f << (char)('!' + (i % ('~' - '!')));
        }
    }
    f.close();

    // Read the last 5000 characters into memory so we can compare
    Blob b;
    EXPECT_TRUE(b.reserve(5000));
    FILE *fp = ::fopen(filename.c_str(), "r");
    EXPECT_TRUE(fp != 0);
    EXPECT_EQ(0, ::fseeko(fp, -5000, SEEK_END));
    size_t read = ::fread(b.data(), 1, 5000, fp);
    EXPECT_EQ(5000, read);
    EXPECT_TRUE(Util::moveData(filename, (1000 * 100) - (50 * 100), 50 * 100, 0, callback, 0));
    compareBlobAndFile(b, fp);
    ::fclose(fp);
}
