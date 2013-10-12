//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// TimeControl.h: TimeControl class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Blob.h>
#include <vector>

namespace ChessCore {

class CHESSCORE_EXPORT TimeControlPeriod {
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

    Type type() const {
        return m_type;
    }

    void setType(Type type) {
        m_type = type;
    }

    unsigned moves() const {
        return m_moves;
    }

    void setMoves(unsigned moves) {
        m_moves = moves;
    }

    unsigned time() const {
        return m_time;
    }

    void setTime(unsigned time) {
        m_time = time;
    }

    int increment() const {
        return m_increment;
    }

    void setIncrement(int increment) {
        m_increment = increment;
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
     * Create the notation of the periods.
     *
     * @param format The format of the notation.  If this is FORMAT_UNKNOWN then FORMAT_NORMAL
     * is used.
     *
     * @return The periods notation.
     */
    std::string notation(TimeControlPeriod::Format format = TimeControlPeriod::FORMAT_NORMAL) const;

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
    unsigned m_timeLeft;            // Until next time control period (milliseconds)
    int m_movesLeft;                // Until next time control period (Can be MOVES_LEFT_INFINITE)
    bool m_outOfTime;               // Time has elapsed

public:
    TimeTracker(const TimeControl &timeControl);

    unsigned numMoves() const {
        return m_numMoves;
    }

    unsigned timeLeft() const {
        return m_timeLeft;
    }

    int movesLeft() const {
        return m_movesLeft;
    }

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