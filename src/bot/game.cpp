#include <budgie/game.hpp>
#include <anserial/anserial.hpp>
#include <stdio.h>
#include <map>
#include <iostream>
#include <bitset>
#include <stdexcept>
// for memcpy
#include <string.h>

namespace mcts_thing {

// TODO: error on board sizes larger than \sqrt(384)
board::board(unsigned size) {
	dimension = size;

	for (unsigned i = 0; i < 384; i++) {
		ownership[i] = grid[i] = point::color::Empty;
		groups[i] = nullptr;
	}

	for (unsigned i = 0; i < dimension * dimension; i++) {
		available_moves.insert(index_to_coord(i));
	}
}

board::board(board *other){
	set(other);
}

board::~board() {
	for (unsigned i = 0; i < dimension*dimension; i++) {
		if (groups[i]) {
			group_clear(groups + i);
		}
	}
}

void board::set(board *other) {
	// copy game info
	komi = other->komi;
	dimension = other->dimension;
	current_player = other->current_player;
	moves = other->moves;
	last_move = other->last_move;

	// copy internal info
	move_list = other->move_list;
	hash = other->hash;
	available_moves.clear();
	available_moves.insert(other->available_moves.begin(),
	                       other->available_moves.end());

	for (unsigned i = 0; i < dimension * dimension; i++) {
		groups[i] = nullptr;
	}

	for (auto& s : group_liberties) {
		for (auto& k : s) {
			k.clear();
		}
	}

	for (unsigned i = 0; i < dimension * dimension; i++) {
		grid[i] = other->grid[i];
		ownership[i] = point::color::Empty;

		if (!groups[i] && other->groups[i]) {
			groups[i] = new group;
			groups[i]->color = other->groups[i]->color;
			groups[i]->members.insert(groups[i]->members.end(),
			                          other->groups[i]->members.begin(),
			                          other->groups[i]->members.end());

			groups[i]->liberties.insert(other->groups[i]->liberties.begin(),
			                            other->groups[i]->liberties.end());

			group_propagate(groups + i);
		}
	}
}

void board::reset(unsigned boardsize, unsigned n_komi) {
	current_player = point::color::Black;
	dimension = boardsize;
	komi = n_komi;
	moves = 0;
	move_list = nullptr;

	available_moves.clear();

	for (unsigned i = 0; i < dimension * dimension; i++) {
		grid[i] = point::color::Empty;

		if (groups[i]) {
			group_clear(groups + i);
		}

		available_moves.insert(index_to_coord(i));
	}

	for (auto& s : group_liberties) {
		for (auto& k : s) {
			k.clear();
		}
	}
}

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
	group *capturable[4];
	size_t n_capturable = group_capturable(coord, capturable);

	if (n_capturable == 0 || n_capturable > 1) {
		// can't have a ko violation if there's more than one group
		// being captured, I think? Makes sense to me, but I'm not sure
		// if it's actually true
		return false;
	}

	uint64_t temphash = group_pseudocapture(coord, capturable);

	// XXX: only check few past moves for superko violations
	unsigned k = 9;

	for (move::moveptr ptr = move_list;
	     ptr != nullptr && k;
	     ptr = ptr->previous, k--)
	{
		if (temphash == ptr->hash) {
			// XXX: doesn't check for hash collisions, too slow
			return true;
		}
	}

	return false;
}

bool board::reaches_iter(const coordinate& coord,
                         point::color color,
                         point::color target,
                         std::bitset<384>& marked)
{
	bool ret = false;
	marked[coord_to_index(coord)] = true;

	for (const auto& thing : adjacent(coord)) {
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
	marked[coord_to_index(coord)] = true;

	for (const auto& thing : adjacent(coord)) {
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
	if (get_coordinate(coord) != color) {
		return;
	}

	set_coordinate(coord, point::color::Empty);

	// TODO: lambdas?
	for (auto thing : adjacent(coord)) {
		if (is_valid_coordinate(thing)) {
			clear_stones(thing, color);
		}
	}
}

bool board::clear_enemy_stones(const coordinate& coord, point::color color) {
	bool ret = false;

	point::color enemy = (color == point::color::Black)
	                     ? point::color::White
	                     : point::color::Black;

	for (auto thing : adjacent(coord)) {
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
	// XXX: pretend to place a stone there, could make the initial coord in
	//      reaches_empty() be filled though...
	set_coordinate(coord, color);
	bool ret = false;

	point::color enemy = (color == point::color::Black)
	                     ? point::color::White
	                     : point::color::Black;

	for (const auto& thing : adjacent(coord)) {
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
	/*
	return !(coord.first < 1 || coord.second < 1
	         || coord.first > dimension || coord.second > dimension);
			 */
	return is_valid_coordinate(coord.first, coord.second);
}

bool board::is_valid_coordinate(unsigned x, unsigned y) {
	return !(x < 1 || y < 1 || x > dimension || y > dimension);
}

point::color board::get_coordinate(const coordinate& coord) {
	if (!is_valid_coordinate(coord)) {
		return point::color::Invalid;
	}

	return (point::color)grid[(coord.second - 1)*dimension + (coord.first - 1)];
}

point::color board::get_coordinate(unsigned x, unsigned y) {
	if (!is_valid_coordinate(x, y)) {
		return point::color::Invalid;
	}

	return (point::color)grid[(y - 1)*dimension + (x - 1)];
}

bool board::make_move(const coordinate& coord) {
	if (!is_valid_move(coord)) {
		fprintf(stderr, "# invalid move at (%u, %u)!", coord.first, coord.second);
		return false;
	}

	set_coordinate(coord, current_player);
	group_place(coord);
	clear_enemy_stones(coord, current_player);
	regen_hash();

	move_list = move::moveptr(new move(move_list, coord, current_player, hash));
	last_move = coord;
	moves++;

	current_player = other_player(current_player);
	return true;
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
		unsigned ret = is_territory;

		for (const auto& thing : adjacent(coord)) {
			ret += territory_flood(thing, is_territory, traversed_map);
		}

		return ret;
	}
}

void board::endgame_mark_captured(const coordinate& coord,
                                  point::color color,
                                  std::bitset<384>& marked)
{
	marked[coord_to_index(coord)] = true;

	for (const auto& thing : adjacent(coord)) {
		if (is_valid_coordinate(thing)
		    && !marked[coord_to_index(thing)]
		    && get_coordinate(thing) == color)
		{
			endgame_mark_captured(thing, color, marked);
		}
	}
}

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
	// NOTE: clear after marking so that seki situations
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
	return dimension*(coord.second - 1) + (coord.first - 1);
}

coordinate board::index_to_coord(unsigned index) {
	return {(index % dimension) + 1, (index / dimension) + 1};
}

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

			bool is_territory = !reaches(coord, point::color::Empty,
			                             other_player(player));
			territory += territory_flood(coord, is_territory, bitmap);
		}
	}

	return territory;
}

#include <unistd.h>

point::color board::determine_winner(void) {
	endgame_clear_captured();

	int white_stones = count_stones(point::color::White) + komi;
	int black_stones = count_stones(point::color::Black);

	int white_territory = count_territory(point::color::White);
	int black_territory = count_territory(point::color::Black);

	int white = white_stones + white_territory;
	int black = black_stones + black_territory;

	return (black > white)? point::color::Black : point::color::White;
}

bool board::owns(const coordinate& coord, point::color color) {
	return grid[coord_to_index(coord)] == color;
}

uint64_t board::gen_hash(const coordinate& coord,
                         point::color color,
                         uint64_t hash)
{
	// generate 12 bit identifier for this move
	uint16_t foo = (color << 10) | (coord.first << 5) | coord.second;

	return (hash << 12) + hash + foo;
}

void board::regen_hash(void) {
	uint64_t ret = InitialHash;

	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			point::color color = get_coordinate(coord);

			if (color != point::color::Empty) {
				ret = gen_hash(coord, color, ret);
			}
		}
	}

	hash = ret;
}

uint64_t board::regen_hash(uint8_t tempgrid[384]) {
	uint64_t ret = InitialHash;

	for (unsigned y = 1; y <= dimension; y++) {
		for (unsigned x = 1; x <= dimension; x++) {
			coordinate coord = {x, y};
			point::color color = (point::color)tempgrid[coord_to_index(coord)];

			if (color != point::color::Empty) {
				ret = gen_hash(coord, color, ret);
			}
		}
	}

	return ret;
}

void board::set_coordinate(const coordinate& coord, point::color color) {
	if (is_valid_coordinate(coord)) {
		grid[coord_to_index(coord)] = color;
	}
}

void board::print(void) {
	for (unsigned y = dimension; y > 0; y--) {
		printf("# %2d ", y);
		for (unsigned x = 1; x <= dimension; x++) {
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

bool board::group_check(void) {
	for (unsigned i = 0; i < dimension*dimension; i++) {
		int y = i / dimension + 1;
		int x = i % dimension + 1;

		if (groups[i]) {
			if (groups[i]->color != grid[i]) {
				puts("color mismatch");
				return false;
			}
		}

		if (!groups[i] && grid[i] != point::color::Empty) {

			print();
			printf("no group for stones on board at (%d, %d)\n", x, y);
			group_print();
			return false;
		}

		auto it = available_moves.find(index_to_coord(i));

		if (it == available_moves.end() && grid[i] == point::color::Empty) {
			printf("have no available move for empty space at (%d, %d)", x, y);
			return false;

		} else if (it != available_moves.end()
		           && grid[i] != point::color::Empty)
		{
			printf("invalid available move at (%d, %d)", x, y);
			return false;
		}
	}

	return true;
}

void board::group_print(void) {
	for (unsigned y = dimension; y > 0; y--) {
		printf("# %2u ", y);

		for (unsigned x = 1; x <= dimension; x++) {
			unsigned index = coord_to_index({x, y});

			if (groups[index]) {
				char c = ((groups[index]->color == point::color::Black)? 'O' : '#'); 
				printf("%c ", c);
			}

			else {
				printf(". ");
			}
		}

		putchar('\n');
	}
}

void board::group_place(const coordinate& coord) {
	// TODO: move this somewhere else eventually
	unsigned index = coord_to_index(coord);
	group **g = groups + index;

	if (*g) {
		puts("group_place(): occupied location...");
		// there's already a group here...
		return;
	}

	*g = new group;
	(*g)->members.push_back(coord);
	(*g)->color = current_player;
	available_moves.erase(coord);

	for (const auto& temp : adjacent(coord)) {
		if (!is_valid_coordinate(temp)) {
			continue;
		}

		point::color p = get_coordinate(temp);

		if (p == point::color::Empty)
			(*g)->liberties.insert(temp);

		else if (p == current_player)
			group_link(g, groups + coord_to_index(temp));

		// enemy group
		else if (p == other_player(current_player))
			group_try_capture(groups + coord_to_index(temp), coord);
	}

	// make sure the current position didn't end up in the liberty lists,
	// in case this resulted in a group link
	if (g && *g) {
		if ((*g)->liberties.size() == 0) {
			puts("group_place(): hmmmst, suicidal move?");
		}

		(*g)->liberties.erase(coord);
		group_libs_update(*g);

		if ((*g)->liberties.size() == 0) {
			puts("group_place(): yep still 0 liberties");
		}
	}

	if (!*g) {
		puts("dude where's my pointer");
	}

	group_libs_update(*g);
}

void board::group_link(group **a, group **b) {
	if (!a || !b || !*a || !*b) {
		puts("group_link(): null pointers");
		return;
	}

	if (*a == *b) {
		// already the same group, nothing to link
		return;
	}

	group **smaller = ((*a)->members.size() <= (*b)->members.size())? a : b;
	group **larger  = ((*a)->members.size() >  (*b)->members.size())? a : b;
	group *small_ptr = *smaller;

	// update group pointers
	for (const coordinate& c : small_ptr->members) {
		unsigned index = coord_to_index(c);
		groups[index] = *larger;
	}

	// splice/copy group data
	(*larger)->members.splice((*larger)->members.end(),
	                          small_ptr->members,
	                          small_ptr->members.begin(),
	                          small_ptr->members.end());
	(*larger)->liberties.insert(small_ptr->liberties.begin(),
	                            small_ptr->liberties.end());

	group_libs_remove(small_ptr);
	group_libs_update(*larger);
	delete small_ptr;
}

void board::group_try_capture(group **a, const coordinate& coord) {
	if (!a || !*a) {
		return;
	}

	if ((*a)->color == current_player) {
		puts("group_try_capture(): friendlies (this is a bug)");
		return;
	}

	(*a)->liberties.erase(coord);
	group_libs_update(*a);

	if ((*a)->liberties.size() == 0) {
		// TODO: wrap available_moves stuff into it's own function
		for (const coordinate& c : (*a)->members) {
			// update map of available moves
			available_moves.insert(c);
		}

		group_update_neighbors(a);
		group_clear(a);
	}
}

void board::group_clear(group **a) {
	if (!a || !*a) {
		puts("group_clear(): nothing to clear");
		return;
	}

	group *deadptr = *a;

	for (const coordinate& c : deadptr->members) {
		unsigned index = coord_to_index(c);

		if (groups[index] != deadptr) {
			puts("group_clear(): mismatched member pointers");
		}

		groups[index] = nullptr;
	}

	group_libs_remove(deadptr);
	delete deadptr;
}

void board::group_update_neighbors(group **a) {
	if (!a || !*a) {
		puts("group_update_neighbors(): null pointer");
		return;
	}

	for (const coordinate& c : (*a)->members) {
		for (const coordinate& temp : adjacent(c)) {
			if (!is_valid_coordinate(temp)) continue;

			unsigned index = coord_to_index(temp);
			group *ptr = groups[index];

			if (ptr && ptr->color == current_player) {
				ptr->liberties.insert(c);
				group_libs_update(ptr);
			}
		}
	}
}

void board::group_libs_remove(group *a) {
	if (!a) return;
	if (a->last_update == 0xdeadbeef) return;

	group_liberties[a->color][a->last_update].erase(a->it);
	a->last_update = 0xdeadbeef;
}

void board::group_libs_update(group *a) {
	if (!a) return;

	if (a->last_update != 0xdeadbeef) {
		std::list<group*>& ogrp = group_liberties[a->color][a->last_update];
		ogrp.erase(a->it);
	}

	std::list<group*>& ngrp = group_liberties[a->color][a->liberties.size()];

	ngrp.push_front(a);
	a->it = ngrp.begin();
	a->last_update = a->liberties.size();
}

void board::group_propagate(group **a) {
	if (!a || !*a) {
		puts("group_propagate(): null pointers");
	}

	for (const coordinate& c : (*a)->members) {
		unsigned index = coord_to_index(c);
		groups[index] = *a;
	}

	group_libs_update(*a);
}

size_t board::group_capturable(const coordinate& coord, group *arr[4]) {
	const auto& coords = adjacent(coord);
	size_t ret = 0;

	for (unsigned i = 0; i < 4; i++) {
		const auto& n = coords[i];

		if (!is_valid_coordinate(n)) {
			arr[i] = nullptr;
			continue;
		}

		group *neighbor = groups[coord_to_index(n)];

		bool captures = neighbor
		    && neighbor->color == other_player(current_player)
		    && neighbor->liberties.size() == 1
		    && neighbor->liberties.find(coord) != neighbor->liberties.end();

		arr[i] = captures? neighbor : nullptr;
		ret += captures;
	}

	return ret;
}

// takes an array generated by group_capturable and generates a hash
// that simulates capturing those groups
uint64_t board::group_pseudocapture(const coordinate& coord, group *arr[4]) {
	//point::color tempgrid[384];
	uint8_t tempgrid[384];
	memcpy(tempgrid, grid, sizeof(grid));

	for (unsigned i = 0; i < 4; i++) {
		if (!arr[i]) continue;

		group *p = arr[i];
		for (coordinate& c : p->members) {
			unsigned index = coord_to_index(c);
			tempgrid[index] = point::color::Empty;
		}
	}

	tempgrid[coord_to_index(coord)] = current_player;
	return regen_hash(tempgrid);
}

void board::serialize(anserial::serializer& ser, uint32_t parent) {
	uint32_t datas = ser.add_entities(parent,
		{"board",
			{"dimension", dimension},
			// TODO: need to add signed int, double types to anserial
			{"komi", (uint32_t)komi},
			{"current_player", current_player}});

	uint32_t move_entry = ser.add_entities(datas, {"moves"});
	uint32_t move_datas = ser.add_entities(move_entry, {});

	for (auto& ptr = move_list; ptr != nullptr; ptr = ptr->previous) {
		ser.add_entities(move_datas,
			{"move",
				{"coordinate", {ptr->coord.first, ptr->coord.second}},
				{"player", ptr->color},
				{"hash", {(uint32_t)(ptr->hash >> 32),
				          (uint32_t)(ptr->hash & 0xffffffff)}},
				});
	}
}

#include <cassert>

void board::deserialize(anserial::s_node *node) {
	anserial::s_node* move_datas;
	uint32_t n_dimension, n_komi;
	point::color n_current;

	if (!anserial::destructure(node,
		{"board",
			{"dimension", &n_dimension},
			{"komi", &n_komi},
			{"current_player", (uint32_t*)&n_current},
			{"moves", &move_datas}}))
	{
		throw std::logic_error("board::deserialize(): invalid board tree structure");
	}

	reset(n_dimension, n_komi);
	current_player = n_current;

	auto& nodes = move_datas->entities();
	for (auto it = nodes.rbegin(); it != nodes.rend(); it++) {
		anserial::s_node* move_node = *it;
		coordinate coord;
		point::color color;
		uint32_t temphash[2];

		if (!anserial::destructure(move_node,
			{"move",
				{"coordinate", {&coord.first, &coord.second}},
				{"player", (uint32_t*)&color},
				{"hash", {temphash, temphash + 1}}}))
		{
			throw std::logic_error("board::deserialize(): invalid move");
		}

		current_player = color;
		make_move(coord);
	}
}

// namespace mcts_thing
};
