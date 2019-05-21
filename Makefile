CXXFLAGS += -g -I./include -std=c++17 -Wall -O3 -march=native
#OBJ = src/main.o src/mcts.o src/game.o src/gtp.o src/pattern_db.o
bot-src = $(wildcard src/bot/*.cpp)
bot-obj = $(bot-src:.cpp=.o)
bot-main = src/bot-main.o

sdl-ui-src =
sdl-ui-obj = $(sdl-ui-src:.cpp=.o)
sdl-ui-main = src/sdl-main.o

thing: $(bot-obj) $(bot-main)
	$(CXX) $(CXXFLAGS) -o $@ $(bot-obj) $(bot-main)

.PHONY: clean
clean:
	-rm -f $(bot-obj) $(bot-main) $(sdl-ui-obj) $(sdl-ui-main) thing
