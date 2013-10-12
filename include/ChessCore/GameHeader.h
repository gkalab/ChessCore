//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// GameHeader.h: Game Header class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Player.h>
#include <ChessCore/TimeControl.h>

namespace ChessCore {
class CHESSCORE_EXPORT GameHeader {
private:
    static const char *m_classname;

public:
    enum Result {
        UNFINISHED, WHITE_WIN, BLACK_WIN, DRAW
    };

protected:
    Player m_white;
    Player m_black;
    std::string m_event;
    std::string m_site;
    std::string m_annotator;
    unsigned m_day, m_month, m_year;
    unsigned m_roundMajor, m_roundMinor;
    Result m_result;
    std::string m_eco;
    TimeControl m_timeControl;
    bool m_readFail;

public:
    GameHeader();

    GameHeader(const GameHeader &other) {
        setHeader(other);
    }

    virtual ~GameHeader();

    void initHeader();

    /**
     * Set the game header from another instance.
     */
    void setHeader(const GameHeader &other);
    void setHeader(const GameHeader *other);

    /**
     * Set a description of the current computer as the site tag.
     */
    bool setSiteComputer();

    /*
     * Set the date to today's date.
     */
    bool setDateNow();

    inline Player &white() {
        return m_white;
    }

    inline const Player &white() const {
        return m_white;
    }

    inline void setWhite(const Player &white) {
        m_white = white;
    }

    inline Player &black() {
        return m_black;
    }

    inline const Player &black() const {
        return m_black;
    }

    inline void setBlack(const Player &black) {
        m_black = black;
    }

    inline const std::string &event() const {
        return m_event;
    }

    inline bool hasEvent() const {
        return !m_event.empty();
    }

    inline void setEvent(const std::string &event) {
        m_event = event;
    }

    inline const std::string &site() const {
        return m_site;
    }

    inline bool hasSite() const {
        return !m_site.empty();
    }

    inline void setSite(const std::string &site) {
        m_site = site;
    }

    inline const std::string &annotator() const {
        return m_annotator;
    }

    inline bool hasAnnotator() const {
        return !m_annotator.empty();
    }

    inline void setAnnotator(const std::string &annotator) {
        m_annotator = annotator;
    }

    inline int day() const {
        return m_day;
    }

    inline void setDay(int day) {
        m_day = day;
    }

    inline int month() const {
        return m_month;
    }

    inline void setMonth(int month) {
        m_month = month;
    }

    inline int year() const {
        return m_year;
    }

    inline void setYear(int year) {
        m_year = year;
    }

    inline bool hasDate() const {
        return m_day != 0 || m_month != 0 || m_year != 0;
    }

    inline unsigned roundMajor() const {
        return m_roundMajor;
    }

    inline void setRoundMajor(unsigned round) {
        m_roundMajor = round;
    }

    inline unsigned roundMinor() const {
        return m_roundMinor;
    }

    inline void setRoundMinor(unsigned round) {
        m_roundMinor = round;
    }

    inline bool hasRound() const {
        return m_roundMajor != 0 || m_roundMinor != 0;
    }

    inline Result result() const {
        return m_result;
    }

    inline void setResult(Result result) {
        m_result = result;
    }

    inline const std::string &eco() const {
        return m_eco;
    }

    inline void setEco(const std::string &eco) {
        m_eco = eco;
    }

    TimeControl &timeControl() {
        return m_timeControl;
    }

    const TimeControl &timeControl() const {
        return m_timeControl;
    }

    void setTimeControl(const TimeControl &timeControl) {
        m_timeControl = timeControl;
    }

    inline bool readFail() const {
        return m_readFail;
    }

    inline void setReadFail(bool readFail) {
        m_readFail = readFail;
    }

    /**
     * Format the game details to the specified string object.
     *
     * @param str The string in which to format the game details.
     * @param unknown The string to use in place of an empty white or black
     * player name.
     * @param forFilename If true then format for use as a filename, else format
     * in human-readable form.
     */
    void format(std::string &str, const std::string &unknown, bool forFilename) const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const GameHeader &gameHeader);
};

} // namespace ChessCore
