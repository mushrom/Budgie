#include <stdio.h>
#include <mcts-gb/game.hpp>
#include <map>
#include <iostream>
#include <bitset>

namespace mcts_thing {

bool board::is_valid_move(const coordinate& coord) {
	unsigned index = (coord.second - 1) * dimension + (coord.first - 1);

	if (!is_valid_coordinate(coord)
		|| grid[index] != point::color::Empty
		|| is_suicide(coord, current_player)
		|| violates_ko(coord))
	{
		return false;
	}

	return true;
}

bool board::violates_ko(const coordinate& coord) {
	if (!captures_enemy(coord, other_player(current_player))) {
		return false;
	}

	//uint64_t hash = gen_hash(coord, current_player);
	// TODO: this kills performance, reaaaaaallly need a fast way to check
	//       group liberties so we can check for captures instead...
	board foo(this);
	foo.make_move(coord);

	// TODO: do we really need to do full superko checking?
	unsigned k = 9;

	for (move::moveptr ptr = move_list;
	     ptr != nullptr && k;
	     ptr = ptr->previous, k--)
	{
		if (foo.hash == ptr->hash) {
			// TODO: check that it's not just a hash collision, although it's
			//       pretty unlikely
			return true;
		}

		/*
		bool is_different = false;

		for (unsigned i = 0; i < dimension * dimension; i++) {
			if (temp.grid[i] != ptr->grid[i]) {
				is_different = true;
				break;
			}
		}

		if (!is_different) {
			return true;
		}
		*/
	}

	return false;
}

bool board::reaches_iter(const coordinate& coord,
                         point::color color,
                         point::color target,
                         std::bitset<384>& marked)
{
	bool ret = false;

	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	marked[coord_to_index(coord)] = true;

	for (const auto& thing : {left, right, up, down}) {
		if (is_valid_coordinate(thing) && !marked[coord_to_index(thing)]) {
			point::color foo = get_coordinate(thing);

			if (foo == target) {
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

bool board::reaches(const coordinate& coord, point::color color, point::color target) {
	std::bitset<384> marked = {0};

	return reaches_iter(coord, color, target, marked);
}

bool board::reaches_empty(const coordinate& coord, point::color color) {
	return reaches(coord, color, point::color::Empty);
}

unsigned board::count_iter(const coordinate& coord,
                           point::color color,
                           point::color target,
                           std::bitset<384>& marked,
                           std::bitset<384>& traversed)
{
	unsigned ret = 0;

	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	marked[coord_to_index(coord)] = true;

	for (const auto& thing : {left, right, up, down}) {
		unsigned index = coord_to_index(thing);

		if (is_valid_coordinate(thing) && !marked[index]) {
			point::color foo = get_coordinate(thing);

			if (foo == target) {
				ret++;
				marked[index] = true;
			}

			else if (foo == color) {
				traversed[index] = true;
				ret += count_iter(thing, color, target, marked, traversed);
			}
		}
	}

	return ret;
}

unsigned board::count_reachable(const coordinate& coord,
                                point::color color,
                                point::color target,
                                std::bitset<384>& traversed)
{
	std::bitset<384> marked = {0};

	return count_iter(coord, color, target, marked, traversed);
}


void board::clear_stones(const coordinate& coord, point::color color) {
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

bool board::clear_enemy_stones(const coordinate& coord, point::color color) {
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
			clear_stones(thing, enemy);
			ret = true;
		}
	}

	return ret;
}

bool board::clear_own_stones(const coordinate& coord, point::color color) {
	if (!reaches_empty(coord, color)) {
		clear_stones(coord, color);
		return true;
	}

	return false;
}

bool board::captures_enemy(const coordinate& coord, point::color color) {
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

bool board::is_suicide(const coordinate& coord, point::color color) {
	return !(reaches_empty(coord, color) || captures_enemy(coord, color));
}

bool board::is_valid_coordinate(const coordinate& coord) {
	return !(coord.first < 1 || coord.second < 1
	         || coord.first > dimension || coord.second > dimension);
}

point::color board::get_coordinate(const coordinate& coord) {
	if (!is_valid_coordinate(coord)) {
		// XXX
		return point::color::Invalid;
	}

	return grid[(coord.second - 1)*dimension + (coord.first - 1)];
}

void board::make_move(const coordinate& coord) {
	if (!is_valid_coordinate(coord)) {
		fprintf(stderr, "# invalid move at (%u, %u)!", coord.first, coord.second);
		return;
	}

	bool any_captured = false;
	//unsigned index = (coord.second - 1) * dimension + (coord.first - 1);
	//grid[index] = current_player;
	set_coordinate(coord, current_player);
	any_captured = clear_enemy_stones(coord, current_player);
	any_captured = any_captured || clear_own_stones(coord, current_player);

	/*
	if (!any_captured) {
		//hash = gen_hash(coord, current_player);
		regen_hash();

	} else {
		regen_hash();
	}
	*/
	regen_hash();

	move_list = move::moveptr(new move(move_list, coord, current_player, hash));
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

unsigned board::territory_flood(const coordinate& coord,
                                bool is_territory,
                                std::bitset<384>& traversed_map)
{
	if (!is_valid_coordinate(coord)) {
		return 0;
	}

	unsigned index = coord_to_index(coord);

	if (traversed_map[index]) {
		return 0;
	}

	traversed_map[index] = true;

	if (get_coordinate(coord) != point::color::Empty) {
		return 0;

	} else {
		coordinate left  = {coord.first - 1, coord.second};
		coordinate right = {coord.first + 1, coord.second};
		coordinate up    = {coord.first,     coord.second - 1};
		coordinate down  = {coord.first,     coord.second + 1};

		unsigned ret = is_territory;

		for (const auto& thing : {left, right, up, down}) {
			ret += territory_flood(thing, is_territory, traversed_map);
		}

		return ret;
	}
}

void board::endgame_mark_captured(const coordinate& coord,
                                  point::color color,
                                  std::bitset<384>& marked)
{
	coordinate left  = {coord.first - 1, coord.second};
	coordinate right = {coord.first + 1, coord.second};
	coordinate up    = {coord.first,     coord.second - 1};
	coordinate down  = {coord.first,     coord.second + 1};

	marked[coord_to_index(coord)] = true;

	for (const auto& thing : {left, right, up, down}) {
		if (is_valid_coordinate(thing)
		    && !marked[coord_to_index(thing)]
		    && get_coordinate(thing) == color)
		{
			endgame_mark_captured(thing, color, marked);
		}
	}
}

// TODO: we could reuse the 'count reachable' map for faster traversal,
//       as a future optimization
//
//       ^ leaving the debug statements here for that
void board::endgame_clear_captured(void) {
	std::bitset<384> marked = {0};
	std::bitset<384> traversed = {0};

	// enumerate all stones immediately capturable
	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			unsigned index = coord_to_index(coord);
			point::color color = get_coordinate(coord);

			if (color == point::color::Empty
			   || traversed[index]
			   || marked[index]) {
				continue;
			}

			unsigned liberties = count_reachable(coord, color,
			                                     point::color::Empty, traversed);

			//std::cerr << "# group has " << liberties << " liberties" << std::endl;
			if (liberties < 2) {
				endgame_mark_captured(coord, color, marked);
			}
		}
	}

	// then clear them
	// NOTE: we clear after marking so that seki situations
	//       end up being counted correctly
	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			unsigned index = coord_to_index(coord);

			if (marked[index]) {
				set_coordinate(coord, point::color::Empty);
			}
		}
	}
}

unsigned board::coord_to_index(const coordinate& coord) {
	return dimension*coord.second + coord.first;
}

// TODO: Right now the bot the bot doesn't count stones in atari as being dead,
//       which impacts territory counting, since playing a stone in your opponents
//       territory immediately makes their territory invalid in the bot's eyes...
//
//       Also makes it very annoying to play sometimes.
unsigned board::count_territory(point::color player) {
	std::bitset<384> bitmap = {0};
	unsigned territory = 0;

	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			unsigned index = coord_to_index(coord);

			if (bitmap[index] || get_coordinate(coord) != point::color::Empty) {
				continue;
			}

			bool is_territory = !reaches(coord, point::color::Empty, other_player(player));
			territory += territory_flood(coord, is_territory, bitmap);
		}
	}

	return territory;
}

std::vector<coordinate> board::available_moves(void) {
	std::vector<coordinate> ret = {};

	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};

			if (is_valid_move(coord) && !is_suicide(coord, current_player)) {
				ret.push_back(coord);
			}
		}
	}

	return ret;
}

#include <unistd.h>

point::color board::determine_winner(void) {
	/*
	std::cerr << "# board before captures: " << std::endl;
	print();

	endgame_clear_captured();

	std::cerr << "# board after captures: " << std::endl;
	print();
	*/

	int white_stones = count_stones(point::color::White) + komi;
	int black_stones = count_stones(point::color::Black);

	int white_territory = count_territory(point::color::White);
	int black_territory = count_territory(point::color::Black);

	int white = white_stones + white_territory;
	int black = black_stones + black_territory;

	/*
	// TODO: maybe add an option to show predicted score and most likely playout
	//print();
	printf(
		"game result: %d points black, %d points white\n"
		"             black: %d territory, %d stones\n"
		"             white: %d territory, %d stones\n"
		"             %u moves made\n",
		black, white,
		black_territory, black_stones,
		white_territory, white_stones,
		moves
	);

	for (move::moveptr thing = move_list; thing != nullptr; thing = thing->previous) {
		printf(
			" -> %s : (%u, %u)\n",
			(thing->color == point::color::Black)? "black" : "white",
			thing->coord.first, thing->coord.second
		);
	}

	sleep(1);
	*/

	return (black > white)? point::color::Black : point::color::White;
}

uint64_t board::gen_hash(const coordinate& coord, point::color color) {
	// generate 12 bit identifier for this move
	uint16_t foo = (color << 10) | (coord.first << 5) | coord.second;

	// note: we need an associative hash here, since the board hash
	//       will need to be rebuilt outside of move order, this is a good
	//       hash for uniquely identifying games though
	//return (hash << 12) + hash + foo;
	return hash + (uint64_t)InitialHash * (uint64_t)foo;
}

void board::regen_hash(void) {
	hash = InitialHash;

	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			point::color color = get_coordinate(coord);

			if (color != point::color::Empty) {
				hash = gen_hash(coord, color);
			}
		}
	}
}

void board::set_coordinate(const coordinate& coord, point::color color) {
	if (is_valid_coordinate(coord)) {
		unsigned index = (coord.second - 1)*dimension + (coord.first - 1);
		grid[index] = color;
	}
}

void board::print(void) {
	//printf("\e[1;1H");

	for (unsigned y = dimension; y > 0; y--) {
		printf("# %2d ", y);
		for (unsigned x = 1; x <= dimension; x++) {
			//unsigned index = y * dimension + x;
			coordinate coord = {x, y};
			printf("%c ", ". O#"[get_coordinate(coord)]);
		}

		printf(" \n");
	}

	std::string letters = "ABCDEFGHJKLMNOPQRSTUVWXYZ";

	printf("#   ");
	for (unsigned x = 0; x < dimension; x++) {
		printf("%2c", letters[x]);
	}
	printf(" \n");
}

// namespace mcts_thing
};
