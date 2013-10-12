#!/usr/bin/env python
#
# Create eco.cfdb.
#

import common, os

def usage():
    print "usage: {0} [options]".format(common.progname())
    print "options:"
    print "    --indb=database         Input database (in a special format; see doc/OpeningClassification/eco.pgn"
    print "    --outdb=database        Output database (must be a .cfdb file)"
    exit(1)

def main():
    common.parser().add_option("--indb", dest = "indb", help = "Input database")
    common.parser().add_option("--outdb", dest = "outdb", help = "Output database")
    
    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    debuglog = True
    logfile1 = common.tmpdir() + "/ccore1.log"
    logfile2 = common.tmpdir() + "/ccore2.log"
    indb = common.options().indb
    if not indb:
        indb = common.rootdir() + "/doc/OpeningClassification/eco.pgn"
    outdb = common.options().outdb
    if not outdb:
        outdb = common.testdir() + "/cfdb/eco.cfdb"

    if os.path.exists(logfile1):
        os.remove(logfile1)
    if os.path.exists(logfile2):
        os.remove(logfile2)
    if os.path.exists(outdb):
        os.remove(outdb)

    # Copy the database
    cmdline = common.ccore()
    if debuglog:
        cmdline += " --debuglog true"

    cmdline += " -l {0} -i {1} -o {2} copydb".format(logfile1, indb, outdb)
    if common.runccore(cmdline):
        common.checkLogfile(logfile1)

        # Create the opening tree in the output database
        cmdline = common.ccore()
        if debuglog:
            cmdline += " --debuglog true"
        cmdline += " -l {0} -i {1} -d 100 buildoptree".format(logfile2, outdb)
        if common.runccore(cmdline):
            common.checkLogfile(logfile2)

if __name__ == "__main__":
    main()
