CFDB Chess Database Schema
==========================

This document covers the CFDB chess database schema version 1.

Author: [Andy Duplain](mailto:andy@trojanfoe.com).

Updated: 22-Sep-2013

Overview
--------

The CFDB database schema is a schema for storing chess games within a relational database.  It can also be used to store Opening Trees.

It is initially implemented using [sqlite3](http://www.sqlite.org), however the author believe the schema should work with little change on other relational database systems.

### Motivation
Within the Chess Community only the [Portable Game Notation](http://en.wikipedia.org/wiki/Portable_Game_Notation) exists as an open standard in which store chess games, however the PGN standard is a text-based file format lending itself mostly as an interchange format between chess user interfaces rather than a long term storage medium.  Any chess user interface that does use PGN to store games generally uses auxiliary data to overcome the short comings of using a text based file format.

Other chess formats have existed for many years, some of them closed and others open, however it was unclear to the author of the licensing of these formats and none, to his knowledge, had been published and made available for others to use.

CFDB hopes to overcome these short comings by providing a fast, free and open file format that anyone is free to use and improve upon.

Sqlite3 Database Schema
-----------------------

This section describes the CFDB database schema as implemented using sqlite3.  Changes to column types will be required if CFDB is implemented using any other relational database system.

### metadata

    CREATE TABLE metadata (
        name TEXT PRIMARY KEY,
        val TEXT)

##### Description

The `metadata` table holds information about the database itself.  The following meta-data is supported:

 - `schema_version`.  Holds the version of the database schema used by this database.  The current supported value is `1`.  Programs opening the CFDB database should read this meta-data first to ensure they support the version of the schema being used.

### game

    CREATE TABLE game (
       game_id INTEGER PRIMARY KEY,
       white_player_id INTEGER,
       black_player_id INTEGER,
       event_id INTEGER,
       site_id INTEGER,
       date INTEGER,
       round_major INTEGER,
       round_minor INTEGER,
       result INTEGER,
       annotator_id INTEGER,
       eco TEXT,
       white_elo INTEGER,
       black_elo INTEGER,
       time_control BLOB,
       halfmoves INTEGER,
       partial BLOB,
       moves BLOB,
       annotations BLOB)
       
#### game indexes

    CREATE UNIQUE INDEX game_index ON game (game_id)
    
##### Description

The `game` table chess games.  The `game_id` holds the unique game number, and each of the columns ending with `_id` reference the primary index of each of the child tables:

 - `white_player_id` and `black_player_id` reference `player.player_id`.
 - `event_id` references `event.event_id`.
 - `site_id` references `site.site_id`.
 - `annotator_id` references `annotator.annotator_id`.

There will be gaps in the `game_id` sequence if games have been deleted from the database.

The `result` column contains the following values:

 - 0: Unfinished.
 - 1: White Win.
 - 2: Black Win. 
 - 3: Draw.

The `date` column holds a day-month-year using the following formula:

    date = (year * 10000) + (month * 100) + day

Any element of the date can be missing (value 0).

The `partial` column holds the starting position of the game.  This will be `NULL` for a game starting from the standard starting position. See the *Position Encoding* section for details of the encoding.

The `moves` column holds the game moves.  See *Moves Encoding* section for details of the encoding.

The `annotations` column holds the move annotations and is closely related to the `moves` column.  Both columns should be modified together in order to maintain integrity.  See *Annotation Encoding* section for details of the encoding.

### player

    CREATE TABLE player (
       player_id INTEGER PRIMARY KEY AUTOINCREMENT,
       last_name TEXT,
       first_names TEXT,
       country_code TEXT)
       
#### player indexes

    CREATE UNIQUE INDEX player_index ON player (player_id)
    CREATE INDEX player_last_name_index ON player (last_name)

### event

    CREATE TABLE event (
       event_id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT)
       
#### event indexes

    CREATE UNIQUE INDEX event_index ON event (event_id)
    CREATE INDEX event_name_index ON event (name)
    
### site

    CREATE TABLE site (
       site_id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT)
       
#### site indexes

    CREATE UNIQUE INDEX site_index ON site (site_id)
    CREATE INDEX site_name_index ON site (name)
    
### annotator

    CREATE TABLE annotator (
       annotator_id INTEGER PRIMARY KEY AUTOINCREMENT,
       name TEXT)
       
#### annotator indexes

    CREATE UNIQUE INDEX annotator_index ON annotator (annotator_id)
    CREATE INDEX annotator_name_index ON annotator (name)   
### optree

    CREATE TABLE optree (
       pos UNSIGNED BIG INT,
       move INTEGER,
       score TINYINT,
       last_move TINYINT,
       game_id INTEGER)
       
#### optree indexes

    CREATE INDEX optree_pos_index ON optree (pos)
    
### Position Encoding

Positions are encoded within `BLOB` objects using the following format:

    position: 32 bytes (4-bits per piece). 
    to-move: 1-bit (1=white, 0=black).
    castling-rights: 4-bits (0x8=wks, 0x4=wqs, 0x2=bks, 0x1=bqs).
    ep-file: 4-bits (0=none, 1=a-file ... 8=h-file).
    halfmove-clock: 16-bits (little endian).
    fullmove-number: 16-bits (little endian).

    Total size: 297-bits.

The `position` starts with index 0 at A1 and each piece is encoded as follows:

The lower 3 bits are the piece type

 - 0: Empty.
 - 1: Pawn.
 - 2: Rook.
 - 3: Knight.
 - 4: Bishop
 - 5: Queen.
 - 6: King.

With the upper bit in the nibble defining the piece colour:

 - 0x0: White.
 - 0x8: Black.

The `halfmove-clock` is the number of half moves since a pawn was moved or a piece was captured.  This is used to help calculate if the game is drawn by the "50-move rule".

### TimeControl Encoding

Timecontrols are encoded within `BLOB` objects using the following format:

    num periods: 4-bits.
    period #1 type: 4-bits.
    period #1 moves: 8-bits.
    period #1 time (secs): 16-bits
    period #1 increment (secs): 4-bits.
    ...
    period #n type: 4-bits.
    period #n moves: 8-bits.
    period #n time (secs): 16-bits
    period #n increment (secs): 4-bits.
    
    Total size: (32-bits * num periods) + 4-bits.

Time Control Period Types:

 - 0: None (invalid)
 - 1: Rollover
 - 2: Game In
 - 3: Moves In

### Moves Encoding

### Annotation Encoding

### Improving Insert Performance
