#!/usr/bin/env python
#
# Classify a database.
#

import common, os

def usage():
    print "usage {0} [options]".format(common.progname())
    print "options:"
    print "    --indb=database         The input database"
    print "    --outdb=database        The output database"    
    print "    --firstgame=number      The first game in the input database to analyze"
    print "    --lastgame=number       The last game in the input database to analyze"
    print "    --ecofile=database      The (.cfdb) database containing the ECO classification"
    exit(1)

def main():
    common.parser().add_option("--indb", dest = "indb", help = "Input database")
    common.parser().add_option("--outdb", dest = "outdb", help = "Output database")
    common.parser().add_option("--firstgame", dest="firstgame", help = "First game in input database to analyze")
    common.parser().add_option("--lastgame", dest="lastgame",help = "First game in input database to analyze")
    common.parser().add_option("--ecofile", dest = "ecofile", help = "The (.cfdb) database containing the ECO classification")

    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    debuglog = True
    logcomms = False
    logfile1 = common.tmpdir() + "/ccore1.log"
    logfile2 = common.tmpdir() + "/ccore2.log"
    configfile = common.configfile()
    indb = common.options().indb
    if not indb:
        indb = common.testdir() + "/pgn/Boris_Spassky.pgn"
    outdb = common.options().outdb
    if not outdb:
        outdb = common.tmpdir() + "/classified.cfdb"
        if os.path.exists(outdb):
            os.remove(outdb)
    ecofile = common.options().ecofile
    if not ecofile:
        ecofile = common.testdir() + "/cfdb/eco.cfdb"
    firstgame = common.options().firstgame
    lastgame = common.options().lastgame

    if os.path.exists(logfile1):
        os.remove(logfile1)
    if os.path.exists(logfile2):
        os.remove(logfile2)

    # Take a copy of the database
    cmdline = common.ccore()
    if debuglog:
        cmdline += " --debuglog true"
    if logcomms:
        cmdline += " --logcomms true"
    if firstgame:
        cmdline += " -n {0}".format(firstgame)
    if lastgame:
        cmdline += " -N {0}".format(lastgame)
    cmdline += " -l {0} -i {1} -o {2} copydb".format(logfile1, indb, outdb);
    if common.runccore(cmdline):
        common.checkLogfile(logfile1)

        # And then classify the copy
        cmdline = common.ccore()
        if debuglog:
            cmdline += " --debuglog true"
        if logcomms:
            cmdline += " --logcomms true"
        cmdline += " -l {0} -i {1} -E {2} classify".format(logfile2, outdb, ecofile)
        if common.runccore(cmdline):
            common.checkLogfile(logfile2)

if __name__ == "__main__":
    main()
