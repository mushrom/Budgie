#pragma once

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <stdint.h>
#include <budgie/mcts.hpp>
#include <kj/common.h>

namespace bdg {

std::unique_ptr<mcts> deserializeTree(kj::ArrayPtr<kj::byte> array);
void serializeTree(kj::BufferedOutputStream& stream, mcts *tree, unsigned since);

// namespace bdg
}
