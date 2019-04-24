CXXFLAGS += -g -I./include -std=c++17 -Wall -O0 -march=native
OBJ = src/main.o src/mcts.o src/game.o src/gtp.o src/pattern_db.o

thing: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	-rm -f src/*.o thing
