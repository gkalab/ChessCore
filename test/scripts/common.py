#!/usr/bin/env python
#
# Common utility functions
#
# The following command line options are supported:
#
# --leakcheck: Invoke the leak detection tool when executing ccore.
# --profile: Invoke the profiling tool when executing ccore.

import os, sys, re, shutil, subprocess, tempfile
from optparse import OptionParser

if __name__ == "__main__":
    print "This module is not supposed to be run directly"
    exit(1)

_parser = OptionParser()
_progname = ""
_scriptdir = ""
_testdir = ""
_rootdir = ""
_tmpdir = ""
_ccore = ""
_configfile = ""
_checktool = ""
_options = dict()
_args = ""
_engine1 = ""
_engine2 = ""

def init(filename):
    global _parser, _progname, _scriptdir, _testdir, _rootdir, _tmpdir, _ccore, _configfile, _checktool, _options, _args, _engine1, _engine2
    _progname = os.path.basename(filename)
    _scriptdir = os.path.dirname(filename)
    _testdir = os.path.abspath(os.path.join(_scriptdir, os.pardir))
    _rootdir = os.path.abspath(os.path.join(_testdir, os.pardir))
    _tmpdir = tempfile.gettempdir()
    
    _parser.add_option("--leakcheck", dest = "leakcheck", default = False, help = "Invoke the leak detection tool when executing ccore")
    _parser.add_option("--profile", dest = "profile", default = False, help = "Invoke the profile tool when executing ccore")

    # Parse the options into a temporary dictionary and then copy each item over, to avoid destroying
    # _options if no options have been parsed
    (_options, _args) = _parser.parse_args()
    if _options.leakcheck:
        if sys.platform == "linux2":
            _checktool = "valgrind"
        elif sys.platform == "darwin":
            _checktool="iprofiler -T 1000s -leaks -d '{0}'".format(_tmpdir)
            dtpsdir = _tmpdir + "/ccore.dtps"
            if os.path.exists(dtpsdir):
                shutil.rmtree(dtpsdir)
        else:
            print "{0}: No leakcheck tool on this platform".format(_progname)
            return False
    elif _options.profile:
        if sys.platform == "darwin":
            _checktool="iprofiler -T1000s -timeprofiler -d '{0}'".format(_tmpdir)
            dtpsdir = _tmpdir + "/ccore.dtps"
            if os.path.exists(dtpsdir):
                shutil.rmtree(dtpsdir)
        else:
            print "{0}: No profiling tool on this platform".format(_progname)
            return False

    if _checktool:
        print "{0}: Using tool '{1}'".format(_progname, _checktool)

    if not os.path.exists(_tmpdir):
        print "{0}: Creating directory {1}".format(_progname, _tmpdir)
        os.makedirs(_tmpdir)

    if sys.platform == "win32":
        ccoreexe = "ccore.exe"
    else:
        ccoreexe = "ccore"

    _ccore = _rootdir + "/bin/" + ccoreexe
    if not (os.path.isfile(_ccore) and os.access(_ccore, os.X_OK)):
        print "{0}: Cannot find ccore executable '{1}'".format(_progname, _ccore)
        return False

    print "{0}: Using ccore binary '{1}'".format(_progname, _ccore)
    print "{0}: Using temp directory '{1}'".format(_progname, _tmpdir)

    # Configure ccore config file and default engines
    if sys.platform == "linux2":
        config = "linux"
        _engine1 = "komodo"
        _engine2 = "stockfish231"
    elif sys.platform == "darwin":
        config = "macosx"
        _engine1 = "critter"
        _engine2 = "stockfish4"
    elif sys.platform == "win32":
        config = "windows"
        _engine1 = "komodo3"
        _engine2 = "stockfish3"
    else:
        print "{0}: No configuration for this platform".format(_progname)
        return False

    _configfile = _testdir + "/config/" + config + "_local.cfg"
    if not os.path.isfile(_configfile):
        _configfile = _testdir + "/config/" + config + ".cfg"
        if not os.path.exists(_configfile):
            print "{0}: Configuration file '{1}' does not exist".format(_progname, _configfile)
            return False
    print "{0}: Using configuration file '{1}'".format(_progname, _configfile)

    return True

def runccore(cmdline):
    if sys.platform == "win32":
        cmdline = cmdline.replace("/", "\\")
    if _checktool:
        cmdline = _checktool + " " + cmdline
    print "{0} calling {1}".format(_progname, cmdline)
    try:
        exitcode = subprocess.call(cmdline, shell = True)
        if exitcode != 0:
            print "{0}: Exit code {1} running ccore".format(_progname, exitcode)
            return False
    except (KeyboardInterrupt, SystemExit):
        print "Cancelled by user"
        return False
    return True

def checkLogfile(logfile):
    if not os.path.exists(logfile):
        print "{0}: Logfile {1} does not exist!".format(_progname, logfile)
        return False
    print "{0}: Checking logfile {1}".format(_progname, logfile)
    foundErr = False
    f = open(logfile, "r")
    for line in f:
        line = line.strip()
        if re.search("WRN", line) or re.search("ERR", line):
            print line
            foundErr = True
    return not foundErr

def parser():
    return _parser

def progname():
    return _progname

def scriptdir():
    return _scriptdir

def testdir():
    return _testdir

def rootdir():
    return _rootdir

def tmpdir():
    return _tmpdir

def ccore():
    return _ccore

def configfile():
    return _configfile

def checktool():
    return _checktool

def options():
    return _options

def args():
    return _args

def engine1():
    return _engine1

def engine2():
    return _engine2

