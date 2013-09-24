#!/usr/bin/env python
#
# Generate versioned source files if they don't exist or if other files in the
# same directory have been modified since they were last generated.
#
# usage: upver.py infile.ver outfile.cpp outfile.h
#

import sys, os, subprocess, re, string
if sys.platform != "win32":
    import signal

progname = ""

def sigHandler(signum, frame):
    """ Handle signals """
    sys.stderr.write("{0}: Terminated with signal {1}\n".format(progname, signum))
    sys.exit(2)

def samefile(file1, file2):
    return os.stat(file1) == os.stat(file2)

def checkupdated(verfile, cppfile, hfile):
    """ Check if any source files have been modified since any of the files passed in """

    if not os.path.exists(verfile) or not os.path.exists(cppfile) or not os.path.exists(hfile):
        sys.stdout.write("{0}: {1}, {2} or {3} don't exist\n".format(progname, verfile, cppfile, hfile))
        return True

    verstat = os.stat(verfile)
    cppstat = os.stat(cppfile)
    hstat = os.stat(hfile)
    if verstat.st_mtime > cppstat.st_mtime or \
        verstat.st_mtime > hstat.st_mtime:
        sys.stdout.write("{0}: {1} is newer than version source files\n".format(progname, verfile))
        return True

    srcdirname = os.path.dirname(verfile)
    allnames = []
    for dirname, dirnames, filenames in os.walk('.'):
        for subdirname in dirnames:
            allnames.append(os.path.join(dirname, subdirname))
        for filename in filenames:
            allnames.append(os.path.join(dirname, filename))
    
    for filename in allnames:
        if samefile(filename, verfile) or \
            samefile(filename, cppfile) or \
            samefile(filename, hfile):
            continue
        extension = os.path.splitext(filename)[1]
        if extension not in ('.c', '.cpp', '.S', '.l', '.h'):
            continue
        filestat = os.stat(filename)
        if filestat.st_mtime > verstat.st_mtime or \
            filestat.st_mtime > cppstat.st_mtime or \
            filestat.st_mtime > hstat.st_mtime:
            sys.stderr.write("{0}: {1} is out of date\n".format(progname, filename))
            return True

    return False
        
def main(argv = None):
    """ Program entry point """
    global progname

    if argv is None:
        argv = sys.argv

    progname = os.path.basename(argv[0])
    if len(argv) != 4:
        sys.stderr.write("Usage: {0} infile.ver outfile.cpp outfile.h\n".format(progname))
        sys.exit(1)
    vername = argv[1]
    cppname = argv[2]
    hname = argv[3]
        
    if sys.platform != "win32":
        for sig in (signal.SIGINT, signal.SIGHUP, signal.SIGTERM):
            signal.signal(sig, sigHandler)

    try:
        version = None
        build = None
        verfile = open(vername, "r")
        for line in verfile:
            match = re.match(r'^version\s+(\S+)', line)
            if match:
                version = match.group(1)
            match = re.match(r'^build\s+(\S+)', line)
            if match:
                build = int(match.group(1))
        if version == None or build == None:
            sys.stderr.write("{0}: Failed to find version/build in {1}\n".format(progname, vername))
            return 3

        update = checkupdated(vername, cppname, hname)
        bump = subprocess.Popen("git ls-files -m", stdout=subprocess.PIPE, shell=True).stdout.read().strip()
        commit = subprocess.Popen("git rev-parse HEAD", stdout=subprocess.PIPE, shell=True).stdout.read().strip()

        if update:
            if bump:
                build += 1
                sys.stdout.write("{0}: Incremented to build {1}\n".format(progname, build))
            else:
                sys.stdout.write("{0}: Staying at build {1}\n".format(progname, build))
            verfile = open(vername, "w")
            verfile.write('version {0}\n'.format(version))
            verfile.write('build {0}\n'.format(build))
            verfile.close()
        
        cppfile = open(cppname, "w")
        cppfile.write('#include <ChessCore/{0}>\n'.format(os.path.basename(hname)))
        cppfile.write('const std::string ChessCore::g_version("{0}");\n'.format(version))
        cppfile.write('const std::string ChessCore::g_build("{0}");\n'.format(build))
        cppfile.write('const std::string ChessCore::g_commit("{0}");\n'.format(commit))
        cppfile.close()
        
        # Turn '/path/to/Version.h' into 'VERSION_H'
        guardmacro = '_'.join(os.path.basename(hname).upper().split('.'))
        
        hfile = open(hname, "w")
        hfile.write('#ifndef {0}\n'.format(guardmacro))
        hfile.write('#define {0}\n'.format(guardmacro))
        hfile.write('#include <ChessCore/ChessCore.h>\n')
        hfile.write('namespace ChessCore\n')
        hfile.write('{\n')
        hfile.write('extern CHESSCORE_EXPORT const std::string g_version;\n')
        hfile.write('extern CHESSCORE_EXPORT const std::string g_build;\n')
        hfile.write('extern CHESSCORE_EXPORT const std::string g_commit;\n');
        hfile.write('}\n')
        hfile.write('#endif // {0}\n'.format(guardmacro))
        hfile.close()

    except IOError, msg:
        sys.stderr.write("{0}: exception {1}\n".format(progname, msg))
        return 2
    return 0

if __name__ == "__main__":
    sys.exit(main())
