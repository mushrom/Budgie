#include <budgie/serialize.hpp>

#include <schemas/Node.capnp.h>
#include <schemas/UpdateTree.capnp.h>
#include <schemas/Board.capnp.h>

using namespace bdg;

static
uint32_t serialize_node(Node::Builder& builder,
                        mcts_node* ptr,
                        uint32_t since)
{
	if (ptr == nullptr) {
		return 0;
	}

	if (ptr->updates < since) {
		return 0;
	}

	if (ptr->traversals == 0) {
		return 0;
	}

	builder.setWins(ptr->wins);
	builder.setTraversals(ptr->traversals);
	builder.setPriorWins(ptr->prior_wins);
	builder.setPriorTraversals(ptr->prior_wins);

	auto coordbuild = builder.getCoord();
	coordbuild.setX(ptr->coord.first);
	coordbuild.setY(ptr->coord.second);

	auto leaves = builder.initLeaves(ptr->leaves_alive.size());
	auto it = ptr->leaves_alive.begin();

	for (unsigned i = 0; it != ptr->leaves_alive.end(); ++it, i++) {
		Node::Builder leafBuilder = leaves[i];
		serialize_node(leafBuilder, *it, since);
	}

	/*
	uint32_t self = ser.add_entities(parent,
		{"node",
			{"color", ptr->color},
			{"coordinate", {ptr->coord.first, ptr->coord.second}},
			{"traversals", ptr->traversals},
			{"updates", ptr->updates}});

	uint32_t leaves = ser.add_entities(self, {"leaves"});
	uint32_t leaves_cont = ser.add_entities(leaves, {});

	for (const auto& x : ptr->leaves) {
		serialize_node(ser, leaves_cont, x.second.get(), since);
	}

	uint32_t nodestats = ser.add_entities(self, {"node-stats"});
	uint32_t nodestats_cont = ser.add_entities(nodestats, {});

	for (const auto& x : ptr->nodestats) {
		coordinate coord = x.first;
		stats sts = x.second;

		ser.add_entities(nodestats_cont,
			{"leaf",
				{"traversals", sts.traversals},
				{"wins", sts.wins},
				{"coordinate", {coord.first, coord.second}}});
	}

	uint32_t ravestats = ser.add_entities(self, {"rave-stats"});
	uint32_t ravestats_cont = ser.add_entities(ravestats, {});

#ifdef SERIALIZE_RAVE
	for (const auto& x : ptr->rave) {
		//if (x.second.traversals > 50) {
			ser.add_entities(ravestats_cont,
				{"stats",
					{"traversals", x.second.traversals},
					{"wins", x.second.wins},
					{"coordinate", {x.first.first, x.first.second}}});
		//}
	}
#endif

	*/
	return 0;
};

static
mcts_node *deserialize_node(Node::Reader& reader, mcts_node *ptr) {
	/*
	point::color color;
	coordinate coord;
	uint32_t node_traversals;
	uint32_t node_updates;

	anserial::s_node *leaves;
	anserial::s_node *node_stats;
	anserial::s_node *rave;

	if (!node) {
		return nullptr;
	}

	if (!ptr) {
		return nullptr;
	}

	if (!anserial::destructure(node,
		{"node",
			{"color", (uint32_t*)&color},
			{"coordinate", {&coord.first, &coord.second}},
			{"traversals", &node_traversals},
			{"updates", &node_updates},
			{"leaves", &leaves},
			{"node-stats", &node_stats},
			{"rave-stats", &rave}}))
	{
		throw std::logic_error("mcts::deserialize_node(): invalid tree structure!");
	}

	ptr->color = color;
	ptr->coord = coord;
	ptr->traversals = node_traversals;
	ptr->updates = node_updates;

	if (leaves) {
		for (auto leaf : leaves->entities()) {
			coordinate leaf_coord;
			point::color leaf_color;

			if (!anserial::destructure(leaf,
				{"node",
					{"color", (uint32_t*)&leaf_color},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}}}))
			{
				std::cerr << "mcts::deserialize_node(): couldn't load leaf coordinate"
					<< std::endl;
				continue;
			}

			// XXX: point::color::Empty since it should be overwritten
			//      in the next level down
			ptr->new_node(leaf_coord, leaf_color);
			deserialize_node(leaf, ptr->leaves[leaf_coord].get());
		}
	}

	if (node_stats) {
		for (auto leaf : node_stats->entities()) {
			coordinate leaf_coord;
			stats leaf_stats;
			anserial::s_node *leaf_node;

			if (!anserial::destructure(leaf,
				{"leaf",
					{"traversals", &leaf_stats.traversals},
					{"wins", &leaf_stats.wins},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}},
					&leaf_node}))
			{
				std::cerr << "mcts::deserialize_node(): asdf should throw here"
					<< std::endl;
			}

			ptr->nodestats[leaf_coord] = leaf_stats;
		}
	}

#ifdef SERIALIZE_RAVE
	if (rave) {
		for (auto stat : rave->entities()) {
			coordinate leaf_coord;
			stats leaf_stats;

			if (!anserial::destructure(stat,
				{"stats",
					{"traversals", &leaf_stats.traversals},
					{"wins", &leaf_stats.wins},
					{"coordinate", {&leaf_coord.first, &leaf_coord.second}}}))
			{
				std::cerr << "mcts::deserialize_node(): asdf rave stats"
					<< std::endl;

				continue;
			}

			ptr->rave[leaf_coord] = leaf_stats;
		}
	}
#endif
	*/

	return nullptr;
}

std::unique_ptr<mcts> bdg::deserializeTree(kj::ArrayPtr<kj::byte> array) {
	auto ret = std::make_unique<mcts>();

	kj::ArrayInputStream arr(array);
	::capnp::PackedMessageReader message(arr);
	UpdateTree::Reader reader = message.getRoot<UpdateTree>();

	return ret;
}

void bdg::serializeTree(kj::BufferedOutputStream& stream, mcts *tree, unsigned since) {
	::capnp::MallocMessageBuilder message;

	UpdateTree::Builder builder = message.initRoot<UpdateTree>();
	auto rootBuilder = builder.getRoot();
	serialize_node(rootBuilder, tree->root.get(), since);

	writePackedMessage(stream, message);
}


// TODO
#if 0
std::vector<uint32_t> serialize(board *state, uint32_t since) {
	/*
	anserial::serializer ret;

	uint32_t data_top = ret.default_layout();
	uint32_t tree_top = ret.add_entities(data_top, {"budgie-tree"});
	ret.add_entities(tree_top, {"id", id});
	ret.add_entities(tree_top, {"updates", updates});
	state->serialize(ret, ret.add_entities(tree_top, {"board"}));
	uint32_t nodes = ret.add_entities(tree_top, {"nodes"});
	serialize_node(ret, nodes, root.get(), since);
	ret.add_symtab(0);

	return ret.serialize();
	*/

	return {};
}

void mcts::deserialize(std::vector<uint32_t>& serialized, board *state) {
	/*
	anserial::s_node *nodes;
	anserial::s_node *board_nodes;

	anserial::deserializer der(serialized);
	anserial::s_tree tree(&der);

	if (!anserial::destructure(tree.data(),
		{{"budgie-tree",
			{"id", &id},
			{"updates", &updates},
			{"board", &board_nodes},
			{"nodes", &nodes}}}))
	{
		tree.dump_nodes();
		throw std::logic_error("mcts::deserialize(): invalid tree structure!");
	}

	state->deserialize(board_nodes);
	//tree.dump_nodes(nodes);
	
	if (nodes) {
		deserialize_node(nodes, root.get());
	}
	*/
}

void board::serialize(anserial::serializer& ser, uint32_t parent) {
	uint32_t datas = ser.add_entities(parent,
		{"board",
			{"dimension", dimension},
			// TODO: need to add signed int, double types to anserial
			{"komi", komi},
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

#endif
