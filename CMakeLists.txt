cmake_minimum_required(VERSION 3.13)
project(Budgie VERSION 0.5)

include_directories("include")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_CXX_FLAGS "-Wall -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "-Wall")

if (NOT CMAKE_BUILD_TYPE)
	message(WARNING "CMAKE_BUILD_TYPE not set, defaulting to Release")
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(libzmq REQUIRED IMPORTED_TARGET libzmq)
pkg_check_modules(capnp  REQUIRED IMPORTED_TARGET capnp)

# optional module for debug viewer
pkg_check_modules(SDL2     IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)

set(LIBS PkgConfig::libzmq)

set(SCHEMAS
	schemas/Node.capnp
)

set(BOT_LOGIC_SRC
	src/bot/args_parser.cpp
	src/bot/distributed_client.cpp
	src/bot/capture_weighted_playout.cpp
	src/bot/distributed_mcts.cpp
	src/bot/gtp.cpp
	src/bot/local-weighted-playouts.cpp
	src/bot/random-playouts.cpp
	src/bot/budgie.cpp
	src/bot/pattern_db.cpp
	src/bot/adjacent-playouts.cpp
	src/bot/attack_opponent_groups_playout.cpp
	src/bot/save_atari_playout.cpp
	src/bot/game.cpp
	src/bot/mcts-tree-policy.cpp
	src/bot/mcts.cpp
	src/bot/uct-tree-policy.cpp
	src/bot/uct-rave-tree-policy.cpp
	src/util/sgf.cpp
)

include_directories(${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/schemas)

foreach(_schema ${SCHEMAS})
	set(_schema_out ${PROJECT_BINARY_DIR}/${_schema})
	set(_schema_in  ${PROJECT_SOURCE_DIR}/${_schema})

	list(APPEND BOT_LOGIC_SRC ${_schema_out}.c++)
	add_custom_command(
		OUTPUT ${_schema_out}.c++ ${_schema_out}.h
		COMMAND capnp compile -oc++:${PROJECT_BINARY_DIR}/schemas
		                      --src-prefix=${PROJECT_SOURCE_DIR}/schemas
		                      ${_schema_in}
		DEPENDS ${_schema_in}
	)
endforeach()

set(BUDGIE_MAIN      src/bot-main.cpp)
set(SDL_MAIN         src/sdl-main.cpp)
set(JOSEKI_GEN_MAIN  src/josekigen.cpp)
set(PATTERN_GEN_MAIN src/patterngen.cpp)

add_library(BudgieLogic ${BOT_LOGIC_SRC})
target_link_libraries(BudgieLogic ${LIBS})
if (NOT WINDOWS)
	target_link_options(BudgieLogic PUBLIC "-pthread")
endif()

add_executable(budgie     ${BUDGIE_MAIN})
add_executable(josekigen  ${JOSEKI_GEN_MAIN})
add_executable(patterngen ${PATTERN_GEN_MAIN})

target_link_libraries(budgie     BudgieLogic)
target_link_libraries(josekigen  BudgieLogic)
target_link_libraries(patterngen BudgieLogic)

if (SDL2_FOUND AND SDL2_ttf_FOUND)
	add_executable(bot-visualize ${SDL_MAIN})
	target_link_libraries(bot-visualize BudgieLogic
	                      PkgConfig::SDL2
	                      PkgConfig::SDL2_ttf)
endif()
