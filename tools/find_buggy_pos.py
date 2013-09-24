#!/usr/bin/env python
#
# This script is being used to find the deeply hidden bug in the move generator!
#
# It attempts to generate a position where ccore's perftdiv (at depth 1) is different
# to that given by critter.  
#
# usage: find_buggy_pos.py [number|filename]
#        If filename is specified then FENs are read from the file, else they are generated
#        using ccore's randompos function.
#

import sys, os, subprocess, re, string
if sys.platform != "win32":
    import signal

progname = ""
critterPath = "/Users/andy/Documents/ChessEngines/critter16a/critter-16a"
ccorePath = "build/ccore -q findbuggypos"

def sigHandler(signum, frame):
    """ Handle signals """
    sys.stderr.write("{0}: Terminated with signal {1}\n".format(progname, signum))
    sys.exit(2)

def randomPos(ccore):
    ccore.stdin.write("randompos\n")
    fen = ccore.stdout.readline().strip()
    #print "got fen", fen
    return fen

def ccoreNodeCount(ccore, fen):
    #print "writing fen {0}".format(fen)
    ccore.stdin.write("setboard {0}\n".format(fen))
    ccore.stdin.write("perftdiv 1\n")
    nodes = ccore.stdout.readline().strip()
    #print "got nodes", nodes
    return int(nodes)

def critterNodeCount(critter, fen):
    #print "writing fen {0}".format(fen)
    critter.stdin.write("setboard {0}\n".format(fen))
    critter.stdin.write("divide 1\n")
    nodes = 0
    while not nodes:
        line = critter.stdout.readline()
        #print line
        match = re.match(r'^(\d+) nodes in.*$', line)
        if match:
            nodes = match.group(1)
    return int(nodes)

def main(argv = None):
    """ Program entry point """
    global progname
    filename = ""
    count = 1000

    if sys.platform != "win32":
        for sig in (signal.SIGINT, signal.SIGHUP, signal.SIGTERM):
            signal.signal(sig, sigHandler)

    if argv is None:
        argv = sys.argv
    progname = os.path.basename(argv[0])
    if len(argv) == 2:
        if os.path.isfile(argv[1]):
            filename = argv[1]
        else:
            count = int(argv[1])

    try:
        # Keep the ccore and criter sub-processes permanently open, to save time and resources
        # continually starting them.
        ccore = subprocess.Popen(ccorePath, stdin = subprocess.PIPE,
            stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell=True)
        critter = subprocess.Popen(critterPath, stdin = subprocess.PIPE,
            stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell=True)

        if filename:
            print "Reading positions from", filename
            i = 0
            fenfile = open(filename)
            for fen in fenfile:
                fen = fen.strip()
                i += 1
                expected = critterNodeCount(critter, fen)
                actual = ccoreNodeCount(ccore, fen)
                if expected != actual:
                    print i+1, "MISMATCH:", fen, "(", actual, "!=", expected, ")"
                    return 1
                else:
                    print i+1, "match", fen                
        else:
            print "Using", count, "random positions"
            for i in range(0, count):
                fen = randomPos(ccore)
                if fen:
                    expected = critterNodeCount(critter, fen)
                    actual = ccoreNodeCount(ccore, fen)
                    if expected != actual:
                        print i+1, "MISMATCH:", fen, "(", actual, "!=", expected, ")"
                        return 1
                    else:
                        print i+1, "match", fen

    except IOError, msg:
        print "{0}: exception {1}\n".format(progname, msg)
        return 2
    return 0

if __name__ == "__main__":
    sys.exit(main())
