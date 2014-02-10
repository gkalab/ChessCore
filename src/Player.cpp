//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Player.cpp: Player class implementation.
//

#include <ChessCore/Player.h>
#include <ChessCore/Util.h>
#include <sstream>

using namespace std;

namespace ChessCore {
const char *Player::m_classname = "Player";

Player::Player() :
    m_lastName(),
    m_firstNames(),
    m_countryCode(),
    m_elo(0)
{
}

Player::Player(const std::string &formattedName) :
    m_lastName(),
    m_firstNames(),
    m_countryCode(),
    m_elo(0)
{
    setFormattedName(formattedName);
}

Player::Player(const Player &other) :
    m_lastName(other.m_lastName),
    m_firstNames(other.m_firstNames),
    m_countryCode(other.m_countryCode),
    m_elo(other.m_elo)
{
}

Player::~Player() {
}

void Player::set(const Player &other) {
    m_lastName = other.m_lastName;
    m_firstNames = other.m_firstNames;
    m_countryCode = other.m_countryCode;
    m_elo = other.m_elo;
}

void Player::set(const Player *other) {
    if (other == 0)
        return;

    m_lastName = other->m_lastName;
    m_firstNames = other->m_firstNames;
    m_countryCode = other->m_countryCode;
    m_elo = other->m_elo;
}

void Player::clear() {
    m_lastName.clear();
    m_firstNames.clear();
    m_countryCode.clear();
    m_elo = 0;
}

string Player::formattedName(bool noSpaces /*=false*/) const {
    ostringstream oss;

    if (!m_lastName.empty()) {
        oss << m_lastName;

        if (!m_firstNames.empty()) {
            if (noSpaces)
                oss << ",";
            else
                oss << ", ";

            oss << m_firstNames;
        }
    } else if (!m_firstNames.empty())
        oss << m_firstNames;

    return oss.str();
}
    
void Player::setFormattedName(const std::string &formattedName) {
    size_t comma = formattedName.find_first_of(',');
    if (comma != formattedName.npos) {
        // "LastName, First Names"
        setLastName(Util::trim(formattedName.substr(0, comma)));
        setFirstNames(Util::trim(formattedName.substr(comma + 1)));
    } else {
        // LastName
        setLastName(Util::trim(formattedName));
        clearFirstNames();
    }
}

ostream &operator << (ostream &os, const Player &player) {
    os << player.formattedName();
    return os;
}

}   // namespace ChessCore
