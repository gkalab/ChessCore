//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// TimeControl.cpp: TimeControl class implementation.
//

#include <ChessCore/TimeControl.h>
#include <ChessCore/Bitstream.h>
#include <ChessCore/Log.h>
#include <ChessCore/Util.h>
#include <sstream>
#include <algorithm>

using namespace std;

namespace ChessCore {

//
// TimeControlPeriod
//

const char *TimeControlPeriod::m_classname = "TimeControlPeriod";

TimeControlPeriod::TimeControlPeriod() :
    m_type(TYPE_NONE),
    m_moves(0),
    m_time(0),
    m_increment(0)
{
}

TimeControlPeriod::TimeControlPeriod(const TimeControlPeriod &other) :
    m_type(other.m_type),
    m_moves(other.m_moves),
    m_time(other.m_time),
    m_increment(other.m_increment)
{
}

TimeControlPeriod::TimeControlPeriod(Type type, unsigned moves, unsigned time, int increment) :
    m_type(type),
    m_moves(moves),
    m_time(time),
    m_increment(increment)
{
}

TimeControlPeriod::TimeControlPeriod(const string &notation, Format format /*=FORMAT_UNKNOWN*/) :
    m_type(TYPE_NONE),
    m_moves(0),
    m_time(0),
    m_increment(0)
{
    set(notation, format);
}

TimeControlPeriod &TimeControlPeriod::operator=(const TimeControlPeriod &other) {
    m_type = other.m_type;
    m_moves = other.m_moves;
    m_time = other.m_time;
    m_increment = other.m_increment;
    return *this;
}

bool TimeControlPeriod::operator==(const TimeControlPeriod &other) const {
    return
        m_type == other.m_type &&
        m_moves == other.m_moves &&
        m_time == other.m_time &&
        m_increment == other.m_increment;
}

void TimeControlPeriod::clear() {
    m_type = TYPE_NONE;
    m_moves = 0;
    m_time = 0;
    m_increment = 0;
}

TimeControlPeriod::Format TimeControlPeriod::set(const string &notation, Format format /*=FORMAT_UNKNOWN*/) {

    clear();

    if (notation.size() == 0)
        return FORMAT_UNKNOWN;

    vector<string> parts;
    unsigned numParts = Util::splitLine(notation, parts, '/');
    if (numParts > 0) {
        // Contains '/' separator
        if (numParts == 3) {
            if (parts[0] == "G" && Util::parse(parts[1], m_time) && Util::parse(parts[2], m_increment)) {
                // Normal format G/minutes/increment
                m_type = TYPE_GAME_IN;
                m_time *= 60;
                return FORMAT_NORMAL;
            } else if (Util::parse(parts[0], m_moves) && Util::parse(parts[1], m_time) && Util::parse(parts[2], m_increment)) {
                // Normal format moves/minutes/increment
                m_type = TYPE_ROLLOVER;
                m_time *= 60;
                return FORMAT_NORMAL;
            }
        } else if (numParts == 2) {
            if ((parts[0] == "G" || parts[0] == "g") && Util::parse(parts[1], m_time)) {
                // Normal format G/minutes
                m_type = TYPE_GAME_IN;
                m_time *= 60;
                return FORMAT_NORMAL;
            } else if ((parts[0] == "M" || parts[0] == "m") && Util::parse(parts[1], m_time)) {
                // Normal format M/seconds
                m_type = TYPE_MOVES_IN;
                m_moves = 1;
                return FORMAT_NORMAL;
            } else if (Util::parse(parts[0], m_moves) && Util::parse(parts[1], m_time)) {
                // Either format "moves/time"
                m_type = TYPE_ROLLOVER;
                if (format == FORMAT_PGN || (format == FORMAT_UNKNOWN && m_time >= 300)) {
                    return FORMAT_PGN;
                } else if (format == FORMAT_NORMAL) {
                    m_time *= 60;
                    return FORMAT_NORMAL;
                } else {
                    // Ambiguous
                    return FORMAT_UNKNOWN;
                }
            } else {
                // Is it PGN "moves/seconds+increment"?
                if (parts[1].find('+') != string::npos) {
                    vector<string> subparts;
                    unsigned numSubparts = Util::splitLine(parts[1], subparts, '+');
                    if (numSubparts == 2) {
                        if (Util::parse(parts[0], m_moves) && Util::parse(subparts[0], m_time) && Util::parse(subparts[1], m_increment)) {
                            m_type = TYPE_ROLLOVER;
                            return FORMAT_PGN;
                        }
                    }
                }
            }
        }
    }

    // Does not contain '/' character
    clear();

    // Is it PGN game in "seconds+increment"?
    if (notation.find('+') != string::npos) {
        unsigned numParts = Util::splitLine(notation, parts, '+');
        if (numParts == 2) {
            if (Util::parse(parts[0], m_time) && Util::parse(parts[1], m_increment)) {
                m_type = TYPE_GAME_IN;
                return FORMAT_PGN;
            }
        }
    }

    if (Util::parse(notation, m_time)) {
        // PGN game in "seconds"
        m_type = TYPE_GAME_IN;
        return FORMAT_PGN;
    } else if (notation[0] == '*') {
        // PGN moves in
        if (Util::parse(notation.substr(1), m_time)) {
            m_type = TYPE_MOVES_IN;
            m_moves = 1;
            return FORMAT_PGN;
        }
    }

    return FORMAT_UNKNOWN;
}

bool TimeControlPeriod::isValid() const {
    bool valid = false;
    switch (m_type) {
        case TYPE_NONE:
        default:
            valid = false;
        case TYPE_ROLLOVER:
            valid = m_moves > 0 && m_time > 0;
            break;
        case TYPE_GAME_IN:
            valid = m_moves == 0 && m_time > 0;
            break;
        case TYPE_MOVES_IN:
            valid = m_moves == 1 && m_time > 0 && m_increment == 0;
            break;
    }
    return valid;
}

string TimeControlPeriod::notation(Format format /*=FORMAT_NORMAL*/) const {
    if (format == FORMAT_UNKNOWN)
        format = FORMAT_NORMAL;
    switch (m_type) {
        case TYPE_ROLLOVER:
            if (m_increment) {
                if (format == FORMAT_PGN)
                    return Util::format("%u/%u%c%d", m_moves, m_time, (m_increment > 0 ? '+' : '-'), abs(m_increment));
                else
                    return Util::format("%u/%u/%d", m_moves, max(m_time / 60, 1u), m_increment);
            } else {
                if (format == FORMAT_PGN)
                    return Util::format("%u/%u", m_moves, m_time);
                else
                    return Util::format("%u/%u", m_moves, max(m_time / 60, 1u));
            }
            break;
        case TYPE_GAME_IN:
            if (m_increment) {
                if (format == FORMAT_PGN)
                    return Util::format("%u%c%d", m_time, (m_increment > 0 ? '+' : '-'), abs(m_increment));
                else
                    return Util::format("G/%u/%d", max(m_time / 60, 1u), m_increment);
            } else {
                if (format == FORMAT_PGN)
                    return Util::format("%u", m_time);
                else
                    return Util::format("G/%u", max(m_time / 60, 1u));
            }
            break;
        case TYPE_MOVES_IN:
            if (format == FORMAT_PGN)
                return Util::format("*%u", m_time);
            else
                return Util::format("M/%u", m_time);
            break;
        default:
            ASSERT(false);
            break;
    }
    return "";
}

string TimeControlPeriod::dump() const {
    ostringstream oss;
    oss << "m_type=";
    switch (m_type) {
        case TYPE_NONE:
        default:            oss << "invalid"; break;
        case TYPE_ROLLOVER: oss << "rollover"; break;
        case TYPE_GAME_IN:  oss << "game_in"; break;
        case TYPE_MOVES_IN: oss << "moves_in"; break;
    }
    oss << ", ";
    oss << "m_moves=" << m_moves << ", ";
    oss << "m_time=" << m_time << ", ";
    oss << "m_increment=" << m_increment;
    return oss.str();
}

//
// TimeControl
//

const char *TimeControl::m_classname = "TimeControl";

TimeControl::TimeControl() :
    m_periods()
{
}

TimeControl::TimeControl(const TimeControl &other) :
    m_periods(other.m_periods)
{

}

TimeControl::TimeControl(const vector<TimeControlPeriod> &periods) :
    m_periods(periods)
{
}

TimeControl::TimeControl(const string &notation,
                         TimeControlPeriod::Format format /*=TimeControlPeriod::FORMAT_UNKNOWN*/) :
    m_periods()
{
    set(notation, format);
}

TimeControl &TimeControl::operator=(const TimeControl &other) {
    m_periods = other.m_periods;
    return *this;
}

bool TimeControl::operator==(const TimeControl &other) const {
    if (m_periods.size() != other.m_periods.size())
        return false;
    for (size_t i = 0; i < m_periods.size(); i++)
        if (m_periods[i] != other.m_periods[i])
            return false;
    return true;
}

void TimeControl::clear() {
    m_periods.clear();
}

bool TimeControl::set(const string &notation, TimeControlPeriod::Format format /*=TimeControlPeriod::FORMAT_UNKNOWN*/) {

    m_periods.clear();

    if (format == TimeControlPeriod::FORMAT_UNKNOWN) {
        if (notation.find(',') != string::npos) {
            format = TimeControlPeriod::FORMAT_NORMAL;
        } else if (notation.find(':') != string::npos) {
            format = TimeControlPeriod::FORMAT_PGN;
        }
    }

    vector<string> parts;
    unsigned numParts = Util::splitLine(notation, parts, format == TimeControlPeriod::FORMAT_NORMAL ? ',' : ':');
    for (unsigned i = 0; i < numParts; i++) {
        string periodNotation = parts[i];
        TimeControlPeriod period;
        Util::trim(periodNotation);
        TimeControlPeriod::Format periodFormat = period.set(periodNotation, format);
        if (periodFormat == TimeControlPeriod::FORMAT_UNKNOWN) {
            LOGERR << "Failed to parse time control period '" << periodNotation << "'";
            return false;
        }

        if (format == TimeControlPeriod::FORMAT_UNKNOWN)
            format = periodFormat;

        m_periods.push_back(period);
    }

    return isValid();
}

bool TimeControl::setFromBlob(const Blob &blob) {
    //
    // Blob layout:
    //
    // num periods:                 4-bits.
    // period #1 type:              4-bits.
    // period #1 moves:             8-bits.
    // period #1 time (secs):       16-bits
    // period #1 increment (secs):  4-bits.
    // ...
    // period #n type:              4-bits.
    // period #n moves:             8-bits.
    // period #n time (secs):       16-bits
    // period #n increment (secs):  4-bits.
    //
    // Total size: (32-bits * num periods) + 4-bits.
    //

    clear();

    if (blob.length() < 5) {
        LOGERR << "Blob is too small (" << blob.length() << ") to contain time control";
        return false;
    }

    Bitstream stream(blob);
    uint32_t numPeriods = 0;
    if (!stream.read(numPeriods, 4)) {
        LOGERR << "Failed to read num-periods from bitstream";
        clear();
        return false;
    }

    for (uint32_t i = 0; i < numPeriods; i++) {
        uint32_t type, moves, time, increment;
        if (!stream.read(type, 4) ||
            !stream.read(moves, 8) ||
            !stream.read(time, 16) ||
            !stream.read(increment, 4)) {
            LOGERR << "Failed to read period #" << i << " from bitstream";
            clear();
            return false;
        }
        TimeControlPeriod period;
        period.setType((TimeControlPeriod::Type)type);
        period.setMoves(moves);
        period.setTime(time);
        period.setIncrement(increment);
        m_periods.push_back(period);
    }

    if (!isValid()) {
        LOGERR << "Time control is invalid";
        clear();
        return false;
    }

    return true;
}

bool TimeControl::blob(Blob &blob) const {
    //
    // See TimeControl::setFromBlob() for blob layout.
    //

    blob.free();

    if (!isValid())
        return true;            // Leave it empty

    if (!blob.reserve(1)) {
        LOGERR << "Failed to reserve space for position in blob";
        return false;
    }

    Bitstream stream(blob);

    uint32_t numPeriods = (uint32_t)m_periods.size();
    if (!stream.write(numPeriods, 4)) {
        LOGERR << "Failed to write num-periods to bitstream";
        return false;
    }

    for (uint32_t i = 0; i < numPeriods; i++) {
        uint32_t type = (uint32_t)m_periods[i].type();
        uint32_t moves = (uint32_t)m_periods[i].moves();
        uint32_t time = (uint32_t)m_periods[i].time();
        uint32_t increment = (uint32_t)m_periods[i].increment();
        if (!stream.write(type, 4) ||
            !stream.write(moves, 8) ||
            !stream.write(time, 16) ||
            !stream.write(increment, 4)) {
            LOGERR << "Failed to write period #" << i << " to bitstream";
            return false;
        }
    }

    return true;
}


bool TimeControl::isValid() const {
    size_t size = m_periods.size();

    if (size == 0) {
        return false;
    }

    for (size_t index = 0; index < size; index++) {
        const TimeControlPeriod &period = m_periods[index];

        // The period must be valid in itself
        if (!period.isValid())
            return false;

        // A 'Game in' or 'Moves in' period cannot be before another period
        if ((period.type() == TimeControlPeriod::TYPE_GAME_IN || period.type() == TimeControlPeriod::TYPE_MOVES_IN) &&
            index < size - 1) {
            return false;
        }
    }

    // The last period must be a 'Game in' or 'Moves in' period
    TimeControlPeriod::Type lastType = TimeControlPeriod::TYPE_NONE;
    if (size > 0)
        lastType = m_periods[size - 1].type();
    if (lastType == TimeControlPeriod::TYPE_GAME_IN ||
        lastType == TimeControlPeriod::TYPE_MOVES_IN) {
        return true;
    }

    return false;
}

bool TimeControl::canPeriodBeRemoved(size_t periodIndex) const {
    if (periodIndex > m_periods.size()) {
        LOGWRN << "Out-of-bounds periodIndex " << periodIndex << " (size=" << m_periods.size() << ")";
        return false;
    }
    if (!isValid()) {
        return false;
    }

    // If the time control is valid, then it's only possible to remove all but the last
    // time control period
    return periodIndex < m_periods.size() - 1;
}

string TimeControl::notation(TimeControlPeriod::Format format /*=TimeControlPeriod::FORMAT_NORMAL*/) const {

    if (format == TimeControlPeriod::FORMAT_UNKNOWN) {
        bool usePgn = false;
        for (auto it = m_periods.begin(); it != m_periods.end() && !usePgn; ++it) {
            usePgn = (it->time() < 60 && it->type() != TimeControlPeriod::TYPE_MOVES_IN);
        }
        format = usePgn ? TimeControlPeriod::FORMAT_PGN : TimeControlPeriod::FORMAT_NORMAL;
    }

    ostringstream oss;
    for (size_t index = 0; index < m_periods.size(); index++) {
        if (index > 0) {
            if (format == TimeControlPeriod::FORMAT_PGN)
                oss << ":";
            else
                oss << ", ";
        }
        oss << m_periods[index].notation(format);
    }

    return oss.str();
}

string TimeControl::dump() const {
    ostringstream oss;
    oss << m_periods.size() << " periods:\n";
    for (const TimeControlPeriod &period : m_periods)
        oss << period.dump() << "\n";
    return oss.str();
}

//
// TimeTracker
//

const char *TimeTracker::m_classname = "TimeTracker";

TimeTracker::TimeTracker(const TimeControl &timeControl) :
    m_timeControl(timeControl),
    m_timeControlPeriodIndex(0),
    m_numMoves(0),
    m_timeLeft(0),
    m_movesLeft(0),
    m_outOfTime(false)
{
    if (m_timeControl.isValid()) {
        reset();
    }
}

unsigned TimeTracker::increment() const {
    unsigned increment = 0;
    const TimeControlPeriod *period = currentPeriod();
    if (period)
        increment = period->increment() * 1000;
        return increment;
}

const TimeControlPeriod *TimeTracker::currentPeriod() const {
    const vector<TimeControlPeriod> &periods = m_timeControl.periods();
    if (m_timeControlPeriodIndex < periods.size())
        return &periods[m_timeControlPeriodIndex];
    return 0;
}

bool TimeTracker::reset() {
    if (!m_timeControl.isValid()) {
        LOGERR << "Time control is invalid";
        return false;
    }

    m_timeControlPeriodIndex = 0;
    m_outOfTime = false;
    m_timeLeft = 0;
    m_movesLeft = 0;

    const TimeControlPeriod *period = currentPeriod();
    if (period == 0) {
        LOGERR << "Failed to determine current time control period";
        return false;
    }

    enterNewPeriod(period);
    return true;
}

bool TimeTracker::update(unsigned timeTaken) {

    if (m_outOfTime) {
        LOGERR << "Already out-of-time";
        return false;
    }

    const TimeControlPeriod *period = currentPeriod();
    if (period == 0) {
        LOGERR << "Failed to determine current time control period";
        return false;
    }

    m_numMoves++;

    if (m_timeLeft < timeTaken) {
        m_timeLeft = 0;
        m_outOfTime = true;
        return true;                // Nothing left to do
    } else {
        m_timeLeft -= timeTaken;
    }

    if (period->increment()) {
        m_timeLeft += period->increment();
    }

    switch (period->type()) {
        case TimeControlPeriod::TYPE_ROLLOVER:
            ASSERT(m_movesLeft != 0);
            if (--m_movesLeft == 0) {
                // Time to move to the next time control period
                m_timeControlPeriodIndex++;

                const TimeControlPeriod *nextPeriod = currentPeriod();
                if (nextPeriod == 0) {
                    LOGERR << "Failed to determine next time control period";
                    return false;
                }

                enterNewPeriod(nextPeriod);
            }
            break;
        case TimeControlPeriod::TYPE_GAME_IN:
            break;
        case TimeControlPeriod::TYPE_MOVES_IN:
            ASSERT(m_movesLeft == 1);
            m_timeLeft = period->time() * 1000;
            break;
        default:
            ASSERT(false);
            break;
    }

    return true;
}

string TimeTracker::dump() const {
    ostringstream oss;
    oss << "m_timeControlPeriodIndex=" << m_timeControlPeriodIndex << ", ";
    oss << "m_numMoves=" << m_numMoves << ", ";
    oss << "m_timeLeft=" << m_timeLeft << ", ";
    oss << "m_movesLeft=" << m_movesLeft << ", ";
    oss << "m_outOfTime=" << boolalpha << m_outOfTime;
    return oss.str();
}

void TimeTracker::enterNewPeriod(const TimeControlPeriod *period) {
    m_timeLeft += period->time() * 1000;
    switch (period->type()) {
        case TimeControlPeriod::TYPE_ROLLOVER:
            m_movesLeft = period->moves();
            break;
        case TimeControlPeriod::TYPE_GAME_IN:
            m_movesLeft = 0;
            break;
        case TimeControlPeriod::TYPE_MOVES_IN:
            m_movesLeft = 1;
            break;
        default:
            ASSERT(false);
            break;
    }
}

}   // namespace ChessCore
