cmake_minimum_required(VERSION 3.2)
project(ChessCore)

add_subdirectory(test/gtest)

ADD_DEFINITIONS(-D_M_IX86)

ADD_DEFINITIONS(-DDEBUG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SQLITE_FILES 3rdparty/sqlite3/sqlite3.c)
file(GLOB_RECURSE LIB_SOURCE_FILES src/*.cpp)
file(GLOB_RECURSE CCORE_SOURCE_FILES ccore/*.cpp)

include_directories(include)
include_directories(3rdparty/sqlite3)

add_executable(ChessCore ${SQLITE_FILES} ${LIB_SOURCE_FILES} ${CCORE_SOURCE_FILES})

file(GLOB_RECURSE UNITTEST_SOURCE_FILES test/unittests/*.cpp)

enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(runUnitTests ${SQLITE_FILES} ${LIB_SOURCE_FILES} ${UNITTEST_SOURCE_FILES})
target_link_libraries(runUnitTests gtest gtest_main)
add_test( runUnitTests runUnitTests )
