#include <budgie/budgie.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
// TODO: only included for 'split_string()', should split that in a util file
#include <budgie/gtp.hpp>
#include <iostream>

namespace mcts_thing {

budgie::budgie(args_parser::option_map& opts) {
	// cache options
	options = opts;

	// initlaize tree state
	tree = init_mcts(opts);

	// initialize game state
	komi = stoi(opts["komi"]);
	boardsize = stoi(opts["boardsize"]);
	playouts = stoi(opts["playouts"]);
	game.reset(boardsize, komi);
}

bool budgie::make_move(move m) {
	switch (m.type) {
		case budgie::move::types::Pass:
			passed = true;
			break;

		case budgie::move::types::Move:
			passed = false;
			game.current_player = m.player;
			return game.make_move(m.coord);
			break;

		default:
			break;
	}

	return true;
}

budgie::move budgie::genmove(void) {
	// TODO: passing heuristics
	if (passed) {
		return move(budgie::move::types::Pass);
	}

	// XXX: we should get rid of reset here and preserve nodes...
	tree->reset();
	coordinate coord = tree->do_search(&game, playouts);

	// TODO: make resign threshold configurable
	if (tree->win_rate(coord) < 0.15) {
		return move(budgie::move::types::Resign);
	}

	else if (!game.is_valid_move(coord)
	         || game.is_suicide(coord, game.current_player))
	{
		return move(budgie::move::types::Pass);
	}

	else {
		return move(budgie::move::types::Move,
		            coord,
		            game.current_player);
	}
}

void budgie::reset(void) {
	tree->reset();
	game.reset(boardsize, komi);
	passed = false;
}

void budgie::set_player(point::color p) {
	// TODO: reset tree if the current player isn't the same as the 
	//       root node
	game.current_player = p;
}

// XXX
std::unique_ptr<mcts> budgie::init_mcts(args_parser::option_map& options) {
	// TODO: we should have an AI instance class that handles
	//       all of the initialization,, board/mcts interaction (make_move, ...)
	pattern_dbptr db = pattern_dbptr(new pattern_db(options["patterns"]));

	// only have this tree policy right now
	tree_policy *tree_pol;
	(options["tree_policy"] == "mcts")
		? (tree_policy *)(new mcts_tree_policy(db))
		: (tree_policy *)(new uct_rave_tree_policy(db));

	if (options["tree_policy"] == "mcts") {
		tree_pol = new mcts_tree_policy(db);
	} else if (options["tree_policy"] == "uct") {
		tree_pol = new uct_tree_policy(db);
	} else {
		tree_pol = new uct_rave_tree_policy(db);
	}

	// TODO: parse options from command line
	std::list<playout_strategy*> strats = {
		new capture_weighted_playout(db),
		new random_playout(db),
	};

	auto policies = mcts_thing::split_string(options["playout_policy"]);

	for (auto policy : policies) {
		if (policy == "local_weighted") {
			strats.push_back(new local_weighted_playout(db));

		} else if (policy == "capture_enemy_ataris") {
			strats.push_back(new capture_weighted_playout(db));

		} else if (policy == "random"){
			strats.push_back(new random_playout(db));

		} else {
			std::cerr << __func__ << ": unknown playout policy \""
				<< policy << "\"" << std::endl;
		}
	}

	if (options["mode"] == "distributed-gtp") {
		return std::unique_ptr<mcts>(new distributed_mcts(tree_pol, strats));

	} else {
		return std::unique_ptr<mcts>(new mcts(tree_pol, strats));
	}
}

// namespace mcts_thing
}
