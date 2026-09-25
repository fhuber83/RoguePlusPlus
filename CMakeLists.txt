CMAKE_MINIMUM_REQUIRED(VERSION 4.00)

PROJECT(RoguePlusPlus LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

OPTION(ROGUE_BUILD_TESTS "Build the unit tests (fetches GoogleTest)" ON)

SET(CURSES_NEED_WIDE true)

FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(Curses REQUIRED)

SET(ROGUE_WARNINGS -Wall -Wextra -Wpedantic)

# nlohmann/json for the score file (and later saved games); uses an installed
# copy if there is one
INCLUDE(FetchContent)
FETCHCONTENT_DECLARE(nlohmann_json
	URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
	DOWNLOAD_EXTRACT_TIMESTAMP ON
	SYSTEM
	FIND_PACKAGE_ARGS 3.12 NAMES nlohmann_json)
FETCHCONTENT_MAKEAVAILABLE(nlohmann_json)

# Game library: everything except the program entry point
FILE(GLOB_RECURSE ROGUE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
LIST(FILTER ROGUE_SOURCES EXCLUDE REGEX "/src/app/")

ADD_LIBRARY(rogue_game STATIC ${ROGUE_SOURCES})
# src/ is a quote-only include path, so project headers never shadow system
# headers such as <curses.h> (included by ui/curses/CursesTerminal.cpp).
TARGET_INCLUDE_DIRECTORIES(rogue_game PUBLIC ${CURSES_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS(rogue_game PUBLIC "-iquote${CMAKE_CURRENT_SOURCE_DIR}/src")
TARGET_COMPILE_DEFINITIONS(rogue_game PUBLIC MINROG ROGUE_CHARSET=3)
TARGET_COMPILE_OPTIONS(rogue_game PRIVATE ${ROGUE_WARNINGS})
TARGET_LINK_LIBRARIES(rogue_game PUBLIC ${CURSES_LIBRARIES})
TARGET_LINK_LIBRARIES(rogue_game PRIVATE nlohmann_json::nlohmann_json)

ADD_EXECUTABLE(rogue++ src/app/main.cpp)
TARGET_COMPILE_OPTIONS(rogue++ PRIVATE ${ROGUE_WARNINGS})
TARGET_LINK_LIBRARIES(rogue++ PRIVATE rogue_game)

IF(ROGUE_BUILD_TESTS)
	ENABLE_TESTING()
	FETCHCONTENT_DECLARE(googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
		DOWNLOAD_EXTRACT_TIMESTAMP ON
		FIND_PACKAGE_ARGS NAMES GTest)
	SET(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	FETCHCONTENT_MAKEAVAILABLE(googletest)

	FILE(GLOB_RECURSE ROGUE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
	ADD_EXECUTABLE(rogue_tests ${ROGUE_TEST_SOURCES})
	TARGET_COMPILE_OPTIONS(rogue_tests PRIVATE ${ROGUE_WARNINGS})
	TARGET_LINK_LIBRARIES(rogue_tests PRIVATE rogue_game GTest::gtest_main)

	INCLUDE(GoogleTest)
	GTEST_DISCOVER_TESTS(rogue_tests)
ENDIF()
