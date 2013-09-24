CUTE
====

ChessCore Test Tool is a tool I wrote to test my illfated chess engine 'Chimp'.  It has now become a
generalised test tool for ChessCore, the C++ library that forms the core of ChessFront.

Database
--------
CCore currently supports reading and writing the following database files:

### PGN

Portable Game Notation: A text file used for interchange purposes.  ChessLib will only write to the end
of a PGN file and will create an index file (.pgi) to help improve random access performance into the PGN
file.  This index can only be used by ChessLib and so shouldn't be distributed with a PGN file.  (ChessBase
also generate a PGN index file for the same purpose, though its format is doubtless different; I will change
the index file extension to '.pgnindex' to avoid confusion with ChessBase's file).

PGN files generated by ChessLib should be viewable using any number of chess software.

### CFDB

ChessFront Database:  This is a proprietary database based on Sqlite3.  It is relational which should make
finding players, etc., very quick.

Engines
-------
Cute can use any UCI engine. The engine must be configured in the configuration file, which also contains
the settings you want to use with the engine.

Command Line Options
--------------------

The following command line options are common to all functions:

-l (--logfile):  Where to write the ChessLib logfile.  The default is ccore.log in the current directory.
-D (--debuglog): Turns on debug logging in the log file.
-L (--logcomms): Turns on logging of input/output to the UCI chess engines.  This logging is very verbose.
-r (--relaxed):  Don't terminate on errors in some situations (not advised).
-c (--cfgfile):  Configuration file, which defines the location and settings for UCI engines.

The following command line options specify various settings, which vary depending on the function being
performed:

-n (--number1):  Number, sometimes the first number in a range.
-N (--number2):  The second number in a range.
-t (--time):     Time, followed by 's' (seconds), 'm' (minutes) or 'h' (hours).
-d (--depth):    Depth.

Functions
---------

Not all functions are documented here, as they are either pointless or not completely implemented.

### tournament:  Play a tournament between two engines.  You must specify the configuration file (-c),
containing the engine details, the time for each engine per game (for example '-t 5m' for a 10 minute game)
and the number of games to play (for example '-n 5' for a 5-game tournament).
You can also specify an output database (with -o), where the tournament games will be stored.  Each move in
the game will contain the engine's 'score' (evaluation) of the current position.

### analyze: Analyze a database of games using an engine.  You must specify the configuration file (-c),
the input and output database, with the games written to the database containing the variations and score
from the analysis engine.  You also need to specify how much the engine will analyze each move, using either
time (-t) or depth (-d).

### processepd:  Test a chess engine using a EPD file.  The EPD file contains positions and a set of *opcodes*
that define various operations.  The most common is 'bm' (best move) and the idea is that you can test your
chess engine against known best moves in the given position.
You must specify the configuration file (-c) and the EPD file to process (-e),  and optionally the range
of EPD lines to process (-n and -N), if you don't want to process the whole file.

### copydb:  Copies a database.  You must specify the input (-i) and output (-o) database files, and ccore
will understand the format of each database by the file extension (.pgn or .cfdb).   You can also specify
the range of games to copy (-n and -N) if you don't want to copy the whole database.

### validatedb:  Validate a database.  The contents of the database are read in order to check for errors.
You must specify the input database (-i) and optionally the range of games to validate (-n and -N) if you
don't want to validate the whole database.