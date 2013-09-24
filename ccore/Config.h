//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Config.h: CCore-specific configuration class definition.
//

#pragma once

#include <ChessCore/Engine.h>
#include <memory>

class Config;

typedef std::unordered_map<std::string, std::shared_ptr<Config> > ConfigMap;

class Config {
private:
    static const char *m_classname;
protected:
    static ConfigMap g_configMap;           // All configurations
    static unsigned g_startupTimeout;
    static unsigned g_timeout;

    std::string m_cmdLine;
    std::string m_workDir;
    unsigned m_startupTimeout;
    unsigned m_timeout;
    ChessCore::StringStringMap m_options;

public:
    Config();
    ~Config();

    static void clear();

    //
    // Parse the options file and store the values it contains.
    //
    static bool read(const std::string &filename);

    const std::string &cmdLine() const {
        return m_cmdLine;
    }

    const std::string &workDir() const {
        return m_workDir;
    }

    unsigned startupTimeout() const {
        return m_startupTimeout;
    }

    unsigned timeout() const {
        return m_timeout;
    }

    //
    // Get the value of the specified option value.
    //
    const char *get(const std::string &key) const;
    bool get(const std::string &key, std::string &value) const;
    bool get(const std::string &key, int32_t &value) const;
    bool get(const std::string &key, int64_t &value) const;
    bool get(const std::string &key, uint64_t &value) const;
    bool get(const std::string &key, bool &value) const;

    const ChessCore::StringStringMap &options() const {
        return m_options;
    }

    //
    // Get a specific engine configuration
    //
    static const std::shared_ptr<Config> config(const std::string &name);
};
