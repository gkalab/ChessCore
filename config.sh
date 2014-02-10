#!/bin/sh
#
# Set-up make.conf for cpu, build type, library type and compiler.
#
# The following environment variables will be carried over from the environment:
#
# $CFLAGS, $CXXFLAGS, $ASFLAGS, $LDFLAGS, $LIBS.
#
# So to enable Electric Fence, which was installed via Macports, you would do:
#
#  LDFLAGS="-L /opt/local/lib" LIBS="-lefence" ./config.sh x64 dynamic clang
#

dir=$(dirname $0)
progname=$(basename $0)
outfile=$dir/make.conf
platform=$(uname -s)

if [ $# -ne 3 ]; then
    echo "usage: $progname cpu libtype compiler"
    echo "  cpu: x64, x86, arm or armcross"
    echo "  libtype: static or shared (or dynamic)"
    echo "  compiler: gcc, clang, intel or analyzer"
    exit 1
fi

cpu=$1
libtype=$2
compiler=$3

# Validate CPU
case $cpu in
x64)
    size=64
    ;;
x86)
    size=32
    ;;
arm)
    unset size
    ;;
armcross)
    unset size
    crosscomp=arm-linux-gnueabi-
    cpu=arm
    ;;
*)
    echo $progname: Unsupported CPU $cpu
    exit 3
    ;;
esac

# Validate libtype
case $libtype in
static|Static)
    libtype=Static
    ;;
shared|Shared|dynamic|Dynamic)
    libtype=Shared
    ;;
*)
    echo $progname: Unknown libtype $libtype
    exit 5
    ;;
esac

if [ $platform = Linux -o $platform = FreeBSD ]; then
    cxxstd="-std=c++0x"
    cxxstdlib=""
    if [ $libtype = Static ]; then
        libsuffix="a"
    else
        libsuffix="so"
    fi
elif [ $platform = Darwin ]; then
    cxxstd="-std=c++0x"
    if [ $compiler = clang ]; then
        cxxstdlib="-stdlib=libc++"
    fi
    if [ $libtype = Static ]; then
        libsuffix="a"
    else
        libsuffix="dylib"
    fi
fi

# Validate compiler
case $compiler in
gcc)
    CC=${crosscomp}gcc
    CXX=${crosscomp}g++
    AR=${crosscomp}ar
    CFLAGS="$CFLAGS -Wall -Wno-unused-but-set-variable -fvisibility=hidden"
    CXXFLAGS="$CXXFLAGS $cxxstd -Wall -Wno-unused-but-set-variable -fvisibility=hidden"
    LDFLAGS="$LDFLAGS $rpath"
    if [ ! -z "$size" ]; then
        CFLAGS="$CFLAGS -m$size"
        CXXFLAGS="$CXXFLAGS -m$size"
        ASFLAGS="$ASFLAGS -m$size"
        LDFLAGS="$LDFLAGS -m$size"
    fi

    if [ "$cpu" = "arm" ]; then
        # Get rid of warning about va_list and assume we are compiling for ARM
        CFLAGS="$CFLAGS -mcpu=arm1176jzf-s -Wno-psabi"
        CXXFLAGS="$CXXFLAGS -mcpu=arm1176jzf-s -Wno-psabi"
    fi
        
    RELEASE_CFLAGS="-O3"
    RELEASE_CXXFLAGS="$RELEASE_CFLAGS"
    RELEASE_ASFLAGS=""
    RELEASE_LDFLAGS=""
    DEBUG_CFLAGS="-ggdb"
    DEBUG_CXXFLAGS="$DEBUG_CFLAGS"
    DEBUG_ASFLAGS="-ggdb"
    DEBUG_LDFLAGS="-ggdb"
    PROFILE_CFLAGS="$RELEASE_CFLAGS $DEBUG_CFLAGS -pg"
    PROFILE_CXXFLAGS="$PROFILE_CFLAGS"
    PROFILE_ASFLAGS="-ggdb"
    PROFILE_LDFLAGS="-ggdb -pg"
    ;;
clang)    # Assumes Apple LLVM 5.0 (-Ofast and -flto)
    if [ $platform = FreeBSD ]; then
        CC=/usr/local/bin/clang
        CXX=/usr/local/bin/clang++
    else
        CC=clang
        CXX=clang++
    fi
    AR=ar
    CFLAGS="$CFLAGS -Wall -Wno-unused-variable -fvisibility=hidden"
    CXXFLAGS="$CXXFLAGS $cxxstd $cxxstdlib -Wall -Wno-unused-variable -fvisibility=hidden"
    LDFLAGS="$LDFLAGS $rpath $cxxstdlib"

    if [ ! -z "$size" ]; then
        CFLAGS="$CFLAGS -m$size"
        CXXFLAGS="$CXXFLAGS -m$size"
        ASFLAGS="$ASFLAGS -m$size"
        LDFLAGS="$LDFLAGS -m$size"
    fi

    if [ $platform = Darwin ]; then
        CFLAGS="$CFLAGS -mmacosx-version-min=10.7"
        CXXFLAGS="$CXXFLAGS -mmacosx-version-min=10.7"
        LDFLAGS="$LDFLAGS -mmacosx-version-min=10.7"
    fi

    RELEASE_CFLAGS="-Ofast -flto"
    RELEASE_CXXFLAGS="$RELEASE_CFLAGS"
    RELEASE_ASFLAGS=""
    RELEASE_LDFLAGS="-flto"
    DEBUG_CFLAGS="-O0 -g"
    DEBUG_CXXFLAGS="$DEBUG_CFLAGS"
    DEBUG_ASFLAGS="-g"
    DEBUG_LDFLAGS="-g"
    PROFILE_CFLAGS="$RELEASE_CFLAGS $DEBUG_CFLAGS"
    PROFILE_CXXFLAGS="$PROFILE_CFLAGS"
    PROFILE_ASFLAGS="-g"
    PROFILE_LDFLAGS="-g"
    ;;
intel)
    CC=icc
    CXX=icpc
    AR=xiar
    CFLAGS="$CFLAGS -Wall"
    CXXFLAGS="$CXXFLAGS -Wall"
    RELEASE_CFLAGS="-O3"
    RELEASE_CXXFLAGS="$RELEASE_CFLAGS"
    RELEASE_ASFLAGS=""
    RELEASE_LDFLAGS=""
    DEBUG_CFLAGS="-ggdb"
    DEBUG_CXXFLAGS="$DEBUG_CFLAGS"
    DEBUG_ASFLAGS="-ggdb"
    DEBUG_LDFLAGS="-ggdb"
    PROFILE_CFLAGS="$RELEASE_CFLAGS $DEBUG_CFLAGS"
    PROFILE_CXXFLAGS="$PROFILE_CFLAGS"
    PROFILE_ASFLAGS="-ggdb"
    PROFILE_LDFLAGS="-ggdb"
    ;;
analyzer)
    CC=/usr/local/bin/scan-build/ccc-analyzer
    CXX=/usr/local/bin/scan-build/c++-analyzer
    AR=ar
    CFLAGS="$CFLAGS -Wall"
    CXXFLAGS="$CXXFLAGS $cxxstd $cxxstdlib -Wall"
    LDFLAGS="$LDFLAGS $rpath $cxxstdlib"
    if [ ! -z "$size" ]; then
        CFLAGS="$CFLAGS -m$size"
        CXXFLAGS="$CXXFLAGS -m$size"
        ASFLAGS="$ASFLAGS -m$size"
        LDFLAGS="$LDFLAGS -m$size"
    fi
    RELEASE_CFLAGS="-Ofast -flto"
    RELEASE_CXXFLAGS="$RELEASE_CFLAGS"
    RELEASE_ASFLAGS=""
    RELEASE_LDFLAGS="-flto"
    DEBUG_CFLAGS="-O0 -g"
    DEBUG_CXXFLAGS="$DEBUG_CFLAGS"
    DEBUG_ASFLAGS="-g"
    DEBUG_LDFLAGS="-g"
    ;;
*)
    echo $progname: Unsupported compiler $compiler
    exit 6
    ;;
esac

# Under FreeBSD, lots of packages are installed in /usr/local
if [ $platform = FreeBSD ]; then
	CFLAGS="$CFLAGS -I/usr/local/include"
	CXXFLAGS="$CXXFLAGS -I /usr/local/include"
	LDFLAGS="$LDFLAGS -L /usr/local/lib"
fi

cat > $outfile << EOF
#
# Configured for $cpu/$libtype/$compiler using $progname
# DO NOT EDIT!
#
PLATFORM := $platform
COMPILER := $compiler
CC := $CC
CXX := $CXX
LD := $CXX
AR := $AR
BUILDCPU := $cpu
BUILDSIZE := $size
LIBTYPE := $libtype
LIBSUFFIX := $libsuffix
CFLAGS := $CFLAGS
CXXFLAGS := $CXXFLAGS
MMFLAGS := $CXXFLAGS
ASFLAGS := $ASFLAGS
LDFLAGS := $LDFLAGS
LIBS := $LIBS

ifeq (\$(BUILDTYPE),Debug)
    CFLAGS += $DEBUG_CFLAGS -DDEBUG
    CXXFLAGS += $DEBUG_CXXFLAGS -DDEBUG
    ASFLAGS += $DEBUG_ASFLAGS -DDEBUG
    LDFLAGS += $DEBUG_LDFLAGS
else ifeq (\$(BUILDTYPE),Release)
    CFLAGS += $RELEASE_CFLAGS
    CXXFLAGS += $RELEASE_CXXFLAGS
    ASFLAGS += $RELEASE_ASFLAGS
    LDFLAGS += $RELEASE_LDFLAGS
else ifeq (\$(BUILDTYPE),Profile)
    CFLAGS += $PROFILE_CFLAGS -DPROFILE
    CXXFLAGS += $PROFILE_CXXFLAGS -DPROFILE
    ASFLAGS += $PROFILE_ASFLAGS -DPROFILE
    LDFLAGS += $PROFILE_LDFLAGS
endif

# On Linux/FreeBSD etc, tools/shellwrapper must be used to set \$LD_LIBRARY_PATH
# in order to find libChessCore.so.  Under OSX this isn't necessary.
ifeq (\$(PLATFORM),Darwin)
    USE_SHELL_WRAPPER=0
else
    USE_SHELL_WRAPPER=1
endif

\$(BUILDDIR)/%.o: \$(SRCDIR)/%.c
	\$(CC) \$(CFLAGS) -c -o \$@ \$<
\$(BUILDDIR)/%.o: \$(SRCDIR)/%.cpp
	\$(CXX) \$(CXXFLAGS) -c -o \$@ \$<
\$(BUILDDIR)/%.o: \$(SRCDIR)/%.mm
	\$(CC) \$(MMFLAGS) -c -o \$@ \$<
\$(BUILDDIR)/%.o: \$(SRCDIR)/%.S
	\$(CC) \$(ASFLAGS) -c -o \$@ \$<

EOF


