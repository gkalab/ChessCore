//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ProgOption.cpp: Program command line option parsing class implementation.
//

#define VERBOSE_LOGGING 0

#include <ChessCore/ProgOption.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>
#include <string.h>
#include <algorithm>
#include <sstream>

using namespace std;
using namespace ChessCore;

const char *ProgOption::m_classname = "ProgOption";

ProgOption::ProgOption() :
    m_shortOption(0),
    m_longOption(),
    m_mandatory(false),
    m_type(POTYPE_NONE),
    m_pindicator(0)
{
}

ProgOption::ProgOption(char shortOption, const std::string &longOption, bool mandatory, std::string *pstring,
                       bool *pindicator /*=0*/) :
    m_shortOption(shortOption),
    m_longOption(longOption),
    m_mandatory(mandatory),
    m_type(POTYPE_STRING),
    m_pindicator(pindicator)
{
    m_value.pstring = pstring;
}

ProgOption::ProgOption(char shortOption, const std::string &longOption, bool mandatory, int *pint,
                       bool *pindicator /*=0*/) :
    m_shortOption(shortOption),
    m_longOption(longOption),
    m_mandatory(mandatory),
    m_type(POTYPE_INT),
    m_pindicator(pindicator)
{
    m_value.pint = pint;
}

ProgOption::ProgOption(char shortOption, const std::string &longOption, bool mandatory, uint64_t *puint64,
                       bool *pindicator /*=0*/) :
    m_shortOption(shortOption),
    m_longOption(longOption),
    m_mandatory(mandatory),
    m_type(POTYPE_UINT64),
    m_pindicator(pindicator)
{
    m_value.puint64 = puint64;
}

ProgOption::ProgOption(char shortOption, const std::string &longOption, bool mandatory, bool *pbool,
                       bool *pindicator /*=0*/) :
    m_shortOption(shortOption),
    m_longOption(longOption),
    m_mandatory(mandatory),
    m_type(POTYPE_BOOL),
    m_pindicator(pindicator)
{
    m_value.pbool = pbool;
}

bool ProgOption::parse(const ProgOption options[], int argc, const char **argv, string &progName,
                       vector<string> &trailingArgs, string &errorMsg, bool allowInvalid /*=false*/) {
    int i, j, intvalue;
    bool boolvalue;
    uint64_t uint64value;
    string option, name, value;
    const char *start, *p;
    const ProgOption *progOption;

    vector<const ProgOption *> parsed;

    errorMsg.clear();

#ifdef _DEBUG
    for (i = 0; i < argc; i++)
        LOGVERBOSE << "argv[" << i << "] = '" << argv[i] << "'";
#endif // _DEBUG

    // Store the program name
    p = strrchr(argv[0], PATHSEP);

    if (p)
        progName.assign(p + 1);
    else
        progName.assign(argv[0]);

    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            // Not an option
            break;
        } else if (argv[i][0] == '-' &&
                 argv[i][1] == '-' &&
                 argv[i][2] == '\0') {
            // -- means 'no more options'
            i++;
            break;
        } else {
            value.clear();
            option = argv[i];
            start = argv[i] + 1;

            if (*start == '-')
                start++; // Allow --name as well as -name

            for (p = start; *p != '\0' && *p != '='; p++)
                ;

            if (*p == '=') {
                name.assign(start, p - start);
                if (*(p + 1) != '\0')
                    value.assign(p + 1);
            } else {
                name.assign(start);
            }

            for (j = 0, progOption = 0; options[j].m_type != POTYPE_NONE && progOption == 0; j++) {
                if (name.length() == 1) {
                    if (options[j].m_shortOption != '\0' && options[j].m_shortOption == name[0])
                        progOption = &options[j];
                } else if (options[j].m_longOption == name) {
                    progOption = &options[j];
                }
            }

            if (progOption == 0) {
                if (allowInvalid)
                    break;

                errorMsg = "Unknown option '" + option + "' specified";
                return false;
            }

            // If the value wasn't specified in the form '--name=value' and the next command
            // line option doesn't start with '-', then use that for the value.
            if (value.empty() &&
                i < argc && argv[i + 1][0] != '-')
                value = argv[++i];

            // Only bool can have no value
            if (progOption->m_type != POTYPE_BOOL && value.empty()) {
                errorMsg = "No value specified for option '" + option + "'";
                return false;
            }

            LOGVERBOSE << "name='" << name << "', value='" << value << "'";

            switch (progOption->m_type) {
            case POTYPE_STRING:
                progOption->setValue(value);
                break;

            case POTYPE_INT:
                if (!Util::parse(value, intvalue)) {
                    errorMsg = "Invalid value specified for option '" + option + "'";
                    return false;
                }

                progOption->setValue(intvalue);
                break;

            case POTYPE_UINT64:
                if (!Util::parse(value, uint64value)) {
                    errorMsg = "Invalid value specified for option '" + option + "'";
                    return false;
                }

                progOption->setValue(uint64value);
                break;

            case POTYPE_BOOL:
                if (!value.empty()) {
                    if (!Util::parse(value, boolvalue)) {
                        errorMsg = "Invalid value specified for option '" + option + "'";
                        return false;
                    }

                    progOption->setValue(boolvalue);
                } else {
                    progOption->setValue(true);
                }

                break;

            default:
                ASSERT(false);
                break;
            }

            parsed.push_back(progOption);
        }
    }

    // Check that any mandatory options have been parsed
    stringstream ss;
    unsigned notfound = 0;

    for (j = 0, progOption = 0; options[j].m_type != POTYPE_NONE; j++) {
        progOption = &options[j];

        if (progOption->m_mandatory) {
            vector<const ProgOption *>::const_iterator it = find(parsed.begin(), parsed.end(), progOption);

            if (it == parsed.end()) {
                if (notfound > 0)
                    ss << endl;

                ss << "Mandatory option '";

                if (progOption->m_shortOption != '\0')
                    ss << "-" << progOption->m_shortOption << "/--" << progOption->m_longOption;
                else
                    ss << "--" << progOption->m_longOption;

                ss << "' was not specified.";
                notfound++;
            }
        }
    }

    if (notfound > 0) {
        errorMsg = ss.str();
        return false;
    }

    // Remove parsed options from argc
    for (j = 1; i < argc; i++, j++)
        trailingArgs.push_back(string(argv[i]));

    return true;
}

void ProgOption::setValue(const string &strvalue) const {
    *(m_value.pstring) = strvalue;

    if (m_pindicator)
        *m_pindicator = true;
}

void ProgOption::setValue(int intvalue) const {
    *(m_value.pint) = intvalue;

    if (m_pindicator)
        *m_pindicator = true;
}

void ProgOption::setValue(uint64_t uint64value) const {
    *(m_value.puint64) = uint64value;

    if (m_pindicator)
        *m_pindicator = true;
}

void ProgOption::setValue(bool boolvalue) const {
    *(m_value.pbool) = boolvalue;

    if (m_pindicator)
        *m_pindicator = true;
}

void ProgOption::toggleValue() const {
    *(m_value.pbool) = !*(m_value.pbool);

    if (m_pindicator)
        *m_pindicator = true;
}
