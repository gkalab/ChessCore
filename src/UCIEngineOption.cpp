//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// UCIEngineOption.cpp: UCI Engine Option class implementation.
//

#include <ChessCore/UCIEngineOption.h>
#include <ChessCore/Log.h>
#include <ChessCore/Util.h>

using namespace std;

namespace ChessCore {
const char *UCIEngineOption::m_classname = "UCIEngineOption";
const char *UCIEngineOption::m_keywords[NUM_KEYWORDS] = {
    "name", "type", "default",  "min", "max", "var"
};
const char *UCIEngineOption::m_typeNames[NUM_TYPES] = {
    "none", "check", "spin", "combo", "button", "string", "filename"
};

UCIEngineOption::UCIEngineOption() :
    m_name(),
    m_type(TYPE_NONE),
    m_defValue(),
    m_minValue(0),
    m_maxValue(0),
    m_values()
{
}

UCIEngineOption::~UCIEngineOption() {
}

int UCIEngineOption::findKeyword(const vector<string> &parts, int startIndex, int &keywordIndex) {

    keywordIndex = -1;
    for (; startIndex < (int)parts.size(); startIndex++) {
        for (int i = 0; i < NUM_KEYWORDS; i++) {
            if (parts[startIndex] == m_keywords[i]) {
                keywordIndex = i;
                return startIndex;
            }
        }
    }

    return (int)parts.size();
}

void UCIEngineOption::clear() {
    m_name.clear();
    m_type = TYPE_NONE;
    m_defValue.clear();
    m_minValue = 0;
    m_maxValue = 0;
    m_values.clear();
}

bool UCIEngineOption::set(const vector<string> &parts) {
    bool retval = true;

    if (parts.size() == 0) {
        LOGERR << "UCI engine option string is empty";
        return false;
    } else if (parts[0] != "option") {
        LOGERR << "UCI engine option string does not begin with 'option'";
        return false;
    }

    bool error;
    string str;
    int index, nextIndex, keywordIndex, nextKeywordIndex;
    index = findKeyword(parts, 1, keywordIndex);

    do {
        error = false;
        nextIndex = findKeyword(parts, index + 1, nextKeywordIndex);

        if (index < (int)parts.size() && keywordIndex >= 0) {
            switch (keywordIndex) {
            case KEYWORD_NAME:
                if (index < nextIndex) {
                    m_name = Util::concat(parts, index + 1, nextIndex);
                }

                error = m_name.empty();
                break;

            case KEYWORD_TYPE:
                if (nextIndex == index + 2) {
                    for (int i = 0; i < NUM_TYPES; i++)
                        if (parts[index + 1] == m_typeNames[i]) {
                            m_type = (Type)i;
                            break;
                        }

                }

                error = (m_type == TYPE_NONE);
                break;

            case KEYWORD_DEFAULT:
                if (index < nextIndex) {
                    m_defValue = Util::concat(parts, index + 1, nextIndex);

                    if (m_defValue == "<empty>")
                        m_defValue.clear();
                }

                break;

            case KEYWORD_MIN:
                error = true;

                if (nextIndex == index + 2)
                    error = !Util::parse(parts[index + 1], m_minValue);

                break;

            case KEYWORD_MAX:
                error = true;

                if (nextIndex == index + 2)
                    error = !Util::parse(parts[index + 1], m_maxValue);

                break;

            case KEYWORD_VAR:
                if (index < nextIndex) {
                    str = Util::concat(parts, index + 1, nextIndex);
                    m_values.push_back(str);
                }

                break;

            default:
                ASSERT(false);
                break;
            }

            if (error) {
                LOGWRN << "UCI engine option has an invalid " << m_keywords[keywordIndex] <<
                    ": '" << Util::concat(parts, 0, (unsigned)parts.size()) << "'";
                retval = false;
            }
        }

        index = nextIndex;
        keywordIndex = nextKeywordIndex;
    } while (index < (int)parts.size() && retval);

    if (retval && m_type == TYPE_STRING)
        // If this is a string option but it looks like the option relates to a file
        // then change the type to TYPE_FILENAME.  Common filename options are:
        // "Book File", "GaviotaTbPath", "Session File", "Search Log Filename"
        // But the following are not:
        // "Use Session File"

        if ((Util::endsWith(m_name, "file", false) ||
             Util::endsWith(m_name, "filename", false) ||
             Util::endsWith(m_name, "path", false)) &&
            !Util::startsWith(m_name, "use", false)) {
            LOGDBG << "Decided that UCI engine option '" << m_name << "' is a file-related option rather than a plain string";
            m_type = TYPE_FILENAME;
        }

    return retval;
}

void UCIEngineOption::setTypeName(const std::string &typeName) {
    m_type = TYPE_NONE;

    for (unsigned i = TYPE_NONE; i < NUM_TYPES; i++) {
        if (typeName == m_typeNames[i]) {
            m_type = (Type)i;
            return;
        }
    }
}

const char *UCIEngineOption::typeName() const {
    if (m_type < NUM_TYPES)
        return m_typeNames[m_type];

    return "unknown";
}

bool UCIEngineOption::isValid() const {
    return
        m_name.length() > 0 &&
        m_type > TYPE_NONE && m_type < NUM_TYPES &&
        isValidValue(m_defValue);
}

bool UCIEngineOption::isValidValue(const std::string &value) const {
    int intValue;

    switch (m_type) {
    case TYPE_NONE:
        return false;

    case TYPE_CHECK:
        return value == "true" || value == "false";

    case TYPE_COMBO:

        for (auto it = m_values.begin(); it != m_values.end(); ++it)
            if (value == *it)
                return true;

        return false;

    case TYPE_SPIN:

        if (!Util::parse(value, intValue))
            return false;

        if (intValue < m_minValue || intValue > m_maxValue)
            return false;

        return true;

    default:
        return true;
    }
}

string UCIEngineOption::dump() const {
    stringstream ss;

    ss << "name='" << m_name << "', type=" << m_typeNames[m_type] <<
        ", defValue='" << m_defValue << "', min=" << m_minValue << ", max=" << m_maxValue <<
        ", values=(";

    bool first = true;

    for (auto it = m_values.begin(); it != m_values.end(); ++it) {
        if (!first)
            ss << ", ";
        else
            first = false;

        ss << "'" << *it << "'";
    }

    ss << ")";

    return ss.str();
}
}   // namespace ChessCore
