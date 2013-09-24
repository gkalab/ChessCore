//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// IndexManager.cpp: IndexManager class implementation.
//

#include <ChessCore/IndexManager.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>
#include <string.h>
#include <errno.h>

using namespace std;

namespace ChessCore {
const char *IndexManager::m_classname = "IndexFile";

IndexManager::IndexManager():m_rootDir() {
}

IndexManager::IndexManager(const string &rootDir):m_rootDir() {
    setRootDir(rootDir);
}

IndexManager::~IndexManager() throw() {
}

bool IndexManager::setRootDir(const string &rootDir) {
    m_rootDir.clear();

    if (!Util::dirExists(rootDir)) {
        if (!Util::createDirectory(rootDir)) {
            LOGERR << "Failed to create root directory '" << rootDir << "'";
            return false;
        }

        LOGDBG << "Create index root directory '" << rootDir << "'";
    }

    m_rootDir = rootDir;

    return true;
}

bool IndexManager::getIndexFile(const std::string &filename, fstream &indexFile, string &indexFilename) {
    indexFilename = getIndexFilenameForFile(filename);
    if (indexFilename.empty())
        return false;

    indexFile.open(indexFilename, ios::binary | ios::in | ios::out | ios::app);

    if (!indexFile.is_open()) {
        LOGERR << "Failed to open index file '" << indexFilename << "'";
        indexFilename.clear();
        return false;
    }

    return true;
}

bool IndexManager::deleteIndexFile(const string &filename) {
    string indexFilename = getIndexFilenameForFile(filename);
    if (indexFilename.empty())
        return false;

    if (!Util::deleteFile(indexFilename)) {
        LOGERR << "Failed to delete index file '" << indexFilename << "': " <<
            strerror(errno) << " (" << errno << ")";
        return false;
    }

    return true;
}

string IndexManager::getIndexFilenameForFile(const string &filename) const {
    string uniqueName = Util::getUniqueName(filename);
    if (uniqueName.empty()) {
        LOGERR << "Failed to generate unique name for file '" << filename << "'";
        return "";
    }
    return Util::format("%s%c%s.index", m_rootDir.c_str(), PATHSEP, uniqueName.c_str());
}
}   // namespace ChessCore
