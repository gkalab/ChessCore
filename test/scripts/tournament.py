#!/usr/bin/env python
#
# Play a tournament between two engines
#
# usage: tournament.py [--engine1=engine --engine2=engine --numgames=number --timelimit=time --ecofile=file --logcomms=0/1]
#

import common, os

def main():
    common.parser().add_option("--engine1", dest = "engine1", help = "Engine #1")
    common.parser().add_option("--engine2", dest = "engine2", help = "Engine #2")
    common.parser().add_option("--numgames", dest = "numgames", type = "int", default = 5, help = "Number of games")
    common.parser().add_option("--timelimit", dest = "timelimit", default = "10m", help = "Time limit")
    common.parser().add_option("--ecofile", dest = "ecofile", help = "The (.cfdb) database containing the ECO classification")
    common.parser().add_option("--logcomms", dest = "logcomms", type = "int", default = 0, help = "Log UCI comms")

    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    debuglog = True
    logcomms = common.options().logcomms != 0
    logfile = common.tmpdir() + "/ccore.log"
    pgnfile = common.tmpdir() + "/games.pgn"
    configfile = common.configfile()
    ecofile = common.options().ecofile
    if not ecofile:
        ecofile = common.testdir() + "/cfdb/eco.cfdb"
    engine1 = common.options().engine1
    if not engine1:
        engine1 = common.engine1()
    engine2 = common.options().engine2
    if not engine2:
        engine2 = common.engine2()
    numgames = common.options().numgames
    timelimit = common.options().timelimit

    if os.path.exists(logfile):
        os.remove(logfile)

    cmdline = common.ccore()
    if debuglog:
        cmdline += " --debuglog"
    if logcomms:
        cmdline += " --logcomms"
    cmdline += " -c {0} -l {1} -o {2} -E {3} -n {4} -t {5} tournament {6} {7}".format(configfile, logfile, pgnfile, ecofile, numgames, timelimit, engine1, engine2)
    if common.runccore(cmdline):
        common.checkLogfile(logfile)

if __name__ == "__main__":
    main()
