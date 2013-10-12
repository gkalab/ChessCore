#!/usr/bin/env python
#
# Process an EPD file using an engine
#

import common, os

def usage():
    print "usage {0} [options] --epdfile=epdfile".format(common.progname())
    print "options:"
    print "    --epdfile=epdfile       Input EPD file"
    print "    --timecontrol=time      The time to analyze each EPD entry"
    print "    --engine=engine         The Engine to use to process the EPD file"
    exit(1)

def main():
    common.parser().add_option("--epdfile", dest = "epdfile", help = "Input EPD file")
    common.parser().add_option("--timecontrol", dest="timecontrol", default = "10s", help = "Time control")
    common.parser().add_option("--engine", dest="engine", help = "The engine to use to process the EPD file")

    if not common.init(__file__):
        exit(2)

    progname = common.progname()
    debuglog = True
    logcomms = False
    logfile = common.tmpdir() + "/ccore.log"
    configfile = common.configfile()
    epdfile = common.options().epdfile
    if not epdfile:
        usage()
    timecontrol = common.options().timecontrol
    engine = common.options().engine
    if not engine:
        engine = common.engine1()

    if os.path.exists(logfile):
        os.remove(logfile)

    # Take a copy of the database
    cmdline = common.ccore()
    if debuglog:
        cmdline += " --debuglog true"
    if logcomms:
        cmdline += " --logcomms true"
    cmdline += " -c {0} -l {1} -e {2} -t {3} processepd {4}".format(configfile, logfile, epdfile, timecontrol, engine);
    if common.runccore(cmdline):
        common.checkLogfile(logfile)

if __name__ == "__main__":
    main()
