//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// GameHeader.cpp: Game Header class implementation.
//

#include <ChessCore/GameHeader.h>
#include <ChessCore/Util.h>

#ifndef WINDOWS
#include <unistd.h>
#include <sys/utsname.h>
#endif // !WINDOWS

#include <sstream>
#include <iomanip>

using namespace std;

namespace ChessCore {
const char *GameHeader::m_classname = "GameHeader";

GameHeader::GameHeader() {
    initHeader();
}

GameHeader::~GameHeader() {
}

void GameHeader::initHeader() {
    m_white.clear();
    m_black.clear();
    m_event.clear();
    m_site.clear();
    m_day = m_month = m_year = 0;
    m_roundMajor = m_roundMinor = 0;
    m_result = UNFINISHED;
    m_timeControl.clear();
    m_eco.clear();
    m_readFail = false;
}

void GameHeader::setHeader(const GameHeader &other) {
    m_white = other.m_white;
    m_black = other.m_black;
    m_event = other.m_event;
    m_site = other.m_site;
    m_annotator = other.m_annotator;
    m_day = other.m_day;
    m_month = other.m_month;
    m_year = other.m_year;
    m_roundMajor = other.m_roundMajor;
    m_roundMinor = other.m_roundMinor;
    m_result = other.m_result;
    m_timeControl = other.m_timeControl;
    m_eco = other.m_eco;
    m_readFail = other.m_readFail;
}

void GameHeader::setHeader(const GameHeader *other) {
    if (other == 0)
        return;

    m_white = other->m_white;
    m_black = other->m_black;
    m_event = other->m_event;
    m_site = other->m_site;
    m_annotator = other->m_annotator;
    m_day = other->m_day;
    m_month = other->m_month;
    m_year = other->m_year;
    m_roundMajor = other->m_roundMajor;
    m_roundMinor = other->m_roundMinor;
    m_result = other->m_result;
    m_timeControl = other->m_timeControl;
    m_eco = other->m_eco;
    m_readFail = other->m_readFail;
}

bool GameHeader::setSiteComputer() {

    char name[128];

#ifdef WINDOWS

	DWORD size = sizeof(name);
	if (!GetComputerName(name, &size))
		return false;

#else // !WINDOWS

	if (gethostname(name, sizeof(name)) < 0)
        return false;

#endif // WINDOWS

    m_site = Util::format("Computer '%s'", name);
    return true;
}

bool GameHeader::setDateNow() {

#ifdef WINDOWS

	SYSTEMTIME st;
	GetLocalTime(&st);
	m_day = (int)st.wDay;
	m_month = (int)st.wMonth;
	m_year = (int)st.wYear;

#else // !WINDOWS

    time_t now;
    tm tm;

    time(&now);
    localtime_r(&now, &tm);
    m_day = tm.tm_mday;
    m_month = tm.tm_mon + 1;
    m_year = tm.tm_year + 1900;

#endif // !WINDOWS
    return true;
}

//
// TODO: Return string
//
void GameHeader::format(string &str, const string &unknownWord, bool forFilename) const {
    ostringstream oss;

    if (m_white.hasName())
        oss << m_white.formattedName(forFilename);
    else
        oss << unknownWord;

    if (forFilename)
        oss << "-";
    else
        oss << " - ";

    if (m_black.hasName())
        oss << m_black.formattedName(forFilename);
    else
        oss << unknownWord;

    if (!m_event.empty()) {
        if (forFilename)
            oss << "-";
        else
            oss << ", ";

        oss << m_event;
    }

    if (!m_site.empty()) {
        if (forFilename)
            oss << "-";
        else
            oss << ", ";

        oss << m_site;
    }

    if (m_year > 0) {
        if (forFilename)
            oss << "-";
        else
            oss << ", ";

        oss << setfill('0') << setw(4) << m_year;

        if (m_month > 0) {
            oss << "-" << setfill('0') << setw(2) << m_month;

            if (m_day > 0)
                oss << "-" << setfill('0') << setw(2) << m_day;
        }
    }

    if (!forFilename && !m_eco.empty())
        oss << " " << m_eco;

    str.assign(oss.str());
}

ostream &operator<<(ostream &os, const GameHeader &gameHeader) {
    string str;
    gameHeader.format(str, "Unknown", false);
    os << str;
    return os;
}

}   // namespace ChessCore
