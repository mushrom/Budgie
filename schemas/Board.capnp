@0xf48d4cc0012a2b80;

enum Color {
	empty @0;
	black @1;
	white @2;
}

struct Coordinate {
	x @0 : UInt8;
	y @1 : UInt8;
}

struct Move {
	coord @0 : Coordinate;
	color @1 : Color;
}

struct Board {
	dimension @0 : UInt8;
	id        @1 : UInt32;

	points @2 : List(Color);
	moves  @3 : List(Move);
}
