#!/usr/bin/env python
#
# Test databases by copying between PGN and CFDB.
#

import common, sys, os, re, difflib

def usage():
    print "usage: {0} [options]".format(common.progname())
    print "options:"
    print "    --indb=database         The input database"
    exit(1)

def run(runcycle, indb, outdb):
    progname = common.progname()
    logfile1 = common.tmpdir() + "/copy" + str(runcycle) + ".log"
    logfile2 = common.tmpdir() + "/validate" + str(runcycle) + ".log"
    if not os.path.exists(indb):
        print "{0}: Input database {1} does not exist".format(progname, indb)
        return False
    if os.path.exists(logfile1):
        os.remove(logfile1)
    if os.path.exists(logfile2):
        os.remove(logfile2)
    if os.path.exists(outdb):
        os.remove(outdb)

    print "========================================================================================"
    print "{0}: Copying {1} to {2}".format(progname, indb, outdb);

    cmdline = common.ccore()
    cmdline += " -l {0} -i {1} -o {2} --quiet=true --debuglog=true copydb".format(logfile1, indb, outdb)
    if not common.runccore(cmdline):
        return False

    common.checkLogfile(logfile1)

    # Check that the copy was successful
    found = False
    f = open(logfile1, "r")
    for line in f:
        if re.search("Successfully copied database", line):
            found = True
            break
    if found:
        print "{0}: Copy successful".format(progname)
    else:
        print "{0}: Copy failed".format(progname)
        return False

    print "----------------------------------------------------------------------------------------"
    print "{0}: Validating {1}".format(progname, outdb);

    cmdline = common.ccore()
    cmdline += " -l {0} -i {1} --quiet true --debuglog true validatedb".format(logfile2, outdb)
    if not common.runccore(cmdline):
        return False

    common.checkLogfile(logfile2)

    # Check that the validate was successful
    found = False
    f = open(logfile2, "r")
    for line in f:
        if re.search("Database is valid", line):
            found = True
            break
    if found:
        print "{0}: Validate successful".format(progname)
    else:
        print "{0}: Validate failed".format(progname)
        return False

    return True

def main():
    common.parser().add_option("--indb", dest = "indb", help = "Input database")

    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    indb = common.options().indb
    if not indb:
        indb = common.testdir() + "/pgn/Kramnik.pgn"
    temppgn1 = common.tmpdir() + "/copydb1.pgn"
    temppgn2 = common.tmpdir() + "/copydb2.pgn"
    tempcfdb1 = common.tmpdir() + "/copydb1.cfdb"

    if run(1, indb, tempcfdb1) and \
        run(2, tempcfdb1, temppgn1) and \
        run(3, indb, temppgn2):
        # temppgn1 and temppgn2 should be identical
        if os.path.exists(temppgn1) and os.path.exists(temppgn2):
            lines1 = open(temppgn1, 'U').readlines()
            lines2 = open(temppgn2, 'U').readlines()
            diff = difflib.unified_diff(lines1, lines2)
            different = False;
            first = True
            for line in diff:
                if first:
                    print "{0}: PGN files {1} and {2} are different:".format(progname, temppgn1, temppgn2)
                    different = True
                    first = False;
                print line
            if not different:
                    print "{0}: Test successful".format(progname)
        else:
            print "{0}: Failed; no output files to compare".format(progname)

if __name__ == "__main__":
    main()
