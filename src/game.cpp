#include <stdio.h>
#include <mcts-gb/game.hpp>
#include <map>

namespace mcts_thing {

bool board::is_valid_move(coordinate& coord) {
	unsigned index = (coord.second - 1) * dimension + (coord.first - 1);

	return grid[index] == point::color::Empty;
}

bool board::reaches_iter(coordinate& coord,
                         point::color color,
                         point::color target,
                         std::map<coordinate, bool>& marked)
{
	bool ret = false;

	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	marked[coord] = true;

	for (auto thing : {left, right, up, down}) {
		if (is_valid_coordinate(thing) && marked.find(thing) == marked.end()) {
			point::color foo = get_coordinate(thing);

			if (foo == point::color::Empty) {
				ret = true;
				break;
			}

			else if (color == foo && (ret = reaches_iter(thing, color, target, marked))) {
				break;
			}
		}
	}

	return ret;
}

bool board::reaches(coordinate& coord, point::color color, point::color target) {
	// XXX: map here is really bad, feels gross tbh
	std::map<coordinate, bool> marked;

	return reaches_iter(coord, color, target, marked);
}

bool board::reaches_empty(coordinate& coord, point::color color) {
	return reaches(coord, color, point::color::Empty);
}

void board::clear_stones(coordinate& coord, point::color color) {
	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	if (get_coordinate(coord) != color) {
		return;
	}

	//printf("clearing (%u, %u, %u (%u))\n", coord.first, coord.second, color, get_coordinate(coord));

	set_coordinate(coord, point::color::Empty);

	// TODO: lambdas?
	for (auto thing : {left, right, up, down}) {
		if (is_valid_coordinate(thing)) {
			clear_stones(thing, color);
		}
	}
}

void board::clear_enemy_stones(coordinate& coord, point::color color) {
	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	point::color enemy = (color == point::color::Black)
	                     ? point::color::White
	                     : point::color::Black;

	for (auto thing : {left, right, up, down}) {
		if (get_coordinate(thing) == enemy && !reaches_empty(thing, enemy)) {
			clear_stones(thing, enemy);
		}
	}
}

void board::clear_own_stones(coordinate& coord, point::color color) {
	if (!reaches_empty(coord, color)) {
		clear_stones(coord, color);
	}
}

bool board::captures_enemy(coordinate& coord, point::color color) {
	if (get_coordinate(coord) != point::color::Empty) {
		return false;
	}

	// XXX: pretend to place a stone there, we could make the initial coord in
	//      reaches_empty() be filled though...
	set_coordinate(coord, color);
	bool ret = false;

	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	point::color enemy = (color == point::color::Black)
	                     ? point::color::White
	                     : point::color::Black;

	for (auto thing : {left, right, up, down}) {
		if (get_coordinate(thing) == enemy && !reaches_empty(thing, enemy)) {
			ret = true;
			break;
		}
	}

	set_coordinate(coord, point::color::Empty);
	return ret;
}

bool board::is_suicide(coordinate& coord, point::color color) {
	return !(reaches_empty(coord, color) || captures_enemy(coord, color));
}

bool board::is_valid_coordinate(coordinate& coord) {
	return !(coord.first < 1 || coord.second < 1
	         || coord.first > dimension || coord.second > dimension);
}

point::color board::get_coordinate(coordinate& coord) {
	if (!is_valid_coordinate(coord)) {
		// XXX
		return point::color::Invalid;
	}

	return grid[(coord.second - 1)*dimension + (coord.first - 1)];
}

void board::make_move(coordinate& coord) {
	//unsigned index = (coord.second - 1) * dimension + (coord.first - 1);
	//grid[index] = current_player;
	set_coordinate(coord, current_player);
	clear_enemy_stones(coord, current_player);
	clear_own_stones(coord, current_player);

	last_move = coord;
	moves++;

	current_player = (current_player == point::color::Black)
	                 ? point::color::White
	                 : point::color::Black;
}

unsigned board::count_stones(point::color player) {
	unsigned ret = 0;

	for (unsigned i = 0; i < dimension*dimension; i++) {
		ret += grid[i] == player;
	}

	return ret;
}

unsigned board::count_territory(point::color player) {
	unsigned territory = 0;

	// XXX: horribly inefficient, need to do a flood fill sort of thing
	for (unsigned x = 0; x < dimension; x++) {
		for (unsigned y = 0; y < dimension; y++) {
			coordinate coord = {x, y};
			territory += reaches(coord, point::color::Empty, player)
			             && !reaches(coord, point::color::Empty, other_player(player));
		}
	}

	return 0;
}

point::color board::determine_winner(void) {
	int white = count_stones(point::color::White) + komi;
	int black = count_stones(point::color::Black);

	white += count_territory(point::color::White);
	black += count_territory(point::color::Black);

	//printf("game result: %u points black, %u points white\n", black, white);

	return (black > white)? point::color::Black : point::color::White;
}

void board::set_coordinate(coordinate& coord, point::color color) {
	if (is_valid_coordinate(coord)) {
		unsigned index = (coord.second - 1)*dimension + (coord.first - 1);
		grid[index] = color;
	}
}

void board::print(void) {
	//printf("\e[1;1H");

	for (unsigned y = 0; y < dimension; y++) {
		printf("# ");
		for (unsigned x = 0; x < dimension; x++) {
			unsigned index = y * dimension + x;

			printf("%c ", ". O#"[grid[index]]);
		}

		printf(" \n");
	}
}

// namespace mcts_thing
};
