#include <budgie/game.hpp>
#include <anserial/anserial.hpp>
#include <stdio.h>
#include <map>
#include <iostream>
#include <bitset>
#include <stdexcept>

namespace mcts_thing {

board::board(unsigned size) {
	grid.reserve(size * size);
	dimension = size;

	for (unsigned i = 0; i < size*size; i++){
		ownership[i] = grid[i] = point::color::Empty;
		groups[i] = nullptr;
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

	grid.reserve(dimension * dimension);

	for (unsigned i = 0; i < dimension * dimension; i++) {
		groups[i] = nullptr;
	}

	for (unsigned i = 0; i < dimension * dimension; i++) {
		grid[i] = other->grid[i];
		ownership[i] = point::color::Empty;

		if (!groups[i] && other->groups[i]) {
			groups[i] = new group;
			groups[i]->color = other->groups[i]->color;
			//*groups[i] = *other->groups[i];
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
	grid.reserve(dimension * dimension);
	moves = 0;

	for (unsigned i = 0; i < dimension * dimension; i++) {
		grid[i] = point::color::Empty;
	}
}

point::color board::other_player(point::color color) {
	return (color == point::color::Black)
		? point::color::White
		: point::color::Black;
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
	if (!captures_enemy(coord, current_player)) {
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
	/*
	if (get_coordinate(coord) != point::color::Empty) {
		return false;
	}
	*/

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

	for (const auto& thing : {left, right, up, down}) {
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

	set_coordinate(coord, current_player);
	group_place(coord);
	bool any_captured = clear_enemy_stones(coord, current_player);
	any_captured = any_captured || clear_own_stones(coord, current_player);
	printf("group check: %u\n", group_check());


	regen_hash();

	move_list = move::moveptr(new move(move_list, coord, current_player, hash));
	last_move = coord;
	moves++;

	/*
	current_player = (current_player == point::color::Black)
	                 ? point::color::White
	                 : point::color::Black;
					 */

	current_player = other_player(current_player);
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
	return dimension*(coord.second - 1) + (coord.first - 1);
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

			bool is_territory = !reaches(coord, point::color::Empty,
			                             other_player(player));
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

			if (is_valid_move(coord)) {
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
	*/

	endgame_clear_captured();

	/*
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

bool board::owns(const coordinate& coord, point::color color) {
	return grid[coord_to_index(coord)] == color;
}

uint64_t board::gen_hash(const coordinate& coord, point::color color) {
	// generate 12 bit identifier for this move
	uint16_t foo = (color << 10) | (coord.first << 5) | coord.second;

	// note: we need an associative hash here, since the board hash
	//       will need to be rebuilt outside of move order, this is a good
	//       hash for uniquely identifying games though
	return (hash << 12) + hash + foo;
	//return hash + (uint64_t)InitialHash * (uint64_t)foo;
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
		//unsigned index = (coord.second - 1)*dimension + (coord.first - 1);
		//grid[index] = color;
		grid[coord_to_index(coord)] = color;
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

bool board::group_check(void) {
	for (unsigned i = 0; i < dimension*dimension; i++) {
		if (groups[i]) {
			if (groups[i]->color != grid[i]) {
				puts("color mismatch");
				return false;
			}
		}

		if (!groups[i] && grid[i] != point::color::Empty) {
			int y = i / dimension + 1;
			int x = i % dimension + 1;

			print();
			printf("no group for stones on board at (%d, %d)\n", x, y);
			group_print();
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

	static std::vector<coordinate> coords = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};

	for (auto& diff : coords) {
		coordinate temp = {coord.first + diff.first, coord.second + diff.second};
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

		if ((*g)->liberties.size() == 0) {
			puts("group_place(): yep still 0 liberties");
		}
	}

	if (!*g) {
		puts("dude where's my pointer");
	}

	/*
	if ((*g)->liberties.size() == 1) {
		ataris[current_player].insert(g);
	}
	*/
}

void board::group_link(group **a, group **b) {
	if (!a || !b || !*a || !*b) {
		//print();
		puts("group_link(): null pointers");
		return;
	}

	if (*a == *b) {
		//puts("ayyy sup");
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

	delete small_ptr;
}

void board::group_try_capture(group **a, const coordinate& coord) {
	if (!a || !*a) {
		//puts("eh?");
		return;
	}

	if ((*a)->color == current_player) {
		puts("friendlies");
		return;
	}

	(*a)->liberties.erase(coord);

	if ((*a)->liberties.size() == 0) {
		//puts("boom");
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

	delete deadptr;
}

void board::group_update_neighbors(group **a) {
	if (!a || !*a) return;
	static std::vector<coordinate> coords = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};

	for (const coordinate& c : (*a)->members) {
		for (const coordinate& diff : coords) {
			coordinate temp = {c.first + diff.first, c.second + diff.second};
			if (!is_valid_coordinate(temp)) continue;

			unsigned index = coord_to_index(temp);
			group *ptr = groups[index];

			if (ptr && ptr->color == other_player(current_player)) {
				ptr->liberties.insert(c);
			}
		}
	}
}

void board::group_propagate(group **a) {
	if (!a || !*a) {
		puts("group_propagate(): null pointers");
	}

	for (const coordinate& c : (*a)->members) {
		unsigned index = coord_to_index(c);
		groups[index] = *a;
	}
}

void board::serialize(anserial::serializer& ser, uint32_t parent) {
	uint32_t datas = ser.add_entities(parent,
		{"board",
			{"dimension", dimension},
			// TODO: need to add signed int, double types to anserial
			{"komi", (uint32_t)komi},
			{"current_player", current_player}});

	uint32_t grid_entry = ser.add_entities(datas, {"grid"});
	uint32_t grid_datas = ser.add_entities(grid_entry, {});

	for (unsigned i = 0; i < dimension*dimension; i++) {
		ser.add_entities(grid_datas, grid[i]);
	}
}

void board::deserialize(anserial::s_node *node) {
	anserial::s_node* grid_datas;
	uint32_t n_dimension, n_komi;
	point::color n_current;

	if (!anserial::destructure(node,
		{"board",
			{"dimension", &n_dimension},
			{"komi", &n_komi},
			{"current_player", (uint32_t*)&n_current},
			{"grid", &grid_datas}}))
	{
		throw std::logic_error("board::deserialize(): invalid board tree structure");
	}

	reset(n_dimension, n_komi);
	current_player = n_current;

	for (unsigned i = 0; i < dimension*dimension; i++) {
		grid[i] = (point::color)grid_datas->get(i)->uint();
	}
}

// namespace mcts_thing
};
