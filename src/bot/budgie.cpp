#include <budgie/budgie.hpp>
#include <budgie/playout_strategies.hpp>
#include <budgie/tree_policies.hpp>
#include <budgie/pattern_db.hpp>
#include <budgie/distributed_mcts.hpp>
#include <budgie/distributed_client.hpp>
// TODO: only included for 'split_string()', should split that in a util file
#include <iostream>

#include <budgie/parameters.hpp>
#include <budgie/utility.hpp>

namespace mcts_thing {

budgie::budgie(args_parser::option_map& opts)
	: pool(stoi(opts["worker-threads"]))
{
	// cache options
	options = opts;

	// initlaize tree state
	tree = init_mcts(opts);

	// initialize game state
	komi      = stof(opts["komi"]);
	boardsize = stoi(opts["boardsize"]);
	playouts  = stoi(opts["playouts"]);

	ogsChat      = opts["ogs_output"] == "true";
	allowPassing = opts["pass"]       == "true";

	game.reset(boardsize, komi);
	game.loadJosekis(opts["joseki_db"]);

	// parse parameter key=value pairs
	std::string& params = opts["parameters"];

	for (const auto& foo : split_string(params, ',')) {
		size_t eq   = foo.find("=");
		size_t type = foo.find(":");

		if (eq == std::string::npos || type == std::string::npos)
			continue;

		std::string typestr = foo.substr(0, type);
		std::string key     = foo.substr(type + 1, eq - type - 1);
		std::string val     = foo.substr(eq + 1);

		std::cerr << "have parameter: "
			<< foo << ": "
			<< "key: " << key << ", "
			<< "type: " << typestr << ", "
			<< "value: " << val << ", "
			<< std::endl;

		auto unknown = [&] () {
			std::cerr << "Unknown key value: " << key
				<< " (for type " << typestr << ")"
				<< std::endl;
		};

		if (typestr == "int") {
			if (int idx = findParam(intParamNames, key.c_str())) {
				setInt(idx, int(stoi(val)));
			} else unknown();

		} else if (typestr == "bool") {
			if (int idx = findParam(boolParamNames, key.c_str())) {
				bool asdf = val == "true";
				setBool(idx, asdf);
			} else unknown();

		} else if (typestr == "float") {
			if (int idx = findParam(boolParamNames, key.c_str())) {
				setFloat(idx, stof(val));
			} else unknown();

		} else {
			std::cerr << "Unknown type: " << typestr << std::endl;
		}
	}
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
	coordinate coord = tree->do_search(&game, pool, playouts);
	float winrate = tree->win_rate(coord);

	// TODO: make resign threshold configurable
	if (winrate < 0.15) {
		return move(move::types::Resign);

	} else if ((woncount >= 3 && winrate >= 1.0)) {
		return move(move::types::Pass);

	} else if (allowPassing && tree->ownership_settled(&game)) {
		return move(move::types::Pass);

	// TODO: option to pass or play out to the bitter end
	} else if (!game.is_valid_move(coord)) {
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
	load_patterns(options["patterns"]);

	tree_policy tree_pol;
	if (options["tree_policy"] == "mcts") {
		tree_pol = policies::mcts_tree_policy;
	} else if (options["tree_policy"] == "uct") {
		tree_pol = policies::uct_tree_policy;
	} else {
		tree_pol = policies::uct_rave_tree_policy;
	}

	std::list<playout_strategy> strats;
	auto policies = split_string(options["playout_policy"], ',');

	for (auto policy : policies) {
		if (policy == "local_weighted") {
			strats.push_back(playouts::local_weighted_playout);

		} else if (policy == "adjacent-3x3") {
			strats.push_back(playouts::adjacent_3x3_playout);

		} else if (policy == "adjacent-5x5") {
			strats.push_back(playouts::adjacent_5x5_playout);

		} else if (policy == "capture_enemy_ataris") {
			strats.push_back(playouts::capture_weighted_playout);

		} else if (policy == "attack_enemy_groups") {
			strats.push_back(playouts::attack_enemy_groups_playout);

		} else if (policy == "save_own_ataris") {
			strats.push_back(playouts::save_atari_playout);

		} else if (policy == "random"){
			strats.push_back(playouts::random_playout);

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
