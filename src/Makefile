
SRCDIR := .
BUILDDIR := ../build
BINDIR := ../bin
include ../make.conf

CFLAGS += -pthread -I../include
CXXFLAGS += -pthread -I../include

ifeq ($(LIBTYPE),Shared)
    CFLAGS += -fPIC 
    CXXFLAGS += -fPIC
    ASFLAGS += -fPIC
    LIBS += -lsqlite3
    ifeq ($(PLATFORM),FreeBSD)
        LIBS += -lexecinfo
    endif
    TARGET := $(BINDIR)/libChessCore.${LIBSUFFIX}
else
    TARGET := $(BUILDDIR)/libChessCore.${LIBSUFFIX}
endif

SRCS := AnnotMove.cpp AsmX86.cpp Bitstream.cpp Blob.cpp CbhDatabase.cpp \
	CfdbDatabase.cpp ChessCore.cpp Data.cpp Database.cpp Engine.cpp \
	EngineMessage.cpp EngineMessageQueue.cpp Epd.cpp GameHeader.cpp Game.cpp \
	IndexManager.cpp IoEvent.cpp IoEventWaiter.cpp Log.cpp Lowlevel.cpp Move.cpp \
	Mutex.cpp OpeningTree.cpp PgnDatabase.cpp PgnScanner.cpp Player.cpp \
	Position.cpp PositionHash.cpp Process.cpp ProgOption.cpp Rand64.cpp \
    SqliteStatement.cpp Thread.cpp TimeControl.cpp UCIEngineOption.cpp \
    Util.cpp Version.cpp

ifeq ($(PLATFORM),Darwin)
	SRCS += AppleUtil.mm
	MMFLAGS += -I../include
	LIBS += -framework Foundation
endif

OBJS=$(addprefix $(BUILDDIR)/,$(addsuffix .o, $(basename ${SRCS})))

VERSION=$(shell grep version Version.ver | cut -f2 -d' ')
MAJOR_VERSION=$(shell echo $(VERSION) | cut -f1 -d'.')
MAJOR_MINOR_VERSION=$(shell echo $(VERSION) | cut -f1,2 -d'.')

all: Version.cpp $(BUILDDIR) $(BINDIR) $(TARGET)

PgnScanner.cpp: PgnScanner.l
	../tools/flex.py $< $@ 

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Build a static library
$(BUILDDIR)/libChessCore.a: $(OBJS)
	$(AR) cr $@ $(OBJS)

# Build the generic .so file (Linux/FreeBSD)
$(BINDIR)/libChessCore.so: $(BINDIR)/libChessCore.so.$(VERSION)
	../tools/create_so_links.sh $(BINDIR)/libChessCore.so.$(VERSION)
	
# Build the versioned .so file (Linux/FreeBSD)
$(BINDIR)/libChessCore.so.$(MAJOR_VERSION): $(OBJS)
	$(CXX) -shared -Wl,-soname,libChessCore.so.$(MAJOR_VERSION) \
		$(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Build the .dylib file (Mac OS X)
$(BINDIR)/libChessCore.dylib: $(OBJS)
	$(CXX) -dynamiclib -current_version $(MAJOR_MINOR_VERSION) \
		-compatibility_version $(MAJOR_MINOR_VERSION) \
		-install_name @executable_path/libChessCore.dylib \
		$(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	
Version.cpp:
	../tools/upver.py Version.ver Version.cpp ../include/ChessCore/Version.h

FORCE:

clean:
	rm -f $(BINDIR)/libChessCore.* $(OBJS)
	rm -f *.bak

depend:
	makedepend -Y -p$(BUILDDIR)/ -- $(CXXFLAGS) $(SRCS)

format:
	astyle $(SRCS)

print-%:
	@echo $* = $($*)

# DO NOT DELETE

../build/AnnotMove.o: ../include/ChessCore/AnnotMove.h
../build/AnnotMove.o: ../include/ChessCore/ChessCore.h
../build/AnnotMove.o: ../include/ChessCore/Types.h
../build/AnnotMove.o: ../include/ChessCore/Move.h
../build/AnnotMove.o: ../include/ChessCore/Position.h
../build/AnnotMove.o: ../include/ChessCore/Blob.h
../build/AnnotMove.o: ../include/ChessCore/Lowlevel.h
../build/AnnotMove.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/AnnotMove.o: ../include/ChessCore/Log.h ../include/ChessCore/Mutex.h
../build/Bitstream.o: ../include/ChessCore/Bitstream.h
../build/Bitstream.o: ../include/ChessCore/ChessCore.h
../build/Bitstream.o: ../include/ChessCore/Types.h
../build/Bitstream.o: ../include/ChessCore/Blob.h ../include/ChessCore/Log.h
../build/Bitstream.o: ../include/ChessCore/Mutex.h
../build/Blob.o: ../include/ChessCore/Blob.h ../include/ChessCore/ChessCore.h
../build/Blob.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Blob.o: ../include/ChessCore/Lowlevel.h ../include/ChessCore/Data.h
../build/Blob.o: ../include/ChessCore/Log.h ../include/ChessCore/Mutex.h
../build/CbhDatabase.o: ../include/ChessCore/CbhDatabase.h
../build/CbhDatabase.o: ../include/ChessCore/ChessCore.h
../build/CbhDatabase.o: ../include/ChessCore/Types.h
../build/CbhDatabase.o: ../include/ChessCore/Database.h
../build/CbhDatabase.o: ../include/ChessCore/Game.h
../build/CbhDatabase.o: ../include/ChessCore/Mutex.h
../build/CbhDatabase.o: ../include/ChessCore/GameHeader.h
../build/CbhDatabase.o: ../include/ChessCore/Player.h
../build/CbhDatabase.o: ../include/ChessCore/TimeControl.h
../build/CbhDatabase.o: ../include/ChessCore/Blob.h
../build/CbhDatabase.o: ../include/ChessCore/Position.h
../build/CbhDatabase.o: ../include/ChessCore/Move.h
../build/CbhDatabase.o: ../include/ChessCore/Lowlevel.h
../build/CbhDatabase.o: ../include/ChessCore/Util.h
../build/CbhDatabase.o: ../include/ChessCore/Data.h
../build/CbhDatabase.o: ../include/ChessCore/AnnotMove.h
../build/CbhDatabase.o: ../include/ChessCore/Log.h
../build/CfdbDatabase.o: ../include/ChessCore/CfdbDatabase.h
../build/CfdbDatabase.o: ../include/ChessCore/ChessCore.h
../build/CfdbDatabase.o: ../include/ChessCore/Types.h
../build/CfdbDatabase.o: ../include/ChessCore/Database.h
../build/CfdbDatabase.o: ../include/ChessCore/Game.h
../build/CfdbDatabase.o: ../include/ChessCore/Mutex.h
../build/CfdbDatabase.o: ../include/ChessCore/GameHeader.h
../build/CfdbDatabase.o: ../include/ChessCore/Player.h
../build/CfdbDatabase.o: ../include/ChessCore/TimeControl.h
../build/CfdbDatabase.o: ../include/ChessCore/Blob.h
../build/CfdbDatabase.o: ../include/ChessCore/Position.h
../build/CfdbDatabase.o: ../include/ChessCore/Move.h
../build/CfdbDatabase.o: ../include/ChessCore/Lowlevel.h
../build/CfdbDatabase.o: ../include/ChessCore/Util.h
../build/CfdbDatabase.o: ../include/ChessCore/Data.h
../build/CfdbDatabase.o: ../include/ChessCore/AnnotMove.h
../build/CfdbDatabase.o: ../include/ChessCore/Bitstream.h
../build/CfdbDatabase.o: ../include/ChessCore/SqliteStatement.h
../build/CfdbDatabase.o: ../include/ChessCore/OpeningTree.h
../build/CfdbDatabase.o: ../include/ChessCore/Log.h
../build/ChessCore.o: ../include/ChessCore/ChessCore.h
../build/ChessCore.o: ../include/ChessCore/Types.h
../build/ChessCore.o: ../include/ChessCore/Lowlevel.h
../build/ChessCore.o: ../include/ChessCore/Rand64.h
../build/ChessCore.o: ../include/ChessCore/Data.h ../include/ChessCore/Log.h
../build/ChessCore.o: ../include/ChessCore/Mutex.h
../build/ChessCore.o: ../include/ChessCore/Util.h
../build/Data.o: ../include/ChessCore/Data.h ../include/ChessCore/ChessCore.h
../build/Data.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Data.o: ../include/ChessCore/Lowlevel.h ../include/ChessCore/Log.h
../build/Data.o: ../include/ChessCore/Mutex.h
../build/Database.o: ../include/ChessCore/Database.h
../build/Database.o: ../include/ChessCore/ChessCore.h
../build/Database.o: ../include/ChessCore/Types.h ../include/ChessCore/Game.h
../build/Database.o: ../include/ChessCore/Mutex.h
../build/Database.o: ../include/ChessCore/GameHeader.h
../build/Database.o: ../include/ChessCore/Player.h
../build/Database.o: ../include/ChessCore/TimeControl.h
../build/Database.o: ../include/ChessCore/Blob.h
../build/Database.o: ../include/ChessCore/Position.h
../build/Database.o: ../include/ChessCore/Move.h
../build/Database.o: ../include/ChessCore/Lowlevel.h
../build/Database.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Database.o: ../include/ChessCore/AnnotMove.h
../build/Database.o: ../include/ChessCore/Log.h
../build/Engine.o: ../include/ChessCore/Engine.h
../build/Engine.o: ../include/ChessCore/ChessCore.h
../build/Engine.o: ../include/ChessCore/Types.h ../include/ChessCore/Thread.h
../build/Engine.o: ../include/ChessCore/Process.h ../include/ChessCore/Move.h
../build/Engine.o: ../include/ChessCore/TimeControl.h
../build/Engine.o: ../include/ChessCore/Blob.h
../build/Engine.o: ../include/ChessCore/EngineMessage.h
../build/Engine.o: ../include/ChessCore/Position.h
../build/Engine.o: ../include/ChessCore/Lowlevel.h
../build/Engine.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Engine.o: ../include/ChessCore/UCIEngineOption.h
../build/Engine.o: ../include/ChessCore/EngineMessageQueue.h
../build/Engine.o: ../include/ChessCore/Mutex.h
../build/Engine.o: ../include/ChessCore/IoEventWaiter.h
../build/Engine.o: ../include/ChessCore/IoEvent.h ../include/ChessCore/Log.h
../build/EngineMessage.o: ../include/ChessCore/EngineMessage.h
../build/EngineMessage.o: ../include/ChessCore/ChessCore.h
../build/EngineMessage.o: ../include/ChessCore/Types.h
../build/EngineMessage.o: ../include/ChessCore/Move.h
../build/EngineMessage.o: ../include/ChessCore/Position.h
../build/EngineMessage.o: ../include/ChessCore/Blob.h
../build/EngineMessage.o: ../include/ChessCore/Lowlevel.h
../build/EngineMessage.o: ../include/ChessCore/Util.h
../build/EngineMessage.o: ../include/ChessCore/Data.h
../build/EngineMessage.o: ../include/ChessCore/UCIEngineOption.h
../build/EngineMessageQueue.o: ../include/ChessCore/EngineMessageQueue.h
../build/EngineMessageQueue.o: ../include/ChessCore/ChessCore.h
../build/EngineMessageQueue.o: ../include/ChessCore/Types.h
../build/EngineMessageQueue.o: ../include/ChessCore/Mutex.h
../build/EngineMessageQueue.o: ../include/ChessCore/EngineMessage.h
../build/EngineMessageQueue.o: ../include/ChessCore/Move.h
../build/EngineMessageQueue.o: ../include/ChessCore/Position.h
../build/EngineMessageQueue.o: ../include/ChessCore/Blob.h
../build/EngineMessageQueue.o: ../include/ChessCore/Lowlevel.h
../build/EngineMessageQueue.o: ../include/ChessCore/Util.h
../build/EngineMessageQueue.o: ../include/ChessCore/Data.h
../build/EngineMessageQueue.o: ../include/ChessCore/UCIEngineOption.h
../build/EngineMessageQueue.o: ../include/ChessCore/IoEventWaiter.h
../build/EngineMessageQueue.o: ../include/ChessCore/IoEvent.h
../build/EngineMessageQueue.o: ../include/ChessCore/Log.h
../build/Epd.o: ../include/ChessCore/Epd.h ../include/ChessCore/ChessCore.h
../build/Epd.o: ../include/ChessCore/Types.h ../include/ChessCore/Position.h
../build/Epd.o: ../include/ChessCore/Move.h ../include/ChessCore/Blob.h
../build/Epd.o: ../include/ChessCore/Lowlevel.h ../include/ChessCore/Util.h
../build/Epd.o: ../include/ChessCore/Data.h ../include/ChessCore/Log.h
../build/Epd.o: ../include/ChessCore/Mutex.h
../build/GameHeader.o: ../include/ChessCore/GameHeader.h
../build/GameHeader.o: ../include/ChessCore/ChessCore.h
../build/GameHeader.o: ../include/ChessCore/Types.h
../build/GameHeader.o: ../include/ChessCore/Player.h
../build/GameHeader.o: ../include/ChessCore/TimeControl.h
../build/GameHeader.o: ../include/ChessCore/Blob.h
../build/GameHeader.o: ../include/ChessCore/Util.h
../build/GameHeader.o: ../include/ChessCore/Lowlevel.h
../build/GameHeader.o: ../include/ChessCore/Data.h
../build/Game.o: ../include/ChessCore/Game.h ../include/ChessCore/ChessCore.h
../build/Game.o: ../include/ChessCore/Types.h ../include/ChessCore/Mutex.h
../build/Game.o: ../include/ChessCore/GameHeader.h
../build/Game.o: ../include/ChessCore/Player.h
../build/Game.o: ../include/ChessCore/TimeControl.h
../build/Game.o: ../include/ChessCore/Blob.h ../include/ChessCore/Position.h
../build/Game.o: ../include/ChessCore/Move.h ../include/ChessCore/Lowlevel.h
../build/Game.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Game.o: ../include/ChessCore/AnnotMove.h
../build/Game.o: ../include/ChessCore/PgnDatabase.h
../build/Game.o: ../include/ChessCore/PgnScanner.h
../build/Game.o: ../include/ChessCore/Database.h
../build/Game.o: ../include/ChessCore/IndexManager.h
../build/Game.o: ../include/ChessCore/Log.h
../build/IndexManager.o: ../include/ChessCore/IndexManager.h
../build/IndexManager.o: ../include/ChessCore/ChessCore.h
../build/IndexManager.o: ../include/ChessCore/Types.h
../build/IndexManager.o: ../include/ChessCore/Blob.h
../build/IndexManager.o: ../include/ChessCore/Util.h
../build/IndexManager.o: ../include/ChessCore/Lowlevel.h
../build/IndexManager.o: ../include/ChessCore/Data.h
../build/IndexManager.o: ../include/ChessCore/Log.h
../build/IndexManager.o: ../include/ChessCore/Mutex.h
../build/IoEvent.o: ../include/ChessCore/IoEvent.h
../build/IoEvent.o: ../include/ChessCore/ChessCore.h
../build/IoEvent.o: ../include/ChessCore/Types.h ../include/ChessCore/Mutex.h
../build/IoEvent.o: ../include/ChessCore/Util.h
../build/IoEvent.o: ../include/ChessCore/Lowlevel.h
../build/IoEvent.o: ../include/ChessCore/Data.h ../include/ChessCore/Log.h
../build/IoEventWaiter.o: ../include/ChessCore/IoEventWaiter.h
../build/IoEventWaiter.o: ../include/ChessCore/ChessCore.h
../build/IoEventWaiter.o: ../include/ChessCore/Types.h
../build/IoEventWaiter.o: ../include/ChessCore/IoEvent.h
../build/IoEventWaiter.o: ../include/ChessCore/Mutex.h
../build/IoEventWaiter.o: ../include/ChessCore/Util.h
../build/IoEventWaiter.o: ../include/ChessCore/Lowlevel.h
../build/IoEventWaiter.o: ../include/ChessCore/Data.h
../build/IoEventWaiter.o: ../include/ChessCore/Log.h
../build/Log.o: ../include/ChessCore/Log.h ../include/ChessCore/ChessCore.h
../build/Log.o: ../include/ChessCore/Types.h ../include/ChessCore/Mutex.h
../build/Log.o: ../include/ChessCore/Util.h ../include/ChessCore/Lowlevel.h
../build/Log.o: ../include/ChessCore/Data.h ../include/ChessCore/Thread.h
../build/Log.o: ../include/ChessCore/Blob.h
../build/Lowlevel.o: ../include/ChessCore/Lowlevel.h
../build/Lowlevel.o: ../include/ChessCore/ChessCore.h
../build/Lowlevel.o: ../include/ChessCore/Types.h
../build/Move.o: ../include/ChessCore/Move.h ../include/ChessCore/ChessCore.h
../build/Move.o: ../include/ChessCore/Types.h ../include/ChessCore/Position.h
../build/Move.o: ../include/ChessCore/Blob.h ../include/ChessCore/Lowlevel.h
../build/Move.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Move.o: ../include/ChessCore/Log.h ../include/ChessCore/Mutex.h
../build/Mutex.o: ../include/ChessCore/Mutex.h
../build/Mutex.o: ../include/ChessCore/ChessCore.h
../build/Mutex.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Mutex.o: ../include/ChessCore/Lowlevel.h ../include/ChessCore/Data.h
../build/Mutex.o: ../include/ChessCore/Log.h
../build/OpeningTree.o: ../include/ChessCore/OpeningTree.h
../build/OpeningTree.o: ../include/ChessCore/ChessCore.h
../build/OpeningTree.o: ../include/ChessCore/Types.h
../build/OpeningTree.o: ../include/ChessCore/Move.h
../build/OpeningTree.o: ../include/ChessCore/Database.h
../build/OpeningTree.o: ../include/ChessCore/Game.h
../build/OpeningTree.o: ../include/ChessCore/Mutex.h
../build/OpeningTree.o: ../include/ChessCore/GameHeader.h
../build/OpeningTree.o: ../include/ChessCore/Player.h
../build/OpeningTree.o: ../include/ChessCore/TimeControl.h
../build/OpeningTree.o: ../include/ChessCore/Blob.h
../build/OpeningTree.o: ../include/ChessCore/Position.h
../build/OpeningTree.o: ../include/ChessCore/Lowlevel.h
../build/OpeningTree.o: ../include/ChessCore/Util.h
../build/OpeningTree.o: ../include/ChessCore/Data.h
../build/OpeningTree.o: ../include/ChessCore/AnnotMove.h
../build/OpeningTree.o: ../include/ChessCore/Log.h
../build/PgnDatabase.o: ../include/ChessCore/PgnDatabase.h
../build/PgnDatabase.o: ../include/ChessCore/ChessCore.h
../build/PgnDatabase.o: ../include/ChessCore/Types.h
../build/PgnDatabase.o: ../include/ChessCore/PgnScanner.h
../build/PgnDatabase.o: ../include/ChessCore/Database.h
../build/PgnDatabase.o: ../include/ChessCore/Game.h
../build/PgnDatabase.o: ../include/ChessCore/Mutex.h
../build/PgnDatabase.o: ../include/ChessCore/GameHeader.h
../build/PgnDatabase.o: ../include/ChessCore/Player.h
../build/PgnDatabase.o: ../include/ChessCore/TimeControl.h
../build/PgnDatabase.o: ../include/ChessCore/Blob.h
../build/PgnDatabase.o: ../include/ChessCore/Position.h
../build/PgnDatabase.o: ../include/ChessCore/Move.h
../build/PgnDatabase.o: ../include/ChessCore/Lowlevel.h
../build/PgnDatabase.o: ../include/ChessCore/Util.h
../build/PgnDatabase.o: ../include/ChessCore/Data.h
../build/PgnDatabase.o: ../include/ChessCore/AnnotMove.h
../build/PgnDatabase.o: ../include/ChessCore/IndexManager.h
../build/PgnDatabase.o: ../include/ChessCore/Log.h
../build/PgnScanner.o: ../include/ChessCore/PgnScanner.h
../build/PgnScanner.o: ../include/ChessCore/ChessCore.h
../build/PgnScanner.o: ../include/ChessCore/Types.h
../build/PgnScanner.o: ../include/ChessCore/Log.h
../build/PgnScanner.o: ../include/ChessCore/Mutex.h
../build/Player.o: ../include/ChessCore/Player.h
../build/Player.o: ../include/ChessCore/ChessCore.h
../build/Player.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Player.o: ../include/ChessCore/Lowlevel.h
../build/Player.o: ../include/ChessCore/Data.h
../build/Position.o: ../include/ChessCore/Position.h
../build/Position.o: ../include/ChessCore/ChessCore.h
../build/Position.o: ../include/ChessCore/Types.h ../include/ChessCore/Move.h
../build/Position.o: ../include/ChessCore/Blob.h
../build/Position.o: ../include/ChessCore/Lowlevel.h
../build/Position.o: ../include/ChessCore/Util.h ../include/ChessCore/Data.h
../build/Position.o: ../include/ChessCore/Bitstream.h
../build/Position.o: ../include/ChessCore/Rand64.h ../include/ChessCore/Log.h
../build/Position.o: ../include/ChessCore/Mutex.h
../build/PositionHash.o: ../include/ChessCore/Position.h
../build/PositionHash.o: ../include/ChessCore/ChessCore.h
../build/PositionHash.o: ../include/ChessCore/Types.h
../build/PositionHash.o: ../include/ChessCore/Move.h
../build/PositionHash.o: ../include/ChessCore/Blob.h
../build/PositionHash.o: ../include/ChessCore/Lowlevel.h
../build/PositionHash.o: ../include/ChessCore/Util.h
../build/PositionHash.o: ../include/ChessCore/Data.h
../build/Process.o: ../include/ChessCore/Process.h
../build/Process.o: ../include/ChessCore/ChessCore.h
../build/Process.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Process.o: ../include/ChessCore/Lowlevel.h
../build/Process.o: ../include/ChessCore/Data.h ../include/ChessCore/Log.h
../build/Process.o: ../include/ChessCore/Mutex.h
../build/ProgOption.o: ../include/ChessCore/ProgOption.h
../build/ProgOption.o: ../include/ChessCore/ChessCore.h
../build/ProgOption.o: ../include/ChessCore/Types.h
../build/ProgOption.o: ../include/ChessCore/Util.h
../build/ProgOption.o: ../include/ChessCore/Lowlevel.h
../build/ProgOption.o: ../include/ChessCore/Data.h ../include/ChessCore/Log.h
../build/ProgOption.o: ../include/ChessCore/Mutex.h
../build/Rand64.o: ../include/ChessCore/Rand64.h
../build/Rand64.o: ../include/ChessCore/ChessCore.h
../build/Rand64.o: ../include/ChessCore/Types.h ../include/ChessCore/Util.h
../build/Rand64.o: ../include/ChessCore/Lowlevel.h
../build/Rand64.o: ../include/ChessCore/Data.h
../build/SqliteStatement.o: ../include/ChessCore/SqliteStatement.h
../build/SqliteStatement.o: ../include/ChessCore/ChessCore.h
../build/SqliteStatement.o: ../include/ChessCore/Types.h
../build/SqliteStatement.o: ../include/ChessCore/Blob.h
../build/SqliteStatement.o: ../include/ChessCore/CfdbDatabase.h
../build/SqliteStatement.o: ../include/ChessCore/Database.h
../build/SqliteStatement.o: ../include/ChessCore/Game.h
../build/SqliteStatement.o: ../include/ChessCore/Mutex.h
../build/SqliteStatement.o: ../include/ChessCore/GameHeader.h
../build/SqliteStatement.o: ../include/ChessCore/Player.h
../build/SqliteStatement.o: ../include/ChessCore/TimeControl.h
../build/SqliteStatement.o: ../include/ChessCore/Position.h
../build/SqliteStatement.o: ../include/ChessCore/Move.h
../build/SqliteStatement.o: ../include/ChessCore/Lowlevel.h
../build/SqliteStatement.o: ../include/ChessCore/Util.h
../build/SqliteStatement.o: ../include/ChessCore/Data.h
../build/SqliteStatement.o: ../include/ChessCore/AnnotMove.h
../build/SqliteStatement.o: ../include/ChessCore/Bitstream.h
../build/Thread.o: ../include/ChessCore/Thread.h
../build/Thread.o: ../include/ChessCore/ChessCore.h
../build/Thread.o: ../include/ChessCore/Types.h ../include/ChessCore/Log.h
../build/Thread.o: ../include/ChessCore/Mutex.h
../build/TimeControl.o: ../include/ChessCore/TimeControl.h
../build/TimeControl.o: ../include/ChessCore/ChessCore.h
../build/TimeControl.o: ../include/ChessCore/Types.h
../build/TimeControl.o: ../include/ChessCore/Blob.h
../build/TimeControl.o: ../include/ChessCore/Bitstream.h
../build/TimeControl.o: ../include/ChessCore/Log.h
../build/TimeControl.o: ../include/ChessCore/Mutex.h
../build/TimeControl.o: ../include/ChessCore/Util.h
../build/TimeControl.o: ../include/ChessCore/Lowlevel.h
../build/TimeControl.o: ../include/ChessCore/Data.h
../build/UCIEngineOption.o: ../include/ChessCore/UCIEngineOption.h
../build/UCIEngineOption.o: ../include/ChessCore/ChessCore.h
../build/UCIEngineOption.o: ../include/ChessCore/Types.h
../build/UCIEngineOption.o: ../include/ChessCore/Move.h
../build/UCIEngineOption.o: ../include/ChessCore/Log.h
../build/UCIEngineOption.o: ../include/ChessCore/Mutex.h
../build/UCIEngineOption.o: ../include/ChessCore/Util.h
../build/UCIEngineOption.o: ../include/ChessCore/Lowlevel.h
../build/UCIEngineOption.o: ../include/ChessCore/Data.h
../build/Util.o: ../include/ChessCore/Util.h ../include/ChessCore/ChessCore.h
../build/Util.o: ../include/ChessCore/Types.h ../include/ChessCore/Lowlevel.h
../build/Util.o: ../include/ChessCore/Data.h ../include/ChessCore/Blob.h
../build/Util.o: ../include/ChessCore/Log.h ../include/ChessCore/Mutex.h
../build/Version.o: ../include/ChessCore/Version.h
../build/Version.o: ../include/ChessCore/ChessCore.h
../build/Version.o: ../include/ChessCore/Types.h
../build/AppleUtil.o: ../include/ChessCore/ChessCore.h
../build/AppleUtil.o: ../include/ChessCore/Types.h
../build/AppleUtil.o: ../include/ChessCore/AppleUtil.h
../build/AppleUtil.o: ../include/ChessCore/Log.h ../include/ChessCore/Mutex.h
