CXXFLAGS += -g -I./include -std=c++17 -Wall -O3 -march=native
#OBJ = src/main.o src/mcts.o src/game.o src/gtp.o src/pattern_db.o
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

thing: $(OBJ)
	echo $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

.PHONY: clean
clean:
	-rm -f src/*.o thing
