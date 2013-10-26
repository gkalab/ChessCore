//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Util.cpp: Utility class implementation.
//

#include <ChessCore/Util.h>
#include <ChessCore/Blob.h>
#include <ChessCore/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WINDOWS
#include <io.h>
#else // !WINDOWS
#include <sys/time.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#endif // WINDOWS

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <exception>

using namespace std;

namespace ChessCore {

const char *Util::m_classname = "Util";

#ifdef WINDOWS
// Convert FILETIME to seconds
// From: http://ix-notes.blogspot.co.uk/2008/03/filetime-to-timet.html

inline uint64_t fileTimeToSeconds(const FILETIME &ft) {
    uint64_t u64 = reinterpret_cast<const ULONGLONG&>(ft);
	u64 -= 116444736000000000ULL;
	u64 /= 10000000ULL;
	return u64;
}

#endif // WINDOWS

string Util::format(const char *fmt, ...) {
    char buffer[4096];
    va_list va;

    va_start(va, fmt);
    ::vsprintf(buffer, fmt, va);
    va_end(va);

    return string(buffer);
}

string Util::formatNPS(int64_t nodes, int64_t time) {
    if (time == 0)
        return "INF";
    int64_t nps = (nodes * 1000LL) / time;
    return format("%d.%03d Mnps", (int)(nps / 1000000LL), (int)((nps % 1000000LL) / 1000LL));
}

string Util::formatBB(uint64_t bb) {
    ostringstream oss;

    oss << "+---------------+" << endl;

    for (int rank = 7; rank >= 0; rank--) {
        oss << '|';

        for (int file = 0; file <= 7; file++) {
            oss << (bb & fileRankBit(file, rank) ? 'X' : '.');
            oss << '|';
        }

        oss << '\n';
    }

    oss << "+---------------+\n";
    return oss.str();
}

string Util::formatTime(bool timeOnly /*=false*/, bool compressed /*=false*/) {

#ifdef WINDOWS
    SYSTEMTIME st;
    ::GetLocalTime(&st);

    if (timeOnly) {
        return format(compressed ? "%02u%02u%02u%03u" : "%02u:%02u:%02u.%03u",
                 (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond, (unsigned)st.wMilliseconds);
    } else {
        if (compressed) {
            return format("%04u%02u%02u%02u%02u%02u%03u",
                 (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
                 (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond, (unsigned)st.wMilliseconds);
        } else {
            return format("%04u-%02u-%02u %02u:%02u:%02u.%03u",
                 (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
                 (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond, (unsigned)st.wMilliseconds);
        }
    }
#else // !WINDOWS
    struct timeval tv;
    struct tm tm;
    ::gettimeofday(&tv, 0);
    ::localtime_r(&tv.tv_sec, &tm);

    if (timeOnly) {
        return format(compressed ? "%02d%02d%02d%03u" : "%02d:%02d:%02d.%03u",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned)(tv.tv_usec / 1000));
    } else {
        if (compressed) {
            return format("%04d%02d%02d%02d%02d%02d%03u",
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned)(tv.tv_usec / 1000));
        } else {
            return format("%04d-%02d-%02d %02d:%02d:%02d.%03u",
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned)(tv.tv_usec / 1000));
        }
    }
#endif // WINDOWS
}

string Util::formatDatePGN() {

#ifdef WINDOWS
	SYSTEMTIME st;
	::GetLocalTime(&st);
	return format("%04u.%02u.%02u", (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay);
#else // !WINDOWS
    time_t now;
    tm tm;
    ::time(&now);
    ::localtime_r(&now, &tm);
    return format("%04u.%02u.%02u", (unsigned)(tm.tm_year + 1900), (unsigned)(tm.tm_mon + 1),
           (unsigned)(tm.tm_mday));
#endif // WINDOWS
}

string Util::formatElapsed(unsigned time) {
    unsigned hours = time / (1000 * 60 * 60);

    time %= 1000 * 60 * 60;
    unsigned mins = time / (1000 * 60);
    time %= 1000 * 60;
    unsigned secs = time / 1000;
    time %= 1000;

    if (hours > 0)
        return format("%u:%02u:%02u.%03u", hours, mins, secs, time);
    else if (mins > 0)
        return format("%u:%02u.%03u", mins, secs, time);
    else
        return format("%u.%03u", secs, time);
}

string Util::formatMilli(int milli) {
    bool negative = milli < 0;

    milli = abs(milli);

    if (negative)
        return format("-%d.%03d", (milli / 1000), (milli % 1000));
    else
        return format("%d.%03d", (milli / 1000), (milli % 1000));
}

string Util::formatCenti(int centi) {
    bool negative = centi < 0;

    centi = abs(centi);
    return format("%c%d.%02d", negative ? '-' : '+', (centi / 100), (centi % 100));
}

string Util::formatData(const uint8_t *data, unsigned length) {
    stringstream ss;

    ss << "length=" << length << " (0x" << hex << length << dec << ")" << endl;

    if (data) {
        unsigned i;
        const uint8_t *p = (const uint8_t *)data;
        const uint8_t *end = p + length;
        uint8_t b;

        while (p < end) {
            ss << setw(8) << setfill('0') << hex << (p - (const uint8_t *)data) << ": ";

            for (i = 0; i < 16 && p + i < end; i++)
                ss << setw(2) << setfill('0') << Util::hexChar(*(p + i)) << ' ';

            while (i < 16) {
                ss << "   ";
                i++;
            }

            for (i = 0; i < 16 && p + i < end; i++) {
                b = *(p + i);

                if (b >= 32 && b < 127)
                    ss << b;
                else
                    ss << '.';
            }

            ss << endl;
            p += 16;
        }
    }

    return ss.str();
}

unsigned Util::percent(unsigned value, unsigned total) {
    if (total == 0)
        return 0;
    return (value * 100) / total;
}

bool Util::parse(const string &str, int32_t &value) {
    // Determine the base to use when parsing
    int base = 10;
    bool negate = false;
    const char *p = str.c_str();

    if (*p == '-') {
        negate = true;
        p++;
    } else if (*p == '+')
        p++;

    // Cater for hex notation
    if (*p == '0' && ::tolower(*(p + 1)) == 'x') {
        base = 16;
        p += 2;
    }

    char *end;
    value = (int32_t)::strtol(p, &end, base);

    if (negate)
        value = -value;

    return *end == '\0';
}

bool Util::parse(const string &str, int64_t &value) {
    // Determine the base to use when parsing
    int base = 10;
    bool negate = false;
    const char *p = str.c_str();

    if (*p == '-') {
        negate = true;
        p++;
    } else if (*p == '+')
        p++;

    // Cater for hex notation
    if (*p == '0' && ::tolower(*(p + 1)) == 'x') {
        base = 16;
        p += 2;
    }

    char *end;
    value = (int64_t)::strtoll(p, &end, base);

    if (negate)
        value = -value;

    return *end == '\0';
}

bool Util::parse(const string &str, uint32_t &value) {
    // Determine the base to use when parsing
    int base = 10;
    const char *p = str.c_str();

    if (*p == '-')
        return false;

    // Cater for hex notation
    if (*p == '0' && ::tolower(*(p + 1)) == 'x') {
        base = 16;
        p += 2;
    }

    char *end;
    value = (uint32_t)::strtoul(p, &end, base);
    return *end == '\0';
}

bool Util::parse(const string &str, uint64_t &value) {
    // Determine the base to use when parsing
    int base = 10;
    const char *p = str.c_str();

    if (*p == '-')
        return false;

    // Cater for hex notation
    if (*p == '0' && ::tolower(*(p + 1)) == 'x') {
        base = 16;
        p += 2;
    }

    char *end;
    value = ::strtoull(p, &end, base);
    return *end == '\0';
}

bool Util::parse(const string &str, bool &value) {
    if (str.empty())
        return false;

    const char *p = str.c_str();

    if (strcasecmp(p, "true") == 0 || strcasecmp(p, "on") == 0 ||
        strcasecmp(p, "yes") == 0 || strcasecmp(p, "1") == 0) {
        value = true;
        return true;
    }

    if (strcasecmp(p, "false") == 0 || strcasecmp(p, "off") == 0 ||
        strcasecmp(p, "no") == 0 || strcasecmp(p, "0") == 0) {
        value = false;
        return true;
    }

    return false;
}

unsigned Util::getTickCount() {
#ifdef WINDOWS
	return ::GetTickCount();
#else // !WINDOWS
    struct timeval tv;
    ::gettimeofday(&tv, 0);
    return unsigned((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#endif // WINDOWS
}

uint64_t Util::currentTime() {
#ifdef WINDOWS
    SYSTEMTIME st;
    FILETIME ft;
    ::GetSystemTime(&st);
    ::SystemTimeToFileTime(&st, &ft);
    return fileTimeToSeconds(ft);
#else // !WINDOWS
    return (uint64_t)::time(0);
#endif
}

string Util::hexChar(uint8_t b) {
    const char *chars = "0123456789abcdef";
    string s;

    s.push_back(chars[(b >> 4) & 0xf]);
    s.push_back(chars[b & 0xf]);
    return s;
}

unsigned Util::splitLine(char *line, char **parts, unsigned maxParts) {
    unsigned numParts;
    char inQuotes = '\0';

    for (numParts = 0; numParts < maxParts; numParts++)
        parts[numParts] = 0;

    numParts = 0;

    while (numParts < maxParts) {
        while (*line != '\0' && isspace(*line))
            line++;

        if (*line == '\0')
            break;

        if (*line == '\'' || *line == '"')
            inQuotes = *line++;

        parts[numParts++] = line;

        if (inQuotes != '\0') {
            while (*line != '\0' && *line != inQuotes)
                line++;

            inQuotes = '\0';
        } else {
            while (*line != '\0' && !isspace(*line))
                line++;
        }

        if (*line == '\0')
            break;
        else
            *line++ = '\0';
    }

    return numParts;
}

unsigned Util::splitLine(const string &line, vector<string> &parts, char delmiter /*=' '*/) {
    char inQuotes = '\0';
    const char *start, *end;

    parts.clear();

    start = end = line.c_str();

    while (*end != '\0') {
        if (*start == '\0')
            break;

        if (*start == '\'' || *start == '"')
            inQuotes = *start++;

        end = start;

        if (inQuotes != '\0') {
            while (*end != '\0' && *end != inQuotes)
                end++;

            inQuotes = '\0';
        } else {
            while (*end != '\0' && *end != delmiter)
                end++;
        }

        if (end > start)
            parts.push_back(string(start, end - start));

        start = end + 1;
    }

    return (unsigned)parts.size();
}

void Util::trim(string &str) {
    str.erase(0, str.find_first_not_of(' '));
    str.erase(str.find_last_not_of(' ') + 1);
}

string Util::trim(const string &str) {
    string temp = str;
    temp.erase(0, temp.find_first_not_of(' '));
    temp.erase(temp.find_last_not_of(' ') + 1);
    return temp;
}

string Util::tolower(const string &str) {
    string temp = str;
    std::transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
    return temp;
}

string Util::toupper(const string &str) {
    string temp = str;
    std::transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp;
}

string Util::concat(const vector<string> &parts, unsigned startIndex, unsigned endIndex) {
    ostringstream oss;
    for (; startIndex < endIndex; startIndex++) {
        if (oss.tellp() > 0)
            oss << " ";
        oss << parts[startIndex];
    }
    string s = oss.str();
    return s;
}

bool Util::startsWith(string const &str, string const &starting, bool caseSensitive /*=true*/) {
    if (str.length() >= starting.length()) {
        string s = str.substr(0, starting.length());

        if (caseSensitive)
            return ::strcmp(s.c_str(), starting.c_str()) == 0;
        else
            return ::strcasecmp(s.c_str(), starting.c_str()) == 0;
    }

    return false;
}

bool Util::endsWith(string const &str, string const &ending, bool caseSensitive /*=true*/) {
    if (str.length() >= ending.length()) {
        string s = str.substr(str.length() - ending.length());

        if (caseSensitive)
            return ::strcmp(s.c_str(), ending.c_str()) == 0;
        else
            return ::strcasecmp(s.c_str(), ending.c_str()) == 0;
    }

    return false;
}

string Util::getEnv(const char *name) {
    string value;

    if (name && *name) {
        char *temp = ::getenv(name);

        if (temp && *temp)
            value.assign(temp);
    }

    return value;
}

string Util::expandEnv(const string &str) {
    stringstream ss;

    const char *p = str.c_str(), *namestart;
    string name, expanded;
    bool done = false;

    while (!done) {
        while (*p != '\0' && *p != '$')
            ss << *p++;

        done = *p == '\0';

        if (!done) {
            if (*(p + 1) == '(') {
                p += 2;
                namestart = p;

                while (*p != '\0' && *p != ')')
                    p++;

                done = *p == '\0';
                name.assign(namestart, p - namestart);
                p++;

                expanded = ::getenv(name.c_str());

                if (expanded.length() > 0)
                    ss << expanded;
            } else // *(p + 1) != '('
                ss << *p++;
        }
    }

    return ss.str();
}

uint64_t Util::size(istream &stream) {
    uint64_t length, current = stream.tellg();

    stream.seekg(0, ios::end);
    length = stream.tellg();
    stream.seekg(current, ios::beg);
    return length;
}

uint64_t Util::modifyTime(const string &filename) {

#ifdef WINDOWS

	uint64_t modTime = 0;
	HANDLE handle = ::CreateFile(filename.c_str(),
		0,						// Query device attributes without opening the file
		0,						// Not shared
		NULL,					// Security attributes
		OPEN_EXISTING,			// Don't create
		FILE_ATTRIBUTE_NORMAL,	// Flags and attributes
		NULL);
	if (handle != INVALID_HANDLE_VALUE) {
		FILETIME lastWriteTime;
		if (::GetFileTime(handle, NULL, NULL, &lastWriteTime)) {
			// Convert to seconds
            modTime = fileTimeToSeconds(lastWriteTime);
		} else {
			throw ChessCoreException("GetFileTime('%s') failed: %s", filename.c_str(), win32ErrorText(GetLastError()));
		}

		::CloseHandle(handle);
	}
	return modTime;
#else // !WINDOWS
	struct stat st;
    if (::stat(filename.c_str(), &st) < 0)
        return 0;
    return (uint64_t)(st.st_mtime);
#endif
}

bool Util::fileExists(const string &filename) {
#ifdef WINDOWS
	DWORD attribs = ::GetFileAttributes(filename.c_str());
	return attribs != INVALID_FILE_ATTRIBUTES &&
        (attribs & (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_NORMAL)) != 0;    // FILE_ATTRIBUTE_NORMAL bit isn't always set?!?
#else // !WINDOWS
    struct stat statbuf;
    return ::stat(filename.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode);
#endif // WINDOWS
}

bool Util::dirExists(const string &dir) {
#ifdef WINDOWS
	DWORD attribs = ::GetFileAttributes(dir.c_str());
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else // !WINDOWS
    struct stat statbuf;
    return ::stat(dir.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
#endif // WINDOWS
}

bool Util::createDirectory(const string &dir) {
    string::size_type pos = 0, index;
    do {
        index = dir.find("/\\", pos);
        string subdir = (index == string::npos) ? dir : dir.substr(0, index);
        if (!dirExists(subdir)) {
#ifdef WINDOWS
            if (!::CreateDirectory(subdir.c_str(), NULL)) {
                LOGERR << "Failed to create directory '" << subdir << "': " <<
                    win32ErrorText(GetLastError());
                return false;
            }
#else // !WINDOWS
            if (::mkdir(subdir.c_str(), 0755) < 0) {
                LOGERR << "Failed to create directory '" << subdir << "': " <<
                    strerror(errno) << " (" << errno << ")";
                return false;
            }
#endif // WINDOWS
        }
        pos = index;
    } while (index != string::npos);
    return true;
}

bool Util::canRead(const string &pathname) {
#ifdef WINDOWS
	return fileExists(pathname);
#else // !WINDOWS
    return ::access(pathname.c_str(), R_OK) == 0;
#endif // WINDOWS
}

bool Util::canWrite(const string &pathname) {
#ifdef WINDOWS
	DWORD attribs = ::GetFileAttributes(pathname.c_str());
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_READONLY) == 0;
#else // !WINDOWS
    return ::access(pathname.c_str(), W_OK) == 0;
#endif // WINDOWS
}

bool Util::canExecute(const string &pathname) {
#ifdef WINDOWS
	return fileExists(pathname);
#else // !WINDOWS
    return ::access(pathname.c_str(), X_OK) == 0;
#endif // WINDOWS
}

string Util::dirName(const string &filename) {
    size_t last = filename.find_last_of(PATHSEP);
    return filename.substr(0, last);
}

string Util::baseName(const string &filename) {
    size_t last = filename.find_last_of(PATHSEP);
    return filename.substr(last + 1);
}

bool Util::replaceExt(string &filename, const string &oldExt, const string &newExt) {
    if (!Util::endsWith(filename, oldExt, true))
        return false;

    filename = filename.substr(0, filename.length() - oldExt.length());
    filename += newExt;
    return true;
}

bool Util::replaceExt(const string &oldFilename, const string &oldExt, string &newFilename, const string &newExt) {
    if (!Util::endsWith(oldFilename, oldExt, true))
        return false;

    newFilename = oldFilename.substr(0, oldFilename.length() - oldExt.length());
    newFilename += newExt;
    return true;
}

string Util::tempFilename(const string &prefix) {
    string filename;
    if (!prefix.empty() && !g_tempDir.empty() && canWrite(g_tempDir)) {
        for (unsigned i = 0; i < 999999; i++) {
            filename = g_tempDir + PATHSEP + prefix + format("%06u.tmp", i);
            if (!fileExists(filename)) {
                break;
            }
        }
    }
    return filename;
}

bool Util::deleteFile(const string &filename) {
#ifdef WINDOWS
	return ::DeleteFile(filename.c_str()) == TRUE;
#else // !WINDOWS
    return ::unlink(filename.c_str()) == 0;
#endif // WINDOWS
}

bool Util::renameFile(const string &oldFilename, const string &newFilename) {
#ifdef WINDOWS
	return ::MoveFile(oldFilename.c_str(), newFilename.c_str()) == TRUE;
#else // !WINDOWS
    return ::rename(oldFilename.c_str(), newFilename.c_str()) == 0;
#endif // WINDOWS
}

bool Util::copyFile(const std::string &srcFilename, const std::string &dstFilename,
                    FILEOP_CALLBACK_FUNC callback /*=0*/, void *callbackContext /*=0*/) {
    bool retval = true;
    struct stat statbuf;

    FILE *src = ::fopen(srcFilename.c_str(), "r");
    if (src) {
        if (::fstat(::fileno(src), &statbuf) == 0) {
            FILE *dst = ::fopen(dstFilename.c_str(), "w+");
            if (dst) {
                uint8_t buffer[4096];
                int numRead;
                off_t totalRead = 0;
                do {
                    if (callback) {
                        float percentComplete = static_cast<float>((totalRead * 100) / (float)statbuf.st_size);
                        if (!callback(srcFilename, percentComplete, callbackContext)) {
                            LOGINF << "Copy operating cancelled by user";
                            retval = false;
                            break;
                        }
                    }
                    numRead = (int)::fread(buffer, 1, sizeof(buffer), src);
                    if (numRead > 0) {
                        if ((int)::fwrite(buffer, 1, numRead, dst) != numRead) {
                            LOGERR << "Error writing to destination file '" << dstFilename << "': " <<
                                strerror(errno) << " (" << errno << ")";
                            retval = false;
                        }
                    } else if (numRead < 0) {
                        LOGERR << "Error reading from source file '" << srcFilename << "': " <<
                            strerror(errno) << " (" << errno << ")";
                        retval = false;
                    }
                    totalRead += (off_t)numRead;
                } while (retval && numRead < sizeof(buffer));

                ::fclose(dst);

            } else {
                LOGERR << "Failed to open destination file '" << dstFilename << "' :" <<
                    strerror(errno) << " (" << errno << ")";
                retval = false;
            }
        } else {
            LOGERR << "Failed to stat source file '" << srcFilename << "': " <<
                strerror(errno) << " (" << errno << ")";
        }
        ::fclose(src);

    } else {
        LOGERR << "Failed to open source file '" << srcFilename << "' :" <<
            strerror(errno) << " (" << errno << ")";
        retval = false;
    }

    return retval;
}

bool Util::moveData(string &filename, uint64_t fromOffset, unsigned length, uint64_t toOffset,
                    FILEOP_CALLBACK_FUNC callback /*=0*/, void *callbackContext /*=0*/) {

    if (toOffset == fromOffset) {
        LOGWRN << "toOffset == fromOffset (" << hex << toOffset << ")";
        return true;
    }

    bool retval = true, copyingForward = (toOffset < fromOffset);
    uint64_t eofOffset;
    Blob buffer;
    unsigned bufferSize = min(length, 4096U);
    unsigned originalLength = length, totalMoved = 0;

    if (!buffer.reserve(bufferSize))
        throw ChessCoreException("Failed to reserve buffer (size=%u)", bufferSize);
    
    FILE *fp = ::fopen(filename.c_str(), "r+");
    if (fp) {
        if (::fseeko(fp, 0, SEEK_END) == 0)
        {
            eofOffset = ::ftello(fp);

            if (!copyingForward) {
                fromOffset = (fromOffset + length) - bufferSize;
                toOffset = (toOffset + length) - bufferSize;
            }

            while (length && retval) {
                if (callback) {
                    float percentComplete = static_cast<float>((totalMoved * 100) / originalLength);
                    if (!callback(filename, percentComplete, callbackContext)) {
                        LOGINF << "Move operation cancelled by user";
                        retval = false;
                        break;
                    }
                }
                unsigned readLen = min(bufferSize, length);
                if (::fseeko(fp, fromOffset, SEEK_SET) == 0) {
                    if (::fread(buffer.data(), 1, readLen, fp) == readLen) {
                        if (::fseeko(fp, toOffset, SEEK_SET) == 0) {
                            if (::fwrite(buffer.data(), 1, readLen, fp) == readLen) {
                                if (copyingForward) {
                                    fromOffset += readLen;
                                    toOffset += readLen;
                                } else {
                                    fromOffset -= readLen;
                                    toOffset -= readLen;
                                }
                                length -= readLen;
                            } else {
                                LOGERR << "Failed to write " << readLen << " bytes to offset 0x" << hex << toOffset <<
                                    " of file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
                                retval = false;
                            }
                        } else {
                            LOGERR << "Failed to seek to offset 0x" << hex << toOffset <<
                                " of file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
                            retval = false;
                        }
                    } else {
                        LOGERR << "Failed to read " << readLen << " bytes from offset 0x" << hex << fromOffset <<
                            " of file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
                        retval = false;
                    }
                } else {
                    LOGERR << "Failed to seek to offset 0x" << hex << fromOffset <<
                        " of file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
                    retval = false;
                }
                totalMoved += readLen;
            }

            if (copyingForward && retval && fromOffset == eofOffset) {
                // Shrink the file as we have contracted
                if (!truncateFile(fp, toOffset)) {
                    LOGERR << "Failed to truncate file to offset 0x" << hex << toOffset;
                    retval = false;
                }
            }
 
        } else {
            LOGERR << "Failed to seek to end of file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
            retval = false;
        }

        ::fclose(fp);

    } else {
        LOGERR << "Failed to open file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
        retval = false;
    }

    return retval;
}

bool Util::truncateFile(FILE *fp, uint64_t length) {
#ifdef WINDOWS
    int fd = ::fileno(fp);
    if (fd < 0) {
        LOGERR << "Invalid file descriptor: " << fd;
        return false;
    }

    bool retval = false;
    LARGE_INTEGER currOffset, offset;
    HANDLE hfile = (HANDLE)::_get_osfhandle(fd);
    offset.QuadPart = 0;
    // Get the current file pointer offset
    if (::SetFilePointerEx(hfile, offset, &currOffset, FILE_CURRENT)) {
        // Seek to the desired end-of-file
        offset.QuadPart = length;
        if (::SetFilePointerEx(hfile, offset, 0, FILE_BEGIN)) {
            // Truncate the file
            if (::SetEndOfFile(hfile)) {
                // Return the file pointer to its previous offset
                if (currOffset.QuadPart < offset.QuadPart) {
                    if (::SetFilePointerEx(hfile, currOffset, 0, FILE_BEGIN)) {
                        retval = true;
                    } else {
                        LOGERR << "Failed to return file pointer to offset 0x" << hex << currOffset.QuadPart << ": " << win32ErrorText(GetLastError());
                    }
                }
            } else {
                LOGERR << "Failed to truncate file to 0x" << hex << length << " bytes: " << win32ErrorText(GetLastError());
            }
        } else {
            LOGERR << "Failed to seek to offset 0x" << hex << length << ": " << win32ErrorText(GetLastError());
        }
    } else {
        LOGERR << "Failed to get current file pointer offset: " << win32ErrorText(GetLastError());
    }

    return retval;
#else // !WINDOWS

    int fd = ::fileno(fp);
    if (fd < 0) {
        LOGERR << "Invalid file descriptor: " << fd;
        return false;
    }

    if (::ftruncate(fd, length) < 0) {
        LOGERR << "Failed to truncate file: " << strerror(errno) << " (" << errno << ")";
        return false;
    }

    return true;
#endif // WINDOWS
}

string Util::getUniqueName(const string &filename) {
#ifdef WINDOWS
    bool retval = false;
    BY_HANDLE_FILE_INFORMATION fileinfo;
    if (!win32GetFileInfo(filename, &fileInfo))
        return "";

    return
        HexFormat<DWORD>::fullFormat(fileInfo.dwVolumeSerialNumber) +
        HexFormat<DWORD>::fullFormat(fileInfo.nFileIndexHigh) +
        HexFormat<DWORD>::fullFormat(fileInfo.nFileIndexLow);
    }

#else // !WINDOWS

    struct stat statbuf;
    if (::stat(filename.c_str(), &statbuf) < 0) {
        LOGERR << "Failed to stat file '" << filename << "': " << strerror(errno) << " (" << errno << ")";
        return "";
    }

    return
        HexFormat<dev_t>::fullFormat(statbuf.st_dev) +
        HexFormat<ino_t>::fullFormat(statbuf.st_ino);

#endif // WINDOWS
}

bool Util::sameFile(const string &filename1, const string &filename2) {

    // Quick win
    if (filename1 == filename2)
        return true;

#ifdef WINDOWS
    BY_HANDLE_FILE_INFORMATION fileInfo1, fileInfo2;
    if (win32GetFileInfo(filename1, &fileInfo1) &&
        win32GetFileInfo(filename2, &fileInfo2)) {
        return
            fileInfo1.dwVolumeSerialNumber == fileInfo2.dwVolumeSerialNumber &&
            fileInfo1.nFileIndexHigh == fileInfo2.nFileIndexHigh &&
            fileInfo1.nFileIndexLow == fileInfo2.nFileIndexLow;
    }
    return false;
#else // !WINDOWS
    struct stat statbuf1, statbuf2;
    if (::stat(filename1.c_str(), &statbuf1) == 0) {
        if (::stat(filename2.c_str(), &statbuf2) == 0) {
            return
                statbuf1.st_dev == statbuf2.st_dev &&
                statbuf1.st_ino == statbuf2.st_ino;
        } else {
            LOGERR << "Failed to stat file '" << filename2 << "': " << strerror(errno) << " (" << errno << ")";
        }
    } else {
        LOGERR << "Failed to stat file '" << filename1 << "': " << strerror(errno) << " (" << errno << ")";
    }

    return false;

#endif // WINDOWS
}

#ifdef WINDOWS

bool Util::win32GetFileInfo(const string &filename, BY_HANDLE_FILE_INFORMATION *fileInfo) {
    bool retval = false;
    HANDLE h = ::CreateFile(filename.c_str(), 0, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h != INVALID_HANDLE_VALUE) {
        if (GetFileInformationByHandle(h, fileInfo)) {
            retval = true;
        } else {
            LOGERR << "Failed to get information for file '" << filename << "': " << win32ErrorText(GetLastError());
        }
        ::CloseHandle(h);
    } else {
        LOGERR << "Failed to open file '" << filename << "': " << win32ErrorText(GetLastError());
        retval = false;
    }
    return retval;
}

string Util::win32ErrorText(DWORD errorCode) {
    string message;
    LPSTR errorText = 0;

    ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errorText,
        0,
        NULL);
    if (errorText)
    {
        message = errorText;
        LocalFree(errorText);
        errorText = NULL;
    }
    return message;
}

bool Util::win32DuplicateHandle(HANDLE in, HANDLE &out) {

	return ::DuplicateHandle(
		::GetCurrentProcess(),  // Process's handle to be duplicated
		in,                     // Input handle
		::GetCurrentProcess(),  // Process to receive the duplicated handle
		&out,					// Duplicated handle process receiver
		0,						// Access for the duplicated/new handle
		TRUE,                   // Inherited
		DUPLICATE_SAME_ACCESS   // Same access as the source handle
        ) == TRUE;
}

#endif // WINDOWS

void Util::testingThrowChessCoreException() {
    throw ChessCoreException("This is only a test");
}

void Util::testingThrowCppException() {
    throw runtime_error("This is only a test");
}

void Util::testingDerefNullPointer() {
    int a = 12;
    int *p = &a;
    p = 0;
    *p = 34;
}

}   // namespace ChessCore
