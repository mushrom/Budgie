#pragma once

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <stdint.h>
#include <budgie/mcts.hpp>
#include <kj/common.h>

namespace mcts_thing {

std::unique_ptr<mcts> deserializeTree(kj::ArrayPtr<kj::byte> array);
void serializeTree(kj::BufferedOutputStream& stream, mcts *tree, unsigned since);

// namespace mcts_thing
}
