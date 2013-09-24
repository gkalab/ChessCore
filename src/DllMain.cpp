//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// DllMain.cpp: Dll entry point.
//

#include <ChessCore/ChessCore.h>
#include <ChessCore/Log.h>

static const char *m_classname = "";

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            LOGDBG << "Process attach";
            break;
        case DLL_PROCESS_DETACH:
            LOGDBG << "Process detach";
            break;
        case DLL_THREAD_ATTACH:
            LOGDBG << "Thread attach";
            break;
        case DLL_THREAD_DETACH:
            LOGDBG << "Thread detach";
            break;
        default:
            break;
    }
    return TRUE;
}
