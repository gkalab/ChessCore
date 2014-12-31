
SRCDIR := .
BUILDDIR := ../../build
BINDIR := ../../bin
GTESTDIR := ../gtest
include ../../make.conf

$(BUILDDIR)/%.o: $(GTESTDIR)/src/%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

TARGET := $(BINDIR)/unittests
SRCS := $(GTESTDIR)/src/gtest-all.cc unittest_main.cpp $(wildcard *_unittest.cpp)
OBJS=$(addprefix $(BUILDDIR)/,$(addsuffix .o, $(basename $(notdir ${SRCS}))))

CXXFLAGS += -I$(GTESTDIR)/include -I$(GTESTDIR) -I../../include -DGTEST_USE_OWN_TR1_TUPLE=1
LIBS += -lChessCore

ifneq ($(COMPILER),clang)
	LDFLAGS += -pthread
endif

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
	cp -f ../../tools/shellwrapper $(TARGET)
endif

clean:
	rm -f $(TARGET) $(TARGET).bin $(OBJS)
	rm -f ccore.log *.orig *.bak

depend:
	makedepend -Y -p$(BUILDDIR)/ -- $(CXXFLAGS) $(SRCS)

format:
	astyle $(SRCS)


# DO NOT DELETE

../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-death-test.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-message.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-param-test.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-printers.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest_prod.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-test-part.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest.cc
../../build/../gtest/src/gtest-all.o: ../gtest/include/gtest/gtest-spi.h
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-internal-inl.h
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-death-test.cc
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-filepath.cc
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-port.cc
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-printers.cc
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-test-part.cc
../../build/../gtest/src/gtest-all.o: ../gtest/src/gtest-typed-test.cc
../../build/unittest_main.o: ../../include/ChessCore/Log.h
../../build/unittest_main.o: ../../include/ChessCore/ChessCore.h
../../build/unittest_main.o: ../../include/ChessCore/Types.h
../../build/unittest_main.o: ../../include/ChessCore/Mutex.h
../../build/unittest_main.o: ../../include/ChessCore/Lowlevel.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-death-test.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-message.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-param-test.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-printers.h
../../build/unittest_main.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest_prod.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-test-part.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/unittest_main.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Bitstream_unittest.o: ../../include/ChessCore/Blob.h
../../build/Bitstream_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Bitstream_unittest.o: ../../include/ChessCore/Types.h
../../build/Bitstream_unittest.o: ../../include/ChessCore/Bitstream.h
../../build/Bitstream_unittest.o: ../../include/ChessCore/Rand64.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Bitstream_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Game_unittest.o: ../../include/ChessCore/Game.h
../../build/Game_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Game_unittest.o: ../../include/ChessCore/Types.h
../../build/Game_unittest.o: ../../include/ChessCore/Mutex.h
../../build/Game_unittest.o: ../../include/ChessCore/GameHeader.h
../../build/Game_unittest.o: ../../include/ChessCore/Player.h
../../build/Game_unittest.o: ../../include/ChessCore/TimeControl.h
../../build/Game_unittest.o: ../../include/ChessCore/Blob.h
../../build/Game_unittest.o: ../../include/ChessCore/Position.h
../../build/Game_unittest.o: ../../include/ChessCore/Move.h
../../build/Game_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/Game_unittest.o: ../../include/ChessCore/Util.h
../../build/Game_unittest.o: ../../include/ChessCore/Data.h
../../build/Game_unittest.o: ../../include/ChessCore/AnnotMove.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Game_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Game_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Lowlevel_unittest.o: ../../include/ChessCore/Util.h
../../build/Lowlevel_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Lowlevel_unittest.o: ../../include/ChessCore/Types.h
../../build/Lowlevel_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/Lowlevel_unittest.o: ../../include/ChessCore/Data.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Lowlevel_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Move_unittest.o: ../../include/ChessCore/Position.h
../../build/Move_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Move_unittest.o: ../../include/ChessCore/Types.h
../../build/Move_unittest.o: ../../include/ChessCore/Move.h
../../build/Move_unittest.o: ../../include/ChessCore/Blob.h
../../build/Move_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/Move_unittest.o: ../../include/ChessCore/Util.h
../../build/Move_unittest.o: ../../include/ChessCore/Data.h
../../build/Move_unittest.o: ../../include/ChessCore/Log.h
../../build/Move_unittest.o: ../../include/ChessCore/Mutex.h
../../build/Move_unittest.o: ../../include/ChessCore/Rand64.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Move_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Move_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Position.h
../../build/Movegen_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Types.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Move.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Blob.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Util.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Data.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Epd.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Log.h
../../build/Movegen_unittest.o: ../../include/ChessCore/Mutex.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Movegen_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/PackUtil_unittest.o: ../../include/ChessCore/Util.h
../../build/PackUtil_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/PackUtil_unittest.o: ../../include/ChessCore/Types.h
../../build/PackUtil_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/PackUtil_unittest.o: ../../include/ChessCore/Data.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/PackUtil_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/TimeControl_unittest.o: ../../include/ChessCore/TimeControl.h
../../build/TimeControl_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/TimeControl_unittest.o: ../../include/ChessCore/Types.h
../../build/TimeControl_unittest.o: ../../include/ChessCore/Blob.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/TimeControl_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
../../build/Util_unittest.o: ../../include/ChessCore/Util.h
../../build/Util_unittest.o: ../../include/ChessCore/ChessCore.h
../../build/Util_unittest.o: ../../include/ChessCore/Types.h
../../build/Util_unittest.o: ../../include/ChessCore/Lowlevel.h
../../build/Util_unittest.o: ../../include/ChessCore/Data.h
../../build/Util_unittest.o: ../../include/ChessCore/Blob.h
../../build/Util_unittest.o: ../../include/ChessCore/Log.h
../../build/Util_unittest.o: ../../include/ChessCore/Mutex.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-internal.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-port.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-tuple.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-string.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-filepath.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-type-util.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-death-test.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-death-test-internal.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-message.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-param-test.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-param-util.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-linked_ptr.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-printers.h
../../build/Util_unittest.o: ../gtest/include/gtest/internal/gtest-param-util-generated.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest_prod.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-test-part.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest-typed-test.h
../../build/Util_unittest.o: ../gtest/include/gtest/gtest_pred_impl.h
