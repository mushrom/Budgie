@0xda0b7055017a2e79;

using Node  = import "Node.capnp".Node;
using Board = import "Board.capnp".Board;

struct UpdateTree {
	root    @0 : Node;
	board   @1 : Board;
	updates @2 : UInt32;
}

