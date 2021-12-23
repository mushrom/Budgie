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
# optional module for debug viewer
pkg_check_modules(SDL2     IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)

set(LIBS PkgConfig::libzmq)

# TODO: either replace anserial, or add cmakelists to anserial
include_directories("libs/anserial/include")
set(ANSERIAL_SRC
	libs/anserial/src/parser.cpp
	libs/anserial/src/anserial.cpp
	libs/anserial/src/s_tree.cpp
)

set(BOT_LOGIC_SRC
	${ANSERIAL_SRC}
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

set(BUDGIE_MAIN     src/bot-main.cpp)
set(SDL_MAIN        src/sdl-main.cpp)
set(JOSEKI_GEN_MAIN src/josekigen.cpp)

add_library(BudgieLogic ${BOT_LOGIC_SRC})
target_link_libraries(BudgieLogic ${LIBS})
if (NOT WINDOWS)
	target_link_options(BudgieLogic PUBLIC "-pthread")
endif()

add_executable(budgie    ${BUDGIE_MAIN})
add_executable(josekigen ${JOSEKI_GEN_MAIN})
target_link_libraries(budgie    BudgieLogic)
target_link_libraries(josekigen BudgieLogic)

if (SDL2_FOUND AND SDL2_ttf_FOUND)
	add_executable(bot-visualize ${SDL_MAIN})
	target_link_libraries(bot-visualize BudgieLogic
	                      PkgConfig::SDL2
	                      PkgConfig::SDL2_ttf)
endif()
