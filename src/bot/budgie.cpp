#include <budgie/budgie.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
// TODO: only included for 'split_string()', should split that in a util file
#include <budgie/gtp.hpp>
#include <iostream>

namespace mcts_thing {

// XXX XXX: global variable used by mcts_node::fully_visited
// TODO: what would be a cleaner way to do this?
unsigned full_traversals = 8;

budgie::budgie(args_parser::option_map& opts) {
	// cache options
	options = opts;

	// initlaize tree state
	tree = init_mcts(opts);

	// initialize game state
	komi = stoi(opts["komi"]);
	boardsize = stoi(opts["boardsize"]);
	playouts = stoi(opts["playouts"]);
	full_traversals = stoi(opts["node_expansion_threshold"]);
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

bool budgie::make_move(const coordinate& c) {
	return make_move(move(move::types::Move, c, game.current_player));
}

budgie::move budgie::genmove(void) {
	// TODO: passing heuristics
	if (passed) {
		return move(budgie::move::types::Pass);
	}

	tree->reset();
	coordinate coord = tree->do_search(&game, playouts);
	float winrate = tree->win_rate(coord);

	// TODO: make resign threshold configurable
	if (winrate < 0.15) {
		return move(move::types::Resign);

	} else if (woncount >= 3 && winrate >= 1.0) {
		return move(move::types::Pass);

	} else if (!game.is_valid_move(coord)
	           || game.is_suicide(coord, game.current_player))
	{
		return move(move::types::Pass);

	} else {
		woncount = (winrate >= 1.0)? woncount + 1 : 0;

		return move(move::types::Move,
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
	game.current_player = p;
}

std::unique_ptr<mcts> budgie::init_mcts(args_parser::option_map& options) {
	pattern_dbptr db = pattern_dbptr(new pattern_db(options["patterns"]));
	double uct_weight = std::stof(options["uct_weight"]);
	unsigned rave_weight = std::stoi(options["rave_weight"]);

	// only have this tree policy right now
	tree_policy *tree_pol;
	(options["tree_policy"] == "mcts")
		? (tree_policy *)(new mcts_tree_policy(db))
		: (tree_policy *)(new uct_rave_tree_policy(db));

	if (options["tree_policy"] == "mcts") {
		tree_pol = new mcts_tree_policy(db);
	} else if (options["tree_policy"] == "uct") {
		tree_pol = new uct_tree_policy(db, uct_weight);
	} else {
		tree_pol = new uct_rave_tree_policy(db, uct_weight, rave_weight);
	}

	std::list<playout_strategy*> strats;
	auto policies = mcts_thing::split_string(options["playout_policy"]);

	for (auto policy : policies) {
		if (policy == "local_weighted") {
			strats.push_back(new local_weighted_playout(db));

		} else if (policy == "adjacent") {
			strats.push_back(new adjacent_playout(db));

		} else if (policy == "capture_enemy_ataris") {
			strats.push_back(new capture_weighted_playout(db));

		} else if (policy == "attack_enemy_groups") {
			strats.push_back(new attack_enemy_groups_playout(db));

		} else if (policy == "save_own_ataris") {
			strats.push_back(new save_atari_playout(db));

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
