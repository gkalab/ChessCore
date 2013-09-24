//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ProgOption.h: Program command line option parsing class declaration.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <vector>

namespace ChessCore {
class CHESSCORE_EXPORT ProgOption {
private:
    static const char *m_classname;

protected:
    enum Type {
        POTYPE_NONE,
        POTYPE_STRING,
        POTYPE_INT,
        POTYPE_UINT64,
        POTYPE_BOOL
    };

    char m_shortOption;
    std::string m_longOption;
    bool m_mandatory;
    Type m_type;
    bool *m_pindicator;

    union {
        std::string *pstring;
        int *pint;
        uint64_t *puint64;
        bool *pbool;
    } m_value;

public:
    ProgOption();                   // End option
    ProgOption(char shortOption, const std::string &longOption, bool mandatory, std::string *pstring,
               bool *pindicator = 0);
    ProgOption(char shortOption, const std::string &longOption, bool mandatory, int *pint, bool *pindicator = 0);
    ProgOption(char shortOption, const std::string &longOption, bool mandatory, uint64_t *puint64, bool *pindicator =
                   0);
    ProgOption(char shortOption, const std::string &longOption, bool mandatory, bool *pbool, bool *pindicator = 0);

    /**
     * Parse the command line.
     *
     * @param options An array of ProgOption objects containing the command line
     * option definitions.  This array must be terminated with an 'End option'
     * (constructed with the zero-argument ProgOption constructor).
     * @param argc The number of arguments from the command line.
     * @param argv The arguments from the command line.
     * @param progName Where to store the program executable name, as derived from
     * argv[0].
     * @param trailingArgs Where to store command line found after options have been
     * parsed.
     * @param errorMsg Where to store any error message related to parsing.
     * @param allowInvalid If true then invalid options are ignored command line
     * parsing is stopped.
     *
     * @return true if the command line arguments was parsed successfully, else false.
     */
    static bool parse(const ProgOption options[], int argc, const char **argv, std::string &progName,
                      std::vector<std::string> &trailingArgs, std::string &errorMsg, bool allowInvalid = false);

protected:
    void setValue(const std::string &strvalue) const;
    void setValue(int intvalue) const;
    void setValue(uint64_t uint64value) const;
    void setValue(bool boolvalue) const;
    void toggleValue() const;
};
}   // namespace ChessCore
