CXXFLAGS += -g -I./include -std=c++17 -Wall -O3 -march=native
SDL2_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf

#OBJ = src/main.o src/mcts.o src/game.o src/gtp.o src/pattern_db.o
bot-src = $(wildcard src/bot/*.cpp)
bot-obj = $(bot-src:.cpp=.o)
bot-main = src/bot-main.o

sdl-ui-src =
sdl-ui-obj = $(sdl-ui-src:.cpp=.o)
sdl-ui-main = src/sdl-main.o

bin/thing: dir-structure $(bot-obj) $(bot-main)
	$(CXX) $(CXXFLAGS) -o $@ $(bot-obj) $(bot-main)

bin/sdl-thing: dir-structure $(bot-obj) $(sdl-ui-main)
	$(CXX) $(CXXFLAGS) $(SDL2_FLAGS) -o $@ $(bot-obj) $(sdl-ui-main)

all: bin/thing bin/sdl-thing

.PHONY: dir-structure
dir-structure:
	mkdir -p ./bin

.PHONY: clean
clean:
	-rm -f $(bot-obj) $(bot-main) $(sdl-ui-obj) $(sdl-ui-main)
	-rm -rf ./bin
