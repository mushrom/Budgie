@0xab9754305e990b4e;

using Coordinate = import "Board.capnp".Coordinate;

struct Node {
	wins             @0 : UInt32;	
	traversals       @1 : UInt32;
	priorWins        @2 : UInt32;
	priorTraversals  @3 : UInt32;

	coord  @4 : Coordinate;
	leaves @5 : List(Node);
}
