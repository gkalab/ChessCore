#include <ChessCore/Log.h>
#include <ChessCore/Lowlevel.h>
#include <gtest/gtest.h>
#include <regex>
#include <exception>

using namespace std;
using namespace ChessCore;

static const char *m_classname = "";

GTEST_API_ int main(int argc, char **argv) {
    int retval = 1;
    try {
        cout << "Initialising ChessCore" << endl;
        if (ChessCore::init()) {

#ifdef USE_ASL_LOGGING
            Log::open("com.trojanfoe.chesscore.unittests");
#else // !USE_ASL_LOGGING
            string logfile = g_tempDir + PATHSEP + "unittests.log";
            if (Log::open(logfile, false))
                cout << "Logging to '" << logfile << "'" << endl;
            else
                cerr << "Failed to open logfile '" << logfile << "'" << endl;
#endif // USE_ASL_LOGGING

            Log::setAllowDebug(true);
            LOGDBG << "Using CPU POPCNT instruction: " << boolalpha << usingCpuPopcnt();

            cout << "Initialising gtest" << endl;
            testing::InitGoogleTest(&argc, argv);

            cout << "Running tests" << endl;
            retval = RUN_ALL_TESTS();
        } else {
            cerr << "Failed to initialise ChessCore" << endl;
        }

    } catch(ChessCoreException &e) {
        cerr << "ChessCore exception: " << e.what() << endl;
    } catch(std::exception &e) {
        cerr << "Runtime exception: " << e.what() << endl;
    } catch(...) {
        cerr << "Unknown exception" << endl;
    }

    ChessCore::fini();
    return retval;
}

// Invoke using EXPECT_TRUE()
testing::AssertionResult regexMatch(const string &toMatch, const string &regexStr) {
    regex r(regexStr);
    if (regex_match(toMatch.begin(), toMatch.end(), r))
        return testing::AssertionSuccess();
    return testing::AssertionFailure() << "Regex match failure. '" << toMatch << "' != '" << regexStr << "'";
}
