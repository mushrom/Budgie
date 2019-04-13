CXXFLAGS += -g -I./include -std=c++17 -Wall
OBJ = src/main.o src/mcts.o src/game.o src/gtp.o

thing: $(OBJ)
	$(CXX) -o $@ $(OBJ)

.PHONY: clean
clean:
	-rm -f src/*.o thing
