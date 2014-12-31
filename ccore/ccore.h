//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// ccore.h: Top-level definitions and include files for 'ccore'.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/IoEvent.h>
#include <ChessCore/TimeControl.h>
#include "Config.h"

//
// in ccore.cpp
//
extern std::string g_progName;
extern std::string g_optCfgFile;
extern int g_optDepth;
extern bool g_optDebugLog;
extern std::string g_optDotDir;
extern std::string g_optEpdFile;
extern std::string g_optEcoFile;
extern std::string g_optFen;
extern bool g_optHelp;
extern std::string g_optInputDb;
extern uint64_t g_optKey;
extern std::string g_optLogFile;
extern bool g_optLogComms;
extern int g_optNumber1;
extern bool g_optNumber1Ind;
extern int g_optNumber2;
extern bool g_optNumber2Ind;
extern std::string g_optOutputDb;
extern bool g_optQuiet;
extern bool g_optRelaxed;
extern ChessCore::TimeControl g_optTimeControl;
extern std::string g_optTimeStr;
extern bool g_optVersion;

extern bool g_quitFlag;
extern ChessCore::IoEvent g_quitEvent;

extern void setQuit();

//
// in playGames.cpp
//
extern bool playGames(const std::string &engineId1, const std::string &engineId2);

//
// in analyzeGames.cpp
//
extern bool analyzeGames(const std::string &engineId);

//
// in processEpd.cpp
//
extern bool processEpd(const std::string &engineId);

//
// in Functions.cpp
//
extern bool funcRandom(bool cstyle);
extern bool funcRandomPositions();
extern bool funcMakeEpd();
extern bool funcValidateDb();
extern bool funcCopyDb();
extern bool funcBuildOpeningTree();
extern bool funcClassify();
extern bool funcPgnIndex();
extern bool funcSearchDb();
extern bool funcPerftdiv();
extern bool funcRecursivePosDump();
extern bool funcFindBuggyPos();
extern bool funcTestPopCnt();
