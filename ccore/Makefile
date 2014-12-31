
SRCDIR := .
BUILDDIR := ../build
BINDIR := ../bin
include ../make.conf

TARGET := $(BINDIR)/ccore

SRCS := AnalyzeGames.cpp Config.cpp ccore.cpp Functions.cpp PlayGames.cpp ProcessEpd.cpp

OBJS=$(addprefix $(BUILDDIR)/,$(addsuffix .o, $(basename ${SRCS})))

CXXFLAGS += -I../include
LIBS += -lChessCore
LDFLAGS += -pthread

ifeq ($(LIBTYPE),Static)
    LDFLAGS += -L$(BUILDDIR)
	LIBS += -lsqlite3
ifeq ($(PLATFORM),Darwin)
    LIBS += -framework Foundation
endif
else ifeq ($(LIBTYPE),Shared)
    LDFLAGS += -L$(BINDIR)
endif

ifeq ($(PLATFORM),Linux)
	LIBS += -lssl
else ifeq ($(PLATFORM),FreeBSD)
    LIBS += -lexecinfo -lssl
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)
ifeq ($(USE_SHELL_WRAPPER),1)
	mv -f $(TARGET) $(TARGET).bin
	cp -f ../tools/shellwrapper $(TARGET)
endif

clean:
	rm -f $(TARGET) $(TARGET).bin $(OBJS)
	rm -f ccore.log *.orig *.bak

depend:
	makedepend -Y -p$(BUILDDIR)/ -- $(CXXFLAGS) $(SRCS)

format:
	astyle $(SRCS)

# DO NOT DELETE

../build/AnalyzeGames.o: ccore.h ../include/ChessCore/ChessCore.h
../build/AnalyzeGames.o: ../include/ChessCore/Types.h
../build/AnalyzeGames.o: ../include/ChessCore/IoEvent.h
../build/AnalyzeGames.o: ../include/ChessCore/Mutex.h
../build/AnalyzeGames.o: ../include/ChessCore/TimeControl.h
../build/AnalyzeGames.o: ../include/ChessCore/Blob.h Config.h
../build/AnalyzeGames.o: ../include/ChessCore/Engine.h
../build/AnalyzeGames.o: ../include/ChessCore/Thread.h
../build/AnalyzeGames.o: ../include/ChessCore/Process.h
../build/AnalyzeGames.o: ../include/ChessCore/Move.h
../build/AnalyzeGames.o: ../include/ChessCore/TimeControl.h
../build/AnalyzeGames.o: ../include/ChessCore/EngineMessage.h
../build/AnalyzeGames.o: ../include/ChessCore/Position.h
../build/AnalyzeGames.o: ../include/ChessCore/Lowlevel.h
../build/AnalyzeGames.o: ../include/ChessCore/Util.h
../build/AnalyzeGames.o: ../include/ChessCore/Data.h
../build/AnalyzeGames.o: ../include/ChessCore/UCIEngineOption.h
../build/AnalyzeGames.o: ../include/ChessCore/EngineMessageQueue.h
../build/AnalyzeGames.o: ../include/ChessCore/IoEventWaiter.h
../build/AnalyzeGames.o: ../include/ChessCore/Database.h
../build/AnalyzeGames.o: ../include/ChessCore/Game.h
../build/AnalyzeGames.o: ../include/ChessCore/GameHeader.h
../build/AnalyzeGames.o: ../include/ChessCore/Player.h
../build/AnalyzeGames.o: ../include/ChessCore/AnnotMove.h
../build/AnalyzeGames.o: ../include/ChessCore/PgnDatabase.h
../build/AnalyzeGames.o: ../include/ChessCore/PgnScanner.h
../build/AnalyzeGames.o: ../include/ChessCore/IndexManager.h
../build/AnalyzeGames.o: ../include/ChessCore/Log.h
../build/Config.o: ccore.h ../include/ChessCore/ChessCore.h
../build/Config.o: ../include/ChessCore/Types.h
../build/Config.o: ../include/ChessCore/IoEvent.h
../build/Config.o: ../include/ChessCore/Mutex.h
../build/Config.o: ../include/ChessCore/TimeControl.h
../build/Config.o: ../include/ChessCore/Blob.h Config.h
../build/Config.o: ../include/ChessCore/Engine.h
../build/Config.o: ../include/ChessCore/Thread.h
../build/Config.o: ../include/ChessCore/Process.h ../include/ChessCore/Move.h
../build/Config.o: ../include/ChessCore/TimeControl.h
../build/Config.o: ../include/ChessCore/EngineMessage.h
../build/Config.o: ../include/ChessCore/Position.h
../build/Config.o: ../include/ChessCore/Lowlevel.h
../build/Config.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Config.o: ../include/ChessCore/UCIEngineOption.h
../build/Config.o: ../include/ChessCore/EngineMessageQueue.h
../build/Config.o: ../include/ChessCore/IoEventWaiter.h
../build/Config.o: ../include/ChessCore/Log.h
../build/ccore.o: ccore.h ../include/ChessCore/ChessCore.h
../build/ccore.o: ../include/ChessCore/Types.h ../include/ChessCore/IoEvent.h
../build/ccore.o: ../include/ChessCore/Mutex.h
../build/ccore.o: ../include/ChessCore/TimeControl.h
../build/ccore.o: ../include/ChessCore/Blob.h Config.h
../build/ccore.o: ../include/ChessCore/Engine.h ../include/ChessCore/Thread.h
../build/ccore.o: ../include/ChessCore/Process.h ../include/ChessCore/Move.h
../build/ccore.o: ../include/ChessCore/TimeControl.h
../build/ccore.o: ../include/ChessCore/EngineMessage.h
../build/ccore.o: ../include/ChessCore/Position.h
../build/ccore.o: ../include/ChessCore/Lowlevel.h ../include/ChessCore/Util.h
../build/ccore.o: ../include/ChessCore/Data.h
../build/ccore.o: ../include/ChessCore/UCIEngineOption.h
../build/ccore.o: ../include/ChessCore/EngineMessageQueue.h
../build/ccore.o: ../include/ChessCore/IoEventWaiter.h
../build/ccore.o: ../include/ChessCore/ProgOption.h
../build/ccore.o: ../include/ChessCore/Game.h
../build/ccore.o: ../include/ChessCore/GameHeader.h
../build/ccore.o: ../include/ChessCore/Player.h
../build/ccore.o: ../include/ChessCore/AnnotMove.h ../include/ChessCore/Log.h
../build/ccore.o: ../include/ChessCore/Database.h
../build/ccore.o: ../include/ChessCore/Version.h
../build/Functions.o: ccore.h ../include/ChessCore/ChessCore.h
../build/Functions.o: ../include/ChessCore/Types.h
../build/Functions.o: ../include/ChessCore/IoEvent.h
../build/Functions.o: ../include/ChessCore/Mutex.h
../build/Functions.o: ../include/ChessCore/TimeControl.h
../build/Functions.o: ../include/ChessCore/Blob.h Config.h
../build/Functions.o: ../include/ChessCore/Engine.h
../build/Functions.o: ../include/ChessCore/Thread.h
../build/Functions.o: ../include/ChessCore/Process.h
../build/Functions.o: ../include/ChessCore/Move.h
../build/Functions.o: ../include/ChessCore/TimeControl.h
../build/Functions.o: ../include/ChessCore/EngineMessage.h
../build/Functions.o: ../include/ChessCore/Position.h
../build/Functions.o: ../include/ChessCore/Lowlevel.h
../build/Functions.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Functions.o: ../include/ChessCore/UCIEngineOption.h
../build/Functions.o: ../include/ChessCore/EngineMessageQueue.h
../build/Functions.o: ../include/ChessCore/IoEventWaiter.h
../build/Functions.o: ../include/ChessCore/Database.h
../build/Functions.o: ../include/ChessCore/Game.h
../build/Functions.o: ../include/ChessCore/GameHeader.h
../build/Functions.o: ../include/ChessCore/Player.h
../build/Functions.o: ../include/ChessCore/AnnotMove.h
../build/Functions.o: ../include/ChessCore/PgnDatabase.h
../build/Functions.o: ../include/ChessCore/PgnScanner.h
../build/Functions.o: ../include/ChessCore/IndexManager.h
../build/Functions.o: ../include/ChessCore/OpeningTree.h
../build/Functions.o: ../include/ChessCore/Epd.h ../include/ChessCore/Log.h
../build/Functions.o: ../include/ChessCore/Rand64.h
../build/PlayGames.o: ccore.h ../include/ChessCore/ChessCore.h
../build/PlayGames.o: ../include/ChessCore/Types.h
../build/PlayGames.o: ../include/ChessCore/IoEvent.h
../build/PlayGames.o: ../include/ChessCore/Mutex.h
../build/PlayGames.o: ../include/ChessCore/TimeControl.h
../build/PlayGames.o: ../include/ChessCore/Blob.h Config.h
../build/PlayGames.o: ../include/ChessCore/Engine.h
../build/PlayGames.o: ../include/ChessCore/Thread.h
../build/PlayGames.o: ../include/ChessCore/Process.h
../build/PlayGames.o: ../include/ChessCore/Move.h
../build/PlayGames.o: ../include/ChessCore/TimeControl.h
../build/PlayGames.o: ../include/ChessCore/EngineMessage.h
../build/PlayGames.o: ../include/ChessCore/Position.h
../build/PlayGames.o: ../include/ChessCore/Lowlevel.h
../build/PlayGames.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/PlayGames.o: ../include/ChessCore/UCIEngineOption.h
../build/PlayGames.o: ../include/ChessCore/EngineMessageQueue.h
../build/PlayGames.o: ../include/ChessCore/IoEventWaiter.h
../build/PlayGames.o: ../include/ChessCore/Database.h
../build/PlayGames.o: ../include/ChessCore/Game.h
../build/PlayGames.o: ../include/ChessCore/GameHeader.h
../build/PlayGames.o: ../include/ChessCore/Player.h
../build/PlayGames.o: ../include/ChessCore/AnnotMove.h
../build/PlayGames.o: ../include/ChessCore/PgnDatabase.h
../build/PlayGames.o: ../include/ChessCore/PgnScanner.h
../build/PlayGames.o: ../include/ChessCore/IndexManager.h
../build/PlayGames.o: ../include/ChessCore/Log.h
../build/PlayGames.o: ../include/ChessCore/OpeningTree.h
../build/ProcessEpd.o: ccore.h ../include/ChessCore/ChessCore.h
../build/ProcessEpd.o: ../include/ChessCore/Types.h
../build/ProcessEpd.o: ../include/ChessCore/IoEvent.h
../build/ProcessEpd.o: ../include/ChessCore/Mutex.h
../build/ProcessEpd.o: ../include/ChessCore/TimeControl.h
../build/ProcessEpd.o: ../include/ChessCore/Blob.h Config.h
../build/ProcessEpd.o: ../include/ChessCore/Engine.h
../build/ProcessEpd.o: ../include/ChessCore/Thread.h
../build/ProcessEpd.o: ../include/ChessCore/Process.h
../build/ProcessEpd.o: ../include/ChessCore/Move.h
../build/ProcessEpd.o: ../include/ChessCore/TimeControl.h
../build/ProcessEpd.o: ../include/ChessCore/EngineMessage.h
../build/ProcessEpd.o: ../include/ChessCore/Position.h
../build/ProcessEpd.o: ../include/ChessCore/Lowlevel.h
../build/ProcessEpd.o: ../include/ChessCore/Util.h
../build/ProcessEpd.o: ../include/ChessCore/Data.h
../build/ProcessEpd.o: ../include/ChessCore/UCIEngineOption.h
../build/ProcessEpd.o: ../include/ChessCore/EngineMessageQueue.h
../build/ProcessEpd.o: ../include/ChessCore/IoEventWaiter.h
../build/ProcessEpd.o: ../include/ChessCore/Log.h ../include/ChessCore/Epd.h
../build/ProcessEpd.o: ../include/ChessCore/Database.h
../build/ProcessEpd.o: ../include/ChessCore/Game.h
../build/ProcessEpd.o: ../include/ChessCore/GameHeader.h
../build/ProcessEpd.o: ../include/ChessCore/Player.h
../build/ProcessEpd.o: ../include/ChessCore/AnnotMove.h
