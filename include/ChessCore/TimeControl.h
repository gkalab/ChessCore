//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// TimeControl.h: TimeControl class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Blob.h>
#include <vector>
#include <algorithm>

namespace ChessCore {

class CHESSCORE_EXPORT TimeControlPeriod {

public:
    enum {
        MAX_MOVES = 200,
        MAX_TIME = 4 * 60 * 60,
        MAX_INCREMENT = 10 * 60
    };

private:
    static const char *m_classname;

public:
    enum Type {
        TYPE_NONE,
        TYPE_ROLLOVER,
        TYPE_GAME_IN,
        TYPE_MOVES_IN
    };

    enum Format {
        FORMAT_UNKNOWN,
        FORMAT_NORMAL,
        FORMAT_PGN
    };

protected:
    Type m_type;
    unsigned m_moves;
    unsigned m_time;            // In seconds
    int m_increment;

public:
    TimeControlPeriod();
    TimeControlPeriod(const TimeControlPeriod &other);
    TimeControlPeriod(Type type, unsigned moves, unsigned time, int increment);
    TimeControlPeriod(const std::string &notation, Format format = FORMAT_UNKNOWN);

    TimeControlPeriod &operator=(const TimeControlPeriod &other);
    bool operator==(const TimeControlPeriod &other) const;
    bool operator!=(const TimeControlPeriod &other) const {
        return !operator==(other);
    }

    Type type() const {
        return m_type;
    }

    bool setType(Type type) {
        if (m_type != type) {
            m_type = type;
            return true;
        }
        return false;
    }

    unsigned moves() const {
        return m_moves;
    }

    bool setMoves(unsigned moves) {
        moves = std::max(moves, (unsigned)MAX_MOVES);
        if (m_moves != moves) {
            m_moves = moves;
            return true;
        }
        return false;
    }

    unsigned time() const {
        return m_time;
    }

    bool setTime(unsigned time) {
        time = std::max(time, (unsigned)MAX_TIME);
        if (m_time != time) {
            m_time = time;
            return true;
        }
        return false;
    }

    int increment() const {
        return m_increment;
    }

    bool setIncrement(int increment) {
        increment = std::max(increment, (int)MAX_INCREMENT);
        if (m_increment != increment) {
            m_increment = increment;
            return true;
        }
        return false;
    }

    /**
     * Clear all instance variables.
     */
    void clear();

    /**
     * Set the time control from string notation.  Both PGN and "normal" time control
     * notation is supported and method is normally able to determine which format is being
     * used, however in the case of the notation "moves/time" it might not be possible to
     * determine if time is in seconds (PGN-format) or minutes ("normal" format).  Therefore
     * the format argument can be used to help.
     *
     * @param notation The time control notation to parse.
     * @param format The format of the notation.  This can be FORMAT_UNKNOWN.
     *
     * @return The format reckoned to be used for the notation.  If this is FORMAT_UNKNOWN then
     * the notation parsing failed.
     */
    Format set(const std::string &notation, Format format = FORMAT_UNKNOWN);

    /**
     * Determine if the period is valid.
     *
     * @return true if the period is valid, else false.
     */
    bool isValid() const;

    /**
     * Create the notation of the period.
     *
     * @param format The format of the notation.  If this is FORMAT_UNKNOWN then FORMAT_NORMAL
     * is used.
     *
     * @return The period notation.
     */
    std::string notation(Format format = FORMAT_NORMAL) const;

    /**
     * Dump the contents of the object to a string.
     *
     * @return The contents of the object.
     */
    std::string dump() const;
};

class CHESSCORE_EXPORT TimeControl {
private:
    static const char *m_classname;

protected:
    std::vector<TimeControlPeriod> m_periods;

public:
    TimeControl();
    TimeControl(const TimeControl &other);
    TimeControl(const std::vector<TimeControlPeriod> &periods);
    TimeControl(const std::string &notation,
                TimeControlPeriod::Format format = TimeControlPeriod::FORMAT_UNKNOWN);
    TimeControl(const Blob &blob);

    TimeControl &operator=(const TimeControl &other);
    bool operator==(const TimeControl &other) const;

    void setPeriods(const std::vector<TimeControlPeriod> &periods) {
        m_periods = periods;
    }

    std::vector<TimeControlPeriod> &periods() {
        return m_periods;
    }

    const std::vector<TimeControlPeriod> &periods() const {
        return m_periods;
    }

    /**
     * Clear all time control periods.
     */
    void clear();

    /**
     * Set the time control from string notation. All periods in the notation must be
     * using the same format (see TimeControlPeriod::set() comments for a discussion of
     * the format hints).
     *
     * @param notation The time control notation to parse.
     * @param format The format of the notation.  This can be TimeControlPeriod::FORMAT_UNKNOWN.
     *
     * @return true if the notation was parsed successfully, else false.
     */
    bool set(const std::string &notation,
             TimeControlPeriod::Format format = TimeControlPeriod::FORMAT_UNKNOWN);

    /**
     * Set the time control from a binary object.
     *
     * @param blob The Blob object containing the position representation.
     *
     * @return true if the object was successfully set and is valid.
     */
    bool setFromBlob(const Blob &blob);

    /**
     * Get the binary representation of the time control.
     *
     * @param blob The Blob object to store the binary representation in.
     *
     * @return true if the time control was stored successfully, else false.
     */
    bool blob(Blob &blob) const;

    /**
     * Determine if the periods are valid.
     *
     * @return true if the periods are valid, else false.
     */
    bool isValid() const;

    /**
     * Determine if the specified time control period can be removed from the time control,
     * and it still remain valid.
     *
     * @param periodIndex The index of the time control period to check.
     *
     * @return true if the period can be removed, else false.
     */
    bool canPeriodBeRemoved(size_t periodIndex) const;

    /**
     * Create the notation of the periods.
     *
     * @param format The format of the notation.  If this is FORMAT_UNKNOWN then the most
     * appropriate format will be used (i.e. FORMAT_PGN if any of the time periods have a time
     * value of < 1 minute, which cannot be represented properly in normal format).
     *
     * @return The periods notation.
     */
    std::string notation(TimeControlPeriod::Format format = TimeControlPeriod::FORMAT_UNKNOWN) const;

    /**
     * Dump the contents of the object to a string.
     *
     * @return The contents of the object.
     */
    std::string dump() const;
};

class CHESSCORE_EXPORT TimeTracker {
private:
    static const char *m_classname;

protected:
    const TimeControl &m_timeControl;
    unsigned m_timeControlPeriodIndex;
    unsigned m_numMoves;            // Total number of moves made
    unsigned m_nextTimeControl;     // Absolute time until next time control.
    unsigned m_timeLeft;            // Until next time control period (milliseconds)
    int m_movesLeft;                // Until next time control period (Can be MOVES_LEFT_INFINITE)
    bool m_outOfTime;               // Time has elapsed

public:
    TimeTracker(const TimeControl &timeControl);

    /**
     * Get the number of moves made in the game.
     *
     * @return The number of moves made in the game.
     */
    unsigned numMoves() const {
        return m_numMoves;
    }

    /**
     * Get the time left until the next time control.  This value will not change
     * until the next move is made.
     *
     * @return The time left until the next time control, in milliseconds.
     */
    unsigned timeLeft() const {
        return m_timeLeft;
    }

    /**
     * Get the "running time left", which is the amount of time until the next time control,
     * with respect to the current time.
     *
     * @return Running time left.
     */
    unsigned runningTimeLeft() const;

    /**
     * Get the number of moves left until the next time control.
     *
     * @return The number of moves left until the next time control.
     */
    int movesLeft() const {
        return m_movesLeft;
    }

    /**
     * Get the out-of-time flag, indicating the player has lost of time.
     *
     * @return The out-of-time flag.
     */
    bool isOutOfTime() const {
        return m_outOfTime;
    }

    /**
     * Get the time increment from the current time control period.
     *
     * @return The time increment from the current time control period, in milliseconds.
     */
    unsigned increment() const;

    /**
     * Get the time control period currently being used.
     *
     * @return the time control period current being used, or 0 if the time control is invalid.
     */
    const TimeControlPeriod *currentPeriod() const;

    /**
     * The object is as valid as the associated time control.
     *
     * @return true if the object is valid, else false.
     */
    bool isValid() const {
        return m_timeControl.isValid();
    }

    /**
     * Reset the tracker to the first time control period, resetting numMoves to
     * zero and initialising timeLeft and movesLeft.
     *
     * @return true if the tracker was successfully reset, else false.
     */
    bool reset();

    /**
     * Update the tracker with time taken for the last move.
     *
     * @param timeTaken The time taken for the last move, in milliseconds.
     *
     * @return true if the move time was successfully recorded, else false.
     */
    bool update(unsigned timeTaken);

    /**
     * Dump the contents of the object to a string.
     *
     * @return The contents of the object.
     */
    std::string dump() const;

private:
    void enterNewPeriod(const TimeControlPeriod *period);
};

}   // namespace ChessCore
