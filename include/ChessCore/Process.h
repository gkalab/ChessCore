//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Process.h: Process class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {

#ifndef WINDOWS
// Define PROCESS_USE_POSIX_SPAWN to use posix_spawn() rather than fork()/exec()
#define PROCESS_USE_POSIX_SPAWN 1
#endif // !WINDOWS

// Define PROCESS_WAIT_FOR_CHILD to use wait() to get process termination status
#define PROCESS_WAIT_FOR_CHILD  1

class CHESSCORE_EXPORT Process {
private:
    static const char *m_classname;

protected:
    std::string m_name; // Process nick name
    bool m_loaded;      // Process is loaded
    int m_procId;		// Process id
    int m_exitCode;

#ifdef WINDOWS

	HANDLE m_procHandle;	// Process handle
	HANDLE m_fromHandle;	// Pipe 'from' handle
	HANDLE m_toHandle;		// Pipe 'to' handle

#else // !WINDOWS

    int m_fromFD;       // Pipe 'from' file descriptor
    int m_toFD;         // Pipe 'to' file descriptor

#endif // WINDOWS

public:
    Process();
    Process(const Process &other);

    virtual ~Process();

    /**
     * Load the specified executable process.
     *
     * @param name the nick name of the process.
     * @param exeFile the path to the executable process,
     * @param workDir the path to the new process working directory.  If this is empty then
     * the working directory is not changed.
     *
     * @return true if the process started successfully, else false.
     */
    bool load(const std::string &name, const std::string &exeFile, const std::string &workDir);

    /**
     * Unload the process
     */
    bool unload();

protected:
    int getExitCode();

public:
    inline const std::string &name() const {
        return m_name;
    }

    inline bool isLoaded() const {
        return m_loaded;
    }

    /**
     * Set the process priority.
     *
     * @param background If true, the process is set to "background priority", else
     * if false, the process is set to "foreground priority".
     *
     * @return true if the process priority was successfully changed.
     */
    bool setBackgroundPriority(bool background);

#ifdef WINDOWS
    inline HANDLE procHandle() const {
        return m_procHandle;
    }

    inline HANDLE fromHandle() const {
        return m_fromHandle;
    }

    inline HANDLE toHandle() const {
        return m_toHandle;
    }
#else // !WINDOWS
    inline unsigned procId() const {
        return m_procId;
    }

    inline int fromFD() const {
        return m_fromFD;
    }

    inline int toFD() const {
        return m_toFD;
    }
#endif // WINDOWS
};
} // namespace ChessCore
