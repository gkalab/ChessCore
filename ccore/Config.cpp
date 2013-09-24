//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Config.cpp: CCore-specific configuration class implementation.
//

#include "ccore.h"
#include <ChessCore/Log.h>
#include <string.h>
#include <fstream>

using namespace std;
using namespace ChessCore;

const char *Config::m_classname = "Config";
ConfigMap Config::g_configMap;
unsigned Config::g_startupTimeout;
unsigned Config::g_timeout;

Config::Config() : 
    m_cmdLine(),
    m_workDir(),
    m_startupTimeout(0),
    m_timeout(0),
    m_options() {
}

Config::~Config() {
    m_options.clear();
}

void Config::clear() {
    g_configMap.clear();
}

//
// Parse the configuration file and store the values it contains.
//
bool Config::read(const string &filename) {
    ifstream file;

    file.open(filename.c_str(), ifstream::in);

    if (!file.is_open()) {
        logerr("Failed to read configuration file '%s'", filename.c_str());
        return false;
    }

    char buffer[1024];
    vector<string> parts;
    unsigned line = 0, numParts, uvalue;
    bool inEngineSection = false, retval = true;
    string engineName, str;
    shared_ptr<Config> config;

    while (!file.eof() && !file.fail() && retval) {
        file.getline(buffer, sizeof(buffer));
        line++;

        size_t len = strlen(buffer);

        while (len > 0 && isspace(buffer[len - 1]))
            buffer[--len] = '\0';

        if (len == 0)
            continue;

        numParts = Util::splitLine(buffer, parts);

        if (numParts == 0 || parts[0][0] == '#')
            continue;

        if (!inEngineSection) {
            if (parts[0] == "engine") {
                if (numParts != 2) {
                    logerr("%s:%u: Engine must be assigned a name",
                           filename.c_str(), line);
                    retval = false;
                    break;
                }

                if (g_configMap.find(parts[1]) != g_configMap.end()) {
                    logerr("%s:%u: Engine configuration %s is already defined",
                           filename.c_str(), line, parts[1].c_str());
                    retval = false;
                    break;
                }

                engineName = parts[1];
                inEngineSection = true;
                config.reset(new Config);
                continue;
            } else {
                // Normal option
                if (numParts != 2) {
                    logerr("%s:%u: Option must have name and value",
                           filename.c_str(), line);
                    retval = false;
                    break;
                } else if (parts[0] == "startup_timeout") {
                    if (!Util::parse(parts[1], uvalue)) {
                        logerr("%s:%u: Invalid startup_timeout value '%s'",
                               filename.c_str(), line, parts[1].c_str());
                        retval = false;
                        break;
                    }

                    g_startupTimeout = uvalue;
                } else if (parts[0] == "timeout") {
                    if (!Util::parse(parts[1], uvalue)) {
                        logerr("%s:%u: Invalid timeout value '%s'",
                               filename.c_str(), line, parts[1].c_str());
                        retval = false;
                        break;
                    }

                    g_timeout = uvalue;
                }
            }
        } else if (inEngineSection) {
            if (parts[0] == "end") {
                if (numParts > 1) {
                    logerr("%s:%u: Engine end statement cannot have a value",
                           filename.c_str(), line);
                    retval = false;
                    break;
                }

                if (config->cmdLine().empty()) {
                    logerr("%s:%u: Engine configuration does not contains 'cmdline' setting",
                           filename.c_str(), line);
                    retval = false;
                    break;
                }

                loginf("Read engine configuration %s from file %s",
                       engineName.c_str(), filename.c_str());

                g_configMap[engineName] = config;
                config.reset();
                inEngineSection = false;
                engineName.clear();
                continue;
            }

            // All other options have values
            if (numParts == 1) {
                logerr("%s:%u: No value defined", filename.c_str(), line);
                retval = false;
                break;
            }

            if (parts[0] == "cmdline") {
                config->m_cmdLine = parts[1];
                continue;
            } else if (parts[0] == "workdir") {
                config->m_workDir = parts[1];
                continue;
            } else if (parts[0] == "startup_timeout") {
                if (!Util::parse(parts[1], uvalue)) {
                    logerr("%s:%u: Invalid startup_timeout value '%s'",
                           filename.c_str(), line, parts[1].c_str());
                    retval = false;
                    break;
                }

                config->m_startupTimeout = uvalue;
            } else if (parts[0] == "timeout") {
                if (!Util::parse(parts[1], uvalue)) {
                    logerr("%s:%u: Invalid timeout value '%s'",
                           filename.c_str(), line, parts[1].c_str());
                    retval = false;
                    break;
                }

                config->m_timeout = uvalue;
            } else if (parts[0] == "option") {
                if (numParts == 2)
                    config->m_options[parts[1]] = "";
                else if (numParts == 3)
                    config->m_options[parts[1]] = parts[2];
                else {
                    logerr("%s:%u: Invalid engine option",
                           filename.c_str(), line);
                    retval = false;
                    break;
                }
            } else {
                logerr("%s:%u: Unknown engine configuration value '%s'",
                       filename.c_str(), line, parts[0].c_str());
                retval = false;
                break;
            }
        }
    }

    if (retval) {
        loginf("Read %u configuration settings from file '%s'",
               g_configMap.size(), filename.c_str());

        for (auto it = g_configMap.begin(); it != g_configMap.end(); ++it) {
            shared_ptr<Config> config = it->second;
            // Use the global timeout values for anyu engine configs that didn't specify them
            if (config->m_startupTimeout == 0)
                config->m_startupTimeout = g_startupTimeout;

            if (config->m_timeout == 0)
                config->m_timeout = g_timeout;
            }
    }

    return retval;
}

const char *Config::get(const string &key) const {
    StringStringMap::const_iterator it;
    it = m_options.find(key);

    if (it == m_options.end())
        return 0;

    return it->second.c_str();
}

bool Config::get(const string &key, string &value) const {
    StringStringMap::const_iterator it;
    it = m_options.find(key);

    if (it == m_options.end())
        return false;

    value = it->second;
    return true;
}

bool Config::get(const string &key, int32_t &value) const {
    // Fetch the string option value
    string str;

    if (!get(key, str))
        return false;

    return Util::parse(str, value);
}

bool Config::get(const string &key, int64_t &value) const {
    // Fetch the string option value
    string str;

    if (!get(key, str))
        return false;

    return Util::parse(str, value);
}

bool Config::get(const string &key, uint64_t &value) const {
    // Fetch the string option value
    string str;

    if (!get(key, str))
        return false;

    return Util::parse(str, value);
}

bool Config::get(const string &key, bool &value) const {
    // Fetch the string option value
    string str;

    if (!get(key, str))
        return false;

    return Util::parse(str, value);
}

const shared_ptr<Config> Config::config(const string &name) {
    auto it = g_configMap.find(name);
    if (it != g_configMap.end())
        return it->second;
    return 0;
}
