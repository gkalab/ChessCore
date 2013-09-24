//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// AppleUtil.h: Apple-specific utility functions.
//

#pragma once

#include <ChessCore/ChessCore.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the app's temporary directory into the specified buffer.
 *
 * @param buffer Where to store the temporary directory.
 * @param buflen The size of the buffer pointed to by 'buffer'.
 *
 * @return TRUE if the temporary directory was successfully retrieved, else FALSE.
 */
extern int CHESSCORE_EXPORT appleTempDir(char *buffer, unsigned buflen);

#ifdef __cplusplus
}   // extern "C"
#endif
