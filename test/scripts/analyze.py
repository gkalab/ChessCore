#!/usr/bin/env python
#
# Analyze games from a database
#

import common, os

def usage():
    print "usage: {0} [options] --indb=database --outdb=database".format(common.progname())
    print "options:"
    print "    --indb=database         Input database"
    print "    --outdb=database        Output database"
    print "    --firstgame=number      The first game in the input database to analyze."
    print "    --lastgame=number       The last game in the input database to analyze"
    print "    --timelimit=time        The time to analyze each move"
    print "    --depth=depth           The depth to analyze each move"
    print "    --engine=engine         The engine to use to analyze"
    exit(1)

def main():
    common.parser().add_option("--indb", dest = "indb", help = "Input database")
    common.parser().add_option("--outdb", dest = "outdb", help = "Output database")
    common.parser().add_option("--timelimit", dest="timelimit", help = "Time limit per move")
    common.parser().add_option("--depth", dest="depth", help = "Depth limit")
    common.parser().add_option("--firstgame", dest="firstgame", help = "First game in input database to analyze")
    common.parser().add_option("--lastgame", dest="lastgame",help = "First game in input database to analyze")
    common.parser().add_option("--engine", dest="engine", help = "The engine to use to analyze the games")
    
    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    debuglog = True
    logcomms = False
    logfile = common.tmpdir() + "/ccore.log"
    configfile = common.configfile()
    indb = common.options().indb
    if not indb:
        usage()
    outdb = common.options().outdb
    if not outdb:
        usage()
    timelimit = common.options().timelimit;
    depth = common.options().depth
    if not timelimit and not depth:
        timelimit = "10s"
    elif timelimit and depth:
        print "Both timelimit and depth specified; ignoring depth option"
        unset(depth)
    firstgame = common.options().firstgame
    lastgame = common.options().lastgame
    engine = common.options().engine
    if not engine:
        engine = common.engine1()

    if os.path.exists(logfile):
        os.remove(logfile)

    cmdline = common.ccore()
    if debuglog:
        cmdline += " --debuglog"
    if logcomms:
        cmdline += " --logcomms"
    if timelimit:
        cmdline += " -t {0}".format(timelimit)
    if depth:
        cmdline += " -d {0}".format(depth)
    if firstgame:
        cmdline += " -n {0}".format(firstgame)
    if lastgame:
        cmdline += " -N {0}".format(lastgame)

    cmdline += " -c {0} -l {1} -i {2} -o {3} analyze {4}".format(configfile, logfile, indb, outdb, engine)
    if common.runccore(cmdline):
        common.checkLogfile(logfile)

if __name__ == "__main__":
    main()
