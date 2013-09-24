#!/usr/bin/env python
#
# Generate a source file from a lex source file, post-processing the
# generated source to replace the functionality of yy_fatal_error(),
# which is called directly in places and not via the macro YY_FATAL_ERROR().
#
# Changes the implementation from:
#
# static void yy_fatal_error (yyconst char* msg , yyscan_t yyscanner)
# {
#	  (void) fprintf( stderr, "%s\n", msg );
#	  exit( YY_EXIT_FAILURE );
# }
#
# to:
#
# static void yy_fatal_error (yyconst char* msg , yyscan_t yyscanner)
# {
#	  throw ChessCoreException(msg);
# }
#
# It also removes any '#line' directives and comment sections starting
# with '/**' as the generated code causes warnings with the Apple LLVM 5.0
# compiler with the -Wdocumentation flag (the generated code has misnamed
# parameters).
#
# usage: flex.py input.l output.cpp
#

import sys, os, atexit, tempfile, subprocess, re
if sys.platform != "win32":
	import signal

progname = None
tempfile = tempfile.mkstemp(suffix='')		 # [0]=file handle, [1]=file name

def fini():
	""" Clean-up temporary file """
	os.remove(tempfile[1])

def sigHandler(signum, frame):
	""" Handle signals """
	sys.stderr.write("{0}: Terminated with signal {1}\n".format(progname, signum))
	sys.exit(2)

def main(argv = None):
	""" Program entry point """
	global progname, tempfile

	if argv is None:
		argv = sys.argv

	progname = os.path.basename(argv[0])

	if len(argv) != 3:
		sys.stderr.write("Usage: {0} input.l output.cpp\n".format(progname))
		sys.exit(1)
	inname = argv[1]
	outname = argv[2]

	if sys.platform != "win32":
		for sig in (signal.SIGINT, signal.SIGHUP, signal.SIGTERM):
			signal.signal(sig, sigHandler)
	atexit.register(fini)

	command = "flex --noline -o{0} {1}".format(tempfile[1], inname)
	if subprocess.call(command, shell=True) != 0:
		sys.stderr.write("{0}: Failed to process input file {1}\n".format(progname, inFile))
		sys.exit(3)
		
	foundFunc = False
	inComment = False
	funcArgs = ""
	count = 0
	try:
		infile = os.fdopen(tempfile[0])
		outfile = open(outname, "w")
		for line in infile:
			if re.match(r'^#line.+', line):
				continue			# Ignore #line directives
			if re.match(r'^/\*\*.+', line):
				inComment = True
				continue
			if inComment:
				if re.match(r'\s*\*/\s*', line):
					inComment = False
				continue
			if not foundFunc:
				match = re.match(r'^static void yy_fatal_error.*\((.+)\)(?!;)$', line)
				if match:
					foundFunc = True
					funcArgs = match.group(1)
				else:
					outfile.write(line)
			else:
				if re.match(r'^}', line):
					outfile.write('// This function has been modified by {0}\n'.format(progname))
					outfile.write('static void yy_fatal_error({0})\n'.format(funcArgs))
					outfile.write('{\n')
					outfile.write('    logerr("Fatal parse error: %s", msg);\n');
					outfile.write('    throw ChessCoreException(msg);\n')
					outfile.write('}\n')
					foundFunc = False
					count += 1
	except IOError, msg:
		sys.stderr.write("{0}: exception {1}\n".format(progname, msg))
		return 2

	if count > 0:
		retval = 0
	else:
		sys.stderr.write("Failed to replace function!\n")
		retval = 1
	return retval

if __name__ == "__main__":
	sys.exit(main())
