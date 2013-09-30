//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Process.cpp: Process class implementation.
//

#include <ChessCore/Process.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>

#ifdef WINDOWS

#include <process.h>

#else // !WINDOWS

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#if PROCESS_WAIT_FOR_CHILD
#include <sys/wait.h>
#endif // PROCESS_WAIT_FOR_CHILD

#if PROCESS_USE_POSIX_SPAWN
#include <spawn.h>
#endif // PROCESS_USE_POSIX_SPAWN

#endif // WINDOWS

#include <iostream>

using namespace std;

#ifdef WINDOWS
// Special CreatePipeEx function, which allows overlapped I/O on the handles
static BOOL APIENTRY MyCreatePipeEx(OUT LPHANDLE lpReadPipe, OUT LPHANDLE lpWritePipe,
                                    IN LPSECURITY_ATTRIBUTES lpPipeAttributes, IN DWORD nSize,
                                    DWORD dwReadMode, DWORD dwWriteMode);
#endif // WINDOWS

namespace ChessCore {

const char *Process::m_classname = "Process";

Process::Process() : 
	m_name(),
	m_loaded(false),
	m_procId(0),
	m_exitCode(0),
#ifdef WINDOWS
	m_procHandle(INVALID_HANDLE_VALUE),
	m_fromHandle(INVALID_HANDLE_VALUE),
	m_toHandle(INVALID_HANDLE_VALUE)
#else // !WINDOWS
	m_fromFD(-1),
	m_toFD(-1)
#endif // WINDOWS	
	{
}

Process::Process(const Process &other) : 
	m_name(other.m_name),
	m_loaded(other.m_loaded),
	m_procId(other.m_procId),
	m_exitCode(other.m_exitCode),
#ifdef WINDOWS
	m_procHandle(INVALID_HANDLE_VALUE),
	m_fromHandle(INVALID_HANDLE_VALUE),
	m_toHandle(INVALID_HANDLE_VALUE)
#else // !WINDOWS
	m_fromFD(-1),
	m_toFD(-1)
#endif // WINDOWS	
	{

#ifdef WINDOWS
	Util::win32DuplicateHandle(other.m_procHandle, m_procHandle);
	Util::win32DuplicateHandle(other.m_fromHandle, m_fromHandle);
	Util::win32DuplicateHandle(other.m_toHandle, m_toHandle);
#else // !WINDOWS
	if (other.m_fromFD >= 0)
		m_fromFD = dup(other.m_fromFD);
	if (other.m_toFD >= 0)
		m_toFD = dup(other.m_toFD);
#endif // WINDOWS
}

Process::~Process() {
    unload();
}

bool Process::load(const string &name, const string &exeFile, const string &workDir) {
    if (m_loaded) {
        LOGWRN << "Process " << name << " is already loaded!";
        return false;
    }

    LOGINF << "Starting process " << name << " from executable '" << exeFile <<
        "' with working directory '" << workDir << "'";

#ifdef WINDOWS

	// See "How to spawn console processes with redirected standard handles"
    // (http://support.microsoft.com/kb/190351)

    HANDLE childStdout, childStderr, childStdin;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;

    // We want handles to be inherited by children
    ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    ::SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = TRUE;

    // From process
    if (!::MyCreatePipeEx(&m_fromHandle, &childStdout, &sa, 0, FILE_FLAG_OVERLAPPED, 0))
    {
        LOGERR << "Failed to create process " << name << " write pipe: " <<
            Util::win32ErrorText(GetLastError());
        unload();
        return false;
    }

    // To process
    if (!::MyCreatePipeEx(&childStdin, &m_toHandle, &sa, 0, 0, 0))
    {
        LOGERR << "Failed to create process " << name << " read pipe: " <<
            Util::win32ErrorText(GetLastError());
        unload();
        return false;
    }

    // Duplicate engine's stdout for stderr to allow it to close it
	if (!Util::win32DuplicateHandle(childStdout, childStderr)) {
        LOGERR << "Failed to duplicate stdout handle to stderr for process " << name << ": " <<
            Util::win32ErrorText(GetLastError());
        unload();
        return false;
    }

    // Don't allow the child to inherit our side of the pipes
    ::SetHandleInformation(m_fromHandle, HANDLE_FLAG_INHERIT, 0);
    ::SetHandleInformation(m_toHandle, HANDLE_FLAG_INHERIT, 0);

    // Launch the child process
    ::memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = childStdout;
    si.hStdInput = childStdin;
    si.hStdError = childStderr;
    si.wShowWindow = SW_HIDE;

    if (!::CreateProcess(NULL, const_cast<char *>(exeFile.c_str()), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL,
                       workDir.length() > 0 ? workDir.c_str() : NULL, &si, &pi)) {
        LOGERR << "Failed to create process " << name << ": " << Util::win32ErrorText(GetLastError());
        unload();
        return false;
    }

    m_name = name;
    m_loaded = true;

    ::CloseHandle(childStderr);
    ::CloseHandle(childStdout);
    ::CloseHandle(childStdin);

    m_procId = pi.dwProcessId;
    m_procHandle = pi.hProcess;
    ::CloseHandle(pi.hThread);

    LOGINF << "Started process " << name;

    return true;

#else // !WINDOWS

    int childStdin[2], childStdout[2];

    if (::pipe(childStdin) < 0) {
        LOGERR << "Failed to create child stdin pipe for process " << name << ": " <<
            strerror(errno) << " (" << errno << ")";
        return false;
    }

    if (::pipe(childStdout) < 0) {
        LOGERR << "Failed to create child stdout pipe for process " << name << ": " <<
            strerror(errno) << " (" << errno << ")";
        ::close(childStdin[0]);
        ::close(childStdin[1]);
        return false;
    }

#if PROCESS_USE_POSIX_SPAWN
    // Temporarily change directory to the working directory
    char currDir[1024];
    ::getcwd(currDir, sizeof(currDir));

    if (!workDir.empty()) {
        if (::chdir(workDir.c_str())) {
            LOGERR << "Failed to change directory to '" << workDir << "': " <<
                strerror(errno) << " (" << errno << ")";
            ::close(childStdin[0]);
            ::close(childStdin[1]);
            ::close(childStdout[0]);
            ::close(childStdout[1]);
            return false;
        }
    }

    // Set-up file actions
    posix_spawn_file_actions_t fileActions;
    posix_spawn_file_actions_init(&fileActions);
    posix_spawn_file_actions_adddup2(&fileActions, childStdin[0], 0);  // stdin
    posix_spawn_file_actions_addclose(&fileActions, childStdin[1]);    // Write side of stdin
    posix_spawn_file_actions_adddup2(&fileActions, childStdout[1], 1); // stdout
    posix_spawn_file_actions_adddup2(&fileActions, childStdout[1], 2); // stderr
    posix_spawn_file_actions_addclose(&fileActions, childStdout[0]);   // Read side of stdout

    // Set-up attributes (note: we don't set the process group, which means the child process will
    // inherit our process group)
    posix_spawnattr_t attribs;
    posix_spawnattr_init(&attribs);

    // Make the arguments writable by copying them into a buffer
    shared_ptr<char> buffer(::strdup(exeFile.c_str()), ::free);
    char *arg[2] = { buffer.get(), NULL };

    // Spawn the process
    int spawnResult = ::posix_spawn(&m_procId, exeFile.c_str(), &fileActions, &attribs, arg, NULL);
    int error = errno;      // Save as we do lots of clean-up before reporting error

    // Clean-up allocated resources
    posix_spawnattr_destroy(&attribs);
    posix_spawn_file_actions_destroy(&fileActions);

    // Change back to our original directory
    if (!workDir.empty()) {
        ::chdir(currDir);
    }

    if (spawnResult) {
        LOGERR << "Failed to spawn process '" << exeFile <<
            "' from directory '" << workDir << "': " <<
            strerror(error) << " (" << error << ")";
        ::close(childStdin[0]);
        ::close(childStdin[1]);
        ::close(childStdout[0]);
        ::close(childStdout[1]);
        return false;
    }

    close(childStdin[0]);   // Read side of child stdin
    m_toFD = childStdin[1];
    close(childStdout[1]);  // Write side of child stdout
    m_fromFD = childStdout[0];

#else // !PROCESS_USE_POSIX_SPAWN

	m_procId = ::fork();

    if ((int)m_procId == -1) {
        LOGERR << "Failed to fork process " << name << ": " <<
            strerror(errno) << " (" << errno << ")";
        ::close(childStdin[0]);
        ::close(childStdin[1]);
        ::close(childStdout[0]);
        ::close(childStdout[1]);
        return false;
    } else if (m_procId == 0) {
        // Child process

        // Put the child process in its own process group so that signals sent to the parent
        // don't affect it
        if (::setpgrp() < 0) {
            cout << "XXfail Failed to set the process group for process '" << name <<
                "': " << strerror(errno) << " (" << errno << ")" << endl;
            _exit(10);
        }

        // Child process
        if (!workDir.empty()) {
            if (::chdir(workDir.c_str()) < 0) {
                cout << "XXfail Failed to change directory for process '" << name <<
                    "' to '" << workDir << "': " << strerror(errno) << " (" << errno << ")" << endl;
                _exit(11);
            }
		}

        close(childStdin[1]);   // Write side of stdin
        close(childStdout[0]);  // Read side of stdout
        dup2(childStdin[0], 0); // stdin
        dup2(childStdout[1], 1); // stdout
        dup2(childStdout[1], 2); // stderr

        // Close all remaining files
        int max_fd = getdtablesize();

        for (int i = 3; i < max_fd; i++)
            close(i);

        const unsigned MAX_ARGS = 64;
        char buffer[4096];
        char *arg[MAX_ARGS];
        strcpy(buffer, exeFile.c_str());
        Util::splitLine(buffer, arg, MAX_ARGS);

        // Exec the new process
        execv(arg[0], arg);

        // If we are here then the exec failed
        cout << "XXfail Failed to execute binary '" << exeFile << "': " <<
            strerror(errno) << " (" << errno << ")" << endl;
        _exit(12);
    } else {
        // Parent process
        close(childStdin[0]); // Read side of child stdin
        m_toFD = childStdin[1];
        close(childStdout[1]); // Write side of child stdout
        m_fromFD = childStdout[0];
    }
#endif // PROCESS_USE_POSIX_SPAWN

    m_name = name;
    m_loaded = true;

    LOGINF << "Started process " << name;

    return true;
#endif // WINDOWS
}

bool Process::unload() {

#ifdef WINDOWS
    if (m_loaded) {
        DWORD dwExitCode = 0;
        bool terminated = false;
        for (int retry = 0; retry < 6 && !terminated; retry++) {
            if (retry >= 3) {
                LOGDBG << "Terminating process " << name();
                TerminateProcess(m_procHandle, 1);
                Util::sleep(500);
            }

            LOGDBG << "Getting process exit code for process " << name();
            if (GetExitCodeProcess(m_procHandle, &dwExitCode)) {
                if (dwExitCode != STILL_ACTIVE) {
                    m_exitCode = (int)dwExitCode;
                    LOGINF << "Process " << name() << " terminated with exit code " << m_exitCode;
                    terminated = true;
                }
            } else {
                LOGERR << "Failed to get process " << name() << " exit status: " <<
                    Util::win32ErrorText(GetLastError());
            }

            if (!terminated)
                Util::sleep(500);
        }
        m_loaded = false;
    }

    if (m_fromHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_fromHandle);
        m_fromHandle = INVALID_HANDLE_VALUE;
    }

    if (m_toHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_toHandle);
        m_toHandle = INVALID_HANDLE_VALUE;
    }

#else // !WINDOWS

    if (m_loaded) {
#if PROCESS_WAIT_FOR_CHILD
        m_exitCode = 88;
        bool terminated = false;

        for (int retry = 0; retry < 6 && !terminated; retry++) {
            int status, signal;

            if (retry >= 3) {
                // Send a SIGINT on the 4th and 5th retry and a SIGKILL on the 6th retry
                signal = (retry == 5) ? SIGKILL : SIGINT;
                LOGINF << "Sending " << (signal == SIGKILL ? "SIGKILL" : "SIGINT") <<
                    " to process " << name();
                kill(m_procId, signal);
                Util::sleep(500);
            }

            LOGDBG << "Getting exit code of process " << name();

            if (waitpid(m_procId, &status, WNOHANG) < 0) {
                LOGERR << "Failed to wait for process " << name() << ": " <<
                    strerror(errno) << " (" << errno << ")";
                break;
            } else if (WIFEXITED(status)) {
                m_exitCode = WEXITSTATUS(status);
                terminated = true;
                LOGINF << "Process " << name() << " terminated with exit code " << m_exitCode;
            } else if (WIFSIGNALED(status)) {
                signal = WTERMSIG(status);
                if (signal < NSIG) {
                    terminated = true;
                    LOGINF << "Process " << name() << " terminated by signal " << signal;
                } else {
                    LOGWRN << "Process " << name() << " produced unusual signal " << signal
                        << "; assuming it's not terminated";
                }
            } else {
                LOGWRN << "Process " << name() << " changed state but did not terminate.  status=0x" <<
                    hex << status;
            }

            if (!terminated)
                Util::sleep(250);
        }

#else // !PROCESS_WAIT_FOR_CHILD
        m_exitCode = 0;
#endif // PROCESS_WAIT_FOR_CHILD

        m_loaded = false;
    }

    if (m_fromFD != -1) {
        close(m_fromFD);
        m_fromFD = -1;
    }

    if (m_toFD != -1) {
        close(m_toFD);
        m_toFD = -1;
    }

#endif // WINDOWS

    return true;
}

bool Process::setBackgroundPriority(bool background) {
#ifdef WINDOWS

    DWORD priority = background ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
    DWORD currentPriority = ::GetPriorityClass(m_procHandle);
    if (priority != currentPriority) {
        if (!::SetPriorityClass(m_procHandle, priority)) {
            LOGERR << "Process " << name() << ": Failed to set " <<
                (background ? "background" : "foreground") << " priority: " <<
                Util::win32ErrorText(GetLastError());
            return false;
        }
        LOGDBG << "Process " << name() << ": Successfully set " <<
            (background ? "background" : "foreground") << " priority";
    }
    return true;

#else // !WINDOWS

#ifdef MACOSX
#define BACKGROUND_PRIORITY PRIO_DARWIN_BG
#else
#define BACKGROUND_PRIORITY 19
#endif

    int priority = background ? BACKGROUND_PRIORITY : 0;
    int currentPriority = ::getpriority(PRIO_PROCESS, m_procId);
    if (priority != currentPriority) {
        if (::setpriority(PRIO_PROCESS, m_procId, priority) < 0) {
            LOGERR << "Process " << name() << ": Failed to set " <<
                (background ? "background" : "foreground") << " priority: " <<
                strerror(errno) << " (" << errno << ")";
            return false;
        }
        LOGDBG << "Process " << name() << ": Successfully set " <<
            (background ? "background" : "foreground") << " priority";
    }
    return true;

#endif // WINDOWS
}

}   // namespace ChessCore

#ifdef WINDOWS
static ULONG PipeSerialNumber = 1;
/*

 The CreatePipeEx API is used to create an anonymous pipe I/O device.
 Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
 both handles.
 Two handles to the device are created.  One handle is opened for
 reading and the other is opened for writing.  These handles may be
 used in subsequent calls to ReadFile and WriteFile to transmit data
 through the pipe.

 Arguments:

 lpReadPipe - Returns a handle to the read side of the pipe.  Data
 may be read from the pipe by specifying this handle value in a
 subsequent call to ReadFile.

 lpWritePipe - Returns a handle to the write side of the pipe.  Data
 may be written to the pipe by specifying this handle value in a
 subsequent call to WriteFile.

 lpPipeAttributes - An optional parameter that may be used to specify
 the attributes of the new pipe.  If the parameter is not
 specified, then the pipe is created without a security
 descriptor, and the resulting handles are not inherited on
 process creation.  Otherwise, the optional security attributes
 are used on the pipe, and the inherit handles flag effects both
 pipe handles.

 nSize - Supplies the requested buffer size for the pipe.  This is
 only a suggestion and is used by the operating system to
 calculate an appropriate buffering mechanism.  A value of zero
 indicates that the system is to choose the default buffering
 scheme.

 Return Value:

 TRUE - The operation was successful.

 FALSE/NULL - The operation failed. Extended error status is available
 using GetLastError.
 */

static BOOL APIENTRY MyCreatePipeEx(OUT LPHANDLE lpReadPipe, OUT LPHANDLE lpWritePipe,
                                    IN LPSECURITY_ATTRIBUTES lpPipeAttributes, IN DWORD nSize,
                                    DWORD dwReadMode, DWORD dwWriteMode) {
    HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    CHAR PipeNameBuffer[MAX_PATH];

    //
    // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
    //
    if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (nSize == 0) {
        nSize = 4096;
    }

    sprintf(PipeNameBuffer, "\\\\.\\Pipe\\ChessCore.%08x.%08x",
            GetCurrentProcessId(), PipeSerialNumber++);

    ReadPipeHandle = ::CreateNamedPipeA(PipeNameBuffer,
                                      PIPE_ACCESS_INBOUND | dwReadMode,
                                      PIPE_TYPE_BYTE | PIPE_WAIT,
                                      1,            // Number of pipes
                                      nSize,        // Out buffer size
                                      nSize,        // In buffer size
                                      120 * 1000,   // Timeout in ms
                                      lpPipeAttributes
                                     );

    if (ReadPipeHandle == 0) {
        return FALSE;
    }

    WritePipeHandle = ::CreateFileA(PipeNameBuffer, GENERIC_WRITE,
                                  0,            // No sharing
                                  lpPipeAttributes,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | dwWriteMode,
                                  NULL          // Template file
                                 );

    if (WritePipeHandle == INVALID_HANDLE_VALUE) {
        dwError = ::GetLastError();
        ::CloseHandle(ReadPipeHandle);
        ::SetLastError(dwError);
        return FALSE;
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return TRUE;
}

#endif // WINDOWS
