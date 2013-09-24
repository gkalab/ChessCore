//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// UCIEngineOption.h: UCI Engine Option class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Move.h>
#include <string>
#include <vector>

namespace ChessCore {
class CHESSCORE_EXPORT UCIEngineOption {
private:
    static const char *m_classname;

public:
    enum Type {
        TYPE_NONE,
        TYPE_CHECK,
        TYPE_SPIN,
        TYPE_COMBO,
        TYPE_BUTTON,
        TYPE_STRING,
        TYPE_FILENAME,
        NUM_TYPES
    };

protected:
    enum {
        KEYWORD_NAME,
        KEYWORD_TYPE,
        KEYWORD_DEFAULT,
        KEYWORD_MIN,
        KEYWORD_MAX,
        KEYWORD_VAR,
        NUM_KEYWORDS
    };

    static const char *m_keywords[NUM_KEYWORDS];
    static const char *m_typeNames[NUM_TYPES];

    std::string m_name;
    Type m_type;
    std::string m_defValue;
    int m_minValue;
    int m_maxValue;
    std::vector<std::string> m_values;

    static int findKeyword(const std::vector<std::string> &parts, int startIndex, int &keywordIndex);

public:
    UCIEngineOption();
    ~UCIEngineOption();

    void clear();

    // Set the option from a vector of strings
    bool set(const std::vector<std::string> &parts);

    const std::string &name() const {
        return m_name;
    }

    void setName(const std::string &name) {
        m_name = name;
    }

    Type type() const {
        return m_type;
    }

    void setType(Type type) {
        m_type = type;
    }

    const char *typeName() const;

    void setTypeName(const std::string &typeName);

    const std::string defValue() const {
        return m_defValue;
    }

    void setDefValue(const std::string &defValue) {
        m_defValue = defValue;
    }

    int minValue() const {
        return m_minValue;
    }

    void setMinValue(int minValue) {
        m_minValue = minValue;
    }

    int maxValue() const {
        return m_maxValue;
    }

    void setMaxValue(int maxValue) {
        m_maxValue = maxValue;
    }

    const std::vector<std::string> &values() const {
        return m_values;
    }

    void addValue(const std::string &value) {
        m_values.push_back(value);
    }

    bool isValid() const;

    bool isValidValue(const std::string &value) const;

    std::string dump() const;
};
}   // namespace ChessCore
