//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Util.h: Utility class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Lowlevel.h>
#include <ChessCore/Data.h>

#include <vector>
#include <istream>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifndef WINDOWS
#include <time.h>
#endif // !WINDOWS

namespace ChessCore {

extern "C"
{
    /**
     * Callback function for long file operations.
     *
     * @param filename The file being manipulated (the source file if the operation is file copy).
     * @param percentComplete Processing progress. (0.0 to 100.0).
     * @param contextInfo Context Info passed to the database method.
     *
     * @return false to terminate processing, else true.
     */
    typedef bool (*FILEOP_CALLBACK_FUNC)(const std::string &filename, float percentComplete, void *contextInfo);
}

// Utility macros
#define OUT_UINT64(x) "0x" << setw(16) << setfill('0') << hex << x << dec

class CHESSCORE_EXPORT Util {
private:
    static const char *m_classname;

public:
    /**
     * Format a string object using printf-style formatting.
     *
     * @param fmt printf-style formatting string.
     *
     * @return The formatted string.
     */
    static std::string format(const char *fmt, ...);

    /**
     * Format Nodes-per-seconds.
     *
     * @param nodes Number of nodes processed.
     * @param time Number of milliseconds taken to process the nodes.
     *
     * @return The formatted Nodes-per-second.
     */
    static std::string formatNPS(int64_t nodes, int64_t time);

    /**
     * Format a bitboard.
     *
     * @param bb Bitboard.
     *
     * @return The formatted bitboard.
     */
    static std::string formatBB(uint64_t bb);

    /**
     * Format current date and/or time.
     *
     * @param timeOnly If true then only format the current time.
     * @param compressed If true then format without any delimiters.
     * else format the date and time.
     *
     * @return The formatted current date and/or time.
     */
    static std::string formatTime(bool timeOnly = false, bool compressed = false);

    /**
     * Format current date in PGN format.
     *
     * @return The current date in PGN formatt.
     */
    static std::string formatDatePGN();

    /**
     * Format millisecond elasped time.
     *
     * @param time Time to format.
     *
     * @return The formatted elapsed time.
     */
    static std::string formatElapsed(unsigned time);

    /**
     * Format a 'milli' integer value in float format
     *
     * @param milli Milli value.
     *
     * @return The formatted value.
     */
    static std::string formatMilli(int milli);

    /**
     * Format a 'centi' integer value in float format
     *
     * @param centi Centi value.
     *
     * @return The formatted value.
     */
    static std::string formatCenti(int centi);

    /**
     * Format a hex dump of binary data.
     *
     * @param data The start of the data.
     * @param len The length of the data.
     *
     * @return The hex dump.
     */
    static std::string formatData(const uint8_t *data, unsigned len);

    /**
     * Calculate the percentage value.
     *
     * @param value The value to calculate.
     * @param total The total value.
     *
     * @return The percentage of a value to a total value.
     */
    static unsigned percent(unsigned value, unsigned total);

    /**
     * Parse a 32-bit signed integer value.
     *
     * @param str The string to parse.
     * @param value reference to returned, parsed, value.
     *
     * @return true if the value was parsed successfully, else false.
     */
    static bool parse(const std::string &str, int32_t &value);

    /**
     * Parse a 64-bit signed integer value.
     *
     * @param str The string parse.
     * @param value reference to returned, parsed, value.
     *
     * @return true if the value was parsed successfully, else false.
     */
    static bool parse(const std::string &str, int64_t &value);

    /**
     * Parse a 32-bit unsigned integer value.
     *
     * @param str The string to parse.
     * @param value reference to returned, parsed, value.
     *
     * @return true if the value was parsed successfully, else false.
     */
    static bool parse(const std::string &str, uint32_t &value);

    /**
     * Parse a 64-bit unsigned integer value.
     *
     * @param str The string to parse.
     * @param value reference to returned, parsed, value.
     *
     * @return true if the value was parsed successfully, else false.
     */
    static bool parse(const std::string &str, uint64_t &value);

    /**
     * Parse a boolean value.
     *
     * @param str The string to parse.
     * @param value reference to returned, parsed, value.
     *
     * @return true if the value was parsed successfully, else false.
     */
    static bool parse(const std::string &str, bool &value);

    /**
     * Get the current 'tick count' (running system timer).
     *
     * @return Tick count value.
     */
    static unsigned getTickCount();

    /**
     * Get the current time.
     *
     * @return The current time.
     *         UNIX: seconds since the UNIX epoch (01-Jan-1970).
     *         Windows: 
     */
    static uint64_t currentTime();

    /**
     * Sleep for the specified length of time.
     *
     * @param time The number of milliseconds to sleep.
     */
    static inline void sleep(unsigned time) {
#ifdef WINDOWS
		Sleep(time);
#else // !WINDOWS
        ::usleep(time * 1000);
#endif // WINDOWS
    }

    /**
     * Get the hex representation of the specified byte.
     *
     * @param b The byte to get the hex representation of.
     *
     * @return The hex representation of the byte.
     */
    static std::string hexChar(uint8_t b);

    /**
     * Split a line of input into words.
     *
     * @param line Line to parse.  This buffer is modified to terminate each word
     * found within it.
     * @param parts Array of pointers to the start of each word found.
     * @param maxParts Maximum number of pointers that can be stored in the parts array.
     *
     * @return The number of words found.
     */
    static unsigned splitLine(char *line, char **parts, unsigned maxParts);

    /**
     * Split a line of input into words.
     *
     * @param line Line to parse.
     * @param parts A vector of strings containing each word found.
     *
     * @return The number of words found.
     */
    static unsigned splitLine(const std::string &line, std::vector<std::string> &parts);

    /**
     * Trim leading and trailing whitespace from a string.
     *
     * @param str The string to trim.
     */
    static void trim(std::string &str);

    /**
     * Trim leading and trailing whitespace from a string.
     *
     * @param str The string to trim.
     *
     * @return The trimmed string.
     */
    static std::string trim(const std::string &str);

    /**
     * Convert all characters in a string to lowercase.
     *
     * @param str The string to convert.
     *
     * @return The string converted to lowercase.
     */
    static std::string tolower(const std::string &str);

    /**
     * Convert all characters in a string to uppercase.
     *
     * @param str The string to convert.
     *
     * @return The string converted to uppercase.
     */
    static std::string toupper(const std::string &str);

    /**
     * Concatenate multiple strings into a single string.
     *
     * @param parts A vector of each string to concatenate.
     * @param startIndex The index of the first string, in parts, to concatenate.
     * @param endIndex (one past) the index of the last string, in parts, to concatenate.
     *
     * @return The concatenated string.
     */
    static std::string concat(const std::vector<std::string> &parts, unsigned startIndex, unsigned endIndex);

    /**
     * Determine if string starts with a particular string.
     *
     * @param str string to determine the end of.
     * @param starting The starting string.
     * @param caseSensitive If true then perform a case-sensitive comparison,
     * else if false, perform a case-insensitive comparison.
     *
     * @return true if 'str' starts with starting string, else false.
     */
    static bool startsWith(std::string const &str, std::string const &starting, bool caseSensitive = true);

    /**
     * Determine if string ends with a particular string.
     *
     * @param str string to determine the end of.
     * @param ending The ending string.
     * @param caseSensitive If true then perform a case-sensitive comparison,
     * else if false, perform a case-insensitive comparison.
     *
     * @return true if 'str' ends with ending string, else false.
     */
    static bool endsWith(std::string const &str, std::string const &ending, bool caseSensitive = true);

    /**
     * Get the value of an environment variable.
     *
     * @param name The name of the environment variable.
     *
     * @return The value of the environment variable, which is empty if
     * the environment variable was not set.
     */
    static std::string getEnv(const char *name);

    /**
     * Return astd::stringcontaining expanded environment variables.  The environment
     * variables are referenced using the syntax $(ENV).
     *
     * @param str Thestd::stringto expand.
     *
     * @return The expanded string.
     */
    static std::string expandEnv(const std::string &str);

    /**
     * Get the size of a stream.
     *
     * @param stream The istream to get the length of.
     *
     * @return The size of the stream.
     */
    static uint64_t size(std::istream &stream);

    /**
     * Get the modification file of the specified file.
     *
     * @param filename The file to get the modification time for.
     *
     * @return The modification time, else 0 if an error occurred.
     */
    static uint64_t modifyTime(const std::string &filename);

    /**
     * Check that a file exists
     *
     * @param filename The name of the file to check.
     *
     * @return true if the file exists, else false.
     */
    static bool fileExists(const std::string &filename);

    /**
     * Check that a directory exists
     *
     * @param dir The name of the directory to check.
     *
     * @return true if the directory exists, else false.
     */
    static bool dirExists(const std::string &dir);

    /**
     * Create the specified directory, including any intermedia directories.
     *
     * @param dir The directory to create.
     *
     * @return true if the directory was created successfully, else false.
     */
    static bool createDirectory(const std::string &dir);

    /**
     * Check of the specified file or directory can be read from.
     *
     * @param pathname The file or directory to check.
     *
     * @return true if the file or directory can be read from, else false.
     */
    static bool canRead(const std::string &pathname);

    /**
     * Check of the specified file or directory can be written to.
     *
     * @param pathname The file or directory to check.
     *
     * @return true if the file or directory can be written to, else false.
     */
    static bool canWrite(const std::string &pathname);

    /**
     * Check if the specified file or directory can be executed.
     *
     * @param pathname The file or directory to check.
     *
     * @return true if the file or directory can be executed, else false.
     */
    static bool canExecute(const std::string &pathname);

    /**
     * Get the directory of the specified file.
     *
     * @param filename The file to get the directory of.
     *
     * @return The directory of the specified file, including the trailing
     * path separator.
     */
    static std::string dirName(const std::string &filename);

    /**
     * Get the base filename of the specified file.
     *
     * @param filename The file to get the basename of.
     *
     * @return The filename of the specified file.
     */
    static std::string baseName(const std::string &filename);

    /**
     * Replace the file extension of the specified file.
     *
     * @param filename The filename to replace the extension of.
     * @param oldExt The old file extension.
     * @param newExt The new file extension.
     *
     * @return true if the file extension was replaced, else false.
     */
    static bool replaceExt(std::string &filename, const std::string &oldExt, const std::string &newExt);

    /**
     * Create a new filename, replacing the file extension.
     *
     * @param oldFilename The filename to replace the extension of.
     * @param oldExt The old file extension.
     * @param newFilename Where to store the new filename.
     * @param newExt The new file extension.
     *
     * @return true if the file extension was replaced, else false.
     */
    static bool replaceExt(const std::string &oldFilename, const std::string &oldExt, std::string &newFilename,
                           const std::string &newExt);

    /**
     * Return a filename in the temporary directory (as stored in g_tempDir).
     *
     * @param prefix The filename name prefix.
     *
     * @return The temporary filename.  This is empty if an error occurred.
     */
    static std::string tempFilename(const std::string &prefix);

    /**
     * Delete the specified file.
     *
     * @param filename The name of the file to delete.
     *
     * @return true if the file was deleted successfully, else false.
     */
    static bool deleteFile(const std::string &filename);

    /**
     * Rename a file.
     *
     * @param oldFilename The name of the file to move.
     * @param newFilename The new name of the file.
     *
     * @return true if the file was moved successfully, else false.
     */
    static bool renameFile(const std::string &oldFilename, const std::string &newFilename);

    /**
     * Copy a file.
     *
     * @param srcFilename The name of the source file.
     * @param dstFilename The name of the destination file.
     * @param callback Optional callback function.
     * @param callbackContext The context pointer to pass back in the callback.
     *
     * @return true if the file was copied successfully, else false.
     */
    static bool copyFile(const std::string &srcFilename, const std::string &dstFilename,
                         FILEOP_CALLBACK_FUNC callback = 0, void *callbackContext = 0);

    /**
     * Move data within a file.  If the data being moved extends to the end of the file
     * then the file is truncated to the new end-of-file.
     *
     * NOTE: Ensure C++ iostream objects are closed between operations!
     *
     * @param filename The name of the file to move data within.
     * @param fromOffset The offset to move the data from.
     * @param length The number of bytes to move.
     * @param toOffset The offset to move the data to.
     * @param callback Optional callback function.
     * @param callbackContext The context pointer to pass back in the callback.
     *
     * @return true if the data was moved successfully, else false.
     */
    static bool moveData(std::string &filename, uint64_t fromOffset, unsigned length, uint64_t toOffset,
                         FILEOP_CALLBACK_FUNC callback = 0, void *callbackContext = 0);

    /**
     * Truncate an open file to specified length.
     *
     * @param fp The file-pointer of the file to truncate.
     * @param length The length of the file.
     *
     * @return true if the file was truncated successfully, else false.
     */
    static bool truncateFile(FILE *fp, uint64_t length);

    /**
     * Return a string that uniqueuly identifies the file, typically related to the device
     * and filesystem info of the file.
     *
     * @param filename The file to get the unique name of.
     *
     * @return The unique name of the file, or an empty string if an error occurs.
     */
    static std::string getUniqueName(const std::string &filename);

#ifdef WINDOWS
	/**
	 * Get a description of the Win32 error code.
	 *
	 * @param errorCode The Win32 error code.
	 *
	 * @return A description of the Win32 error code.
	 */
	static std::string win32ErrorText(DWORD errorCode);

	/**
	 * Duplicate a handle.
	 *
	 * @param in The handle to duplicate.
	 * @param out The duplicated handle.
	 *
	 * @return true if the handle was duplicated successfully, else false.
	 */
	static bool win32DuplicateHandle(HANDLE in, HANDLE &out);
#endif // WINDOWS

    //
    // Magic Bitboard slider generation functions.
    //
    static inline uint64_t magicBishopAttacks(uint8_t offset, uint64_t occupy) {
        return magicBishopAtkMasks[magicBishopIndex[offset] +
            (((occupy & magicBishopMask[offset]) * magicBishopMult[offset]) >> magicBishopShift[offset])];
    }

    static inline uint64_t magicRookAttacks(uint8_t offset, uint64_t occupy) {
        return magicRookAtkMasks[magicRookIndex[offset] +
            (((occupy & magicRookMask[offset]) * magicRookMult[offset]) >> magicRookShift[offset])];
    }

    static inline uint64_t magicQueenAttacks(uint8_t offset, uint64_t occupy) {
        return magicBishopAttacks(offset, occupy) | magicRookAttacks(offset, occupy);
    }
};

#ifdef LITTLE_ENDIAN
inline uint16_t le16(uint16_t x) {
    return x;
}

inline uint32_t le32(uint32_t x) {
    return x;
}

inline uint64_t le64(uint64_t x) {
    return x;
}

inline uint16_t be16(uint16_t x) {
    return bswap16(x);
}

inline uint32_t be32(uint32_t x) {
    return bswap32(x);
}

inline uint64_t be64(uint64_t x) {
    return bswap64(x);
}

#else // !LITTLE_ENDIAN

inline uint16_t le16(uint16_t x) {
    return bswap16(x);
}

inline uint32_t le32(uint32_t x) {
    return bswap32(x);
}

inline uint64_t le64(uint64_t x) {
    return bswap64(x);
}

inline uint16_t be16(uint16_t x) {
    return x;
}

inline uint32_t be32(uint32_t x) {
    return x;
}

inline uint64_t be64(uint64_t x) {
    return x;
}
#endif // LITTLE_ENDIAN

//
// Stream utility class
//
template <typename T>
class StreamUtil {
public:
    /**
     * Read a primitive type value from the specified stream.
     *
     * @param stream The stream to read from.
     * @param value Where to store the value.
     *
     * @return true if the value was read successfully, else false.
     */
    static bool read(std::istream &stream, T &value) {
        union {
            char c[sizeof(T)];
            T i;
        } u;
        stream.read(u.c, sizeof(u.c));
        value = u.i;
        return !stream.bad() && !stream.fail();
    }

    /**
     * Write a primitive type value to the specified stream.
     *
     * @param stream The stream to read from.
     * @param value Where to store the value.
     *
     * @return true if the value was read successfully, else false.
     */
    static bool write(std::ostream &stream, T value) {
        union {
            char c[sizeof(T)];
            T i;
        } u;
        u.i = value;
        stream.write(u.c, sizeof(u.c));
        return !stream.bad() && !stream.fail();
    }
};

//
// Byte buffer packing utility class.
//
template <typename T>
class PackUtil {
public:
    /**
     * Get an integer from a byte buffer encoded using little endian.
     *
     * @param buffer Byte buffer.
     * @param length Number of bytes to get.
     *
     * @return Integer value.
     */
    static T little(const uint8_t *buffer, unsigned length) {
        T value = 0;

        while (length > 0) {
            value <<= 8;
            value |= buffer[--length];
        }

        return value;
    }

    /**
     * Put an integer into a byte buffer encoded using little endian.
     *
     * @param value The integer to put.
     * @param buffer Byte buffer.
     * @param length Number of bytes to put.
     */
    static void little(T value, uint8_t *buffer, unsigned length) {
        while (length-- > 0) {
            *buffer++ = value & 0xff;
            value >>= 8;
        }
    }

    /**
     * Get an integer from a byte buffer encoded using big endian.
     *
     * @param buffer Byte buffer.
     * @param length Number of bytes to get.
     *
     * @return Integer value.
     */
    static T big(const uint8_t *buffer, unsigned length) {
        T value = 0;

        while (length-- > 0) {
            value <<= 8;
            value |= *buffer++;
        }

        return value;
    }

    /**
     * Put an integer into a byte buffer encoded using big endian.
     *
     * @param value The integer to put.
     * @param buffer Byte buffer.
     * @param length Number of bytes to put.
     */
    static void big(T value, uint8_t *buffer, unsigned length) {
        while (length > 0) {
            buffer[--length] = value & 0xff;
            value >>= 8;
        }
    }
};

//
// Format an integer type as a hex string to its full size
//
template <typename T>
class HexFormat {
public:
    static std::string fullFormat(T value) {
        std::ostringstream oss;
        oss << std::setw(sizeof(value) * 2) << std::setfill('0') << std::hex << value;
        return oss.str();
    }
};

} // namespace ChessCore
