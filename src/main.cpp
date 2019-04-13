#include <mcts-gb/mcts.hpp>
#include <mcts-gb/gtp.hpp>
#include <stdio.h>

int main(int argc, char *argv[]) {
	/*
	mcts_thing::board foo(9);
	mcts_thing::mcts asdf;
	mcts_thing::mcts_node *move = asdf.root;

	move->explore(mcts_thing::coordinate(5, 5), &foo);

	while (true) {
		mcts_thing::coordinate coord = move->best_move();

		if (!foo.is_valid_coordinate(coord)) {
			break;
		}

		foo.make_move(coord);
		foo.print();

		printf("%u: (%u, %u)\n", foo.current_player, coord.first, coord.second);

		move = &move->leaves[coord];
	}
	*/
	mcts_thing::gtp_client gtp;

	gtp.repl();
	
	return 0;
}
