//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Player.h: Player class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {
class CHESSCORE_EXPORT Player {
private:
    static const char *m_classname;

protected:
    std::string m_lastName;
    std::string m_firstNames;
    std::string m_countryCode;
    unsigned m_elo;

public:
    Player();

    Player(const Player &other) {
        set(other);
    }

    virtual ~Player();

    void initPlayer();

    // Set the player from another instance
    void set(const Player &other);
    void set(const Player *other);

    inline const std::string &lastName() const {
        return m_lastName;
    }

    inline void setLastName(const std::string &lastName) {
        m_lastName = lastName;
    }

    inline void clearLastName() {
        m_lastName.clear();
    }

    inline const std::string &firstNames() const {
        return m_firstNames;
    }

    inline void setFirstNames(const std::string &firstNames) {
        m_firstNames = firstNames;
    }

    inline void clearFirstNames() {
        m_firstNames.clear();
    }

    inline bool hasName() const {
        return !m_lastName.empty() || !m_firstNames.empty();
    }

    inline const std::string &countryCode() const {
        return m_countryCode;
    }

    inline void setCountryCode(const std::string &countryCode) {
        m_countryCode = countryCode;
    }

    inline unsigned elo() const {
        return m_elo;
    }

    inline void setElo(unsigned elo) {
        m_elo = elo;
    }

    /**
     * @return The formatted name.
     */
    std::string formattedName(bool noSpaces = false) const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Player &player);
};

} // namespace ChessCore
