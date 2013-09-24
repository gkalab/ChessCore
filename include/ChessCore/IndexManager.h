//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// IndexManager.h: IndexManager class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Blob.h>
#include <sys/stat.h>
#include <string>
#include <map>
#include <fstream>

namespace ChessCore {
/**
 * The IndexManager class is used to store index files (typically for PGN files) in
 * a temporary directory.  The index file is stored in the root directory using the
 * device and inode number of the file being indexed:
 *
 * deviceinode.index
 *
 * Where device and inode are a 16-digit hex number.
 */

class CHESSCORE_EXPORT IndexManager {
private:
    static const char *m_classname;

protected:
    std::string m_rootDir;

public:
    IndexManager();

    /**
     * @see setRootDir().
     */
    IndexManager(const std::string &rootDir);

    virtual ~IndexManager() throw();

    /**
     * Get the current root directory.
     *
     * @return The current root directory.  If this is empty then the object
     * is not initialised.
     */
    const std::string &rootDir() const {
        return m_rootDir;
    }

    /**
     * Set the root directory.
     *
     * @param rootDir The root directory for the index.  If this doesn't
     * exist then it is created.
     *
     * @return true if the root directory was successfully opened/created.
     */
    bool setRootDir(const std::string &rootDir);

    /**
     * Open or create the index file for the specified filename.
     *
     * @param filename The name of the file the index file will be indexing.
     * This file must exist on the filesystem in order to get its device/inode.
     * @param indexFile The index file.
     * @param indexFilename The fullpath of the index file.
     *
     * @return true if the index file was opened/created successfully, else
     * false.
     */
    bool getIndexFile(const std::string &filename, std::fstream &indexFile, std::string &indexFilename);

    /**
     * Delete the index file for the specified file.
     *
     * @param filename The name of the file that was being indexed.
     *
     * @return true if the index file was successfully removed.
     */
    bool deleteIndexFile(const std::string &filename);

protected:
    std::string getIndexFilenameForFile(const std::string &filename) const;
};
}   // namespace ChessCore
