#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdio.h>

#ifndef MAXSIZE
// TODO: constants header, so things like this can be baked at compile time
//       (so 9x9 can be built with more efficient structures,
//        some checks/calculations can be omitted if min/max are the same, etc)
static const unsigned MAXSIZE = 19;
static const unsigned MINSIZE = 5;
#endif

namespace mcts_thing {

class josekiDB {
	public:
		// TODO: use real coordinate type here, avoid redefining
		using coordinate = std::pair<unsigned, unsigned>;

		// seperate maps for each board size
		std::map<uint64_t, std::set<coordinate>> sizes[MAXSIZE];

		void loadAll(std::string filenames) {
			size_t start = 0;
			size_t end = filenames.find(",");

			while (start < filenames.length()) {
				std::string tmp = filenames.substr(start, end - start);
				load(tmp);

				if (end == std::string::npos)
					break;

				start = end + 1;
				end = filenames.find(",", start);
			}
		}

		void load(std::string filename) {
			printf("Loading: %s\n", filename.c_str());

			FILE *db = fopen(filename.c_str(), "r");

			if (!db) {
				fprintf(stderr, "%s: couldn't open file\n", filename.c_str());
				return;
			}

			uint32_t magic;
			uint32_t version;
			uint32_t boardsize;

			fread(&magic, 4, 1, db);
			fread(&version, 4, 1, db);
			fread(&boardsize, 4, 1, db);

			if (magic != 0xabadc0de) {
				fprintf(stderr, "%s: invalid magic number, not a database file\n", filename.c_str());
				return;
			}

			if (boardsize > MAXSIZE) {
				fprintf(stderr, "%s: board size is larger than compiled max: %u > %u\n", filename.c_str(), boardsize, MAXSIZE);
				return;
			}

			while (!feof(db)) {
				uint64_t hash;
				fread(&hash, 8, 1, db);

				while (true) {
					uint8_t x, y;

					fread(&x, 1, 1, db);
					if (x == 0)
						break;
					fread(&y, 1, 1, db);

					sizes[boardsize-1][hash].insert({x, y});
					//printf("Adding in %u: 0x%016lx: %u, %u, %lu\n", boardsize-1, hash, x, y, sizes[boardsize-1].size());

				}
			}

			fclose(db);
			fprintf(stderr, "%s: loaded successfully\n", filename.c_str());
		}

		const std::set<coordinate>& search(size_t size, uint64_t hash) {
			static const std::set<coordinate> nullret = {};

			if (size > MAXSIZE) return nullret;

			//printf("search: %lu, %016lx\n", size-1, hash);

			auto& hashes = sizes[size-1];
			auto it = hashes.find(hash);

			//printf("search: %lu, %016lx, size: %lu, hashes: %lu\n", size-1, hash, hashes.size(), sizes[size-1].size());

			if (it != hashes.end()) {
				//puts("got here");
				return it->second;

			} else  {
				return nullret;
			}
		}
};

// namespace mcts_thing
}
