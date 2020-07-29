CXXFLAGS += -I./include -I./libs/anserial/include -std=c++17 -Wall \
            -O3 -march=native -lzmq -pthread $(BUDGIE_DEFINES) \
            -MD -g

SDL2_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf

intree-libs = libs/anserial/build/lib/anserial.a

#OBJ = src/main.o src/mcts.o src/game.o src/gtp.o src/pattern_db.o
bot-src = $(wildcard src/bot/*.cpp)
bot-obj = $(bot-src:.cpp=.o)
bot-dep = $(bot-src:.cpp=.d)
bot-main = src/bot-main.o

sdl-ui-src =
sdl-ui-obj = $(sdl-ui-src:.cpp=.o)
sdl-ui-main = src/sdl-main.o

bin/thing: dir-structure $(intree-libs) $(bot-obj) $(bot-main)
	$(CXX) $(CXXFLAGS) -o $@ $(bot-obj) $(bot-main) $(intree-libs)

bin/sdl-thing: dir-structure $(intree-libs) $(bot-obj) $(sdl-ui-main)
	$(CXX) $(CXXFLAGS) $(SDL2_FLAGS) -o $@ $(bot-obj) $(intree-libs) $(sdl-ui-main)

libs/anserial/build/lib/anserial.a:
	cd libs/anserial; make libs

all: bin/thing bin/sdl-thing

-include $(bot-dep)

.PHONY: dir-structure
dir-structure:
	mkdir -p ./bin

.PHONY: clean
clean:
	-rm -f $(bot-obj) $(bot-main) $(sdl-ui-obj) $(sdl-ui-main)
	-cd libs/anserial; make clean
	-rm -rf ./bin
