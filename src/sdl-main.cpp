#include <mcts-gb/args_parser.hpp>
#include <mcts-gb/game.hpp>
#include <mcts-gb/mcts.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>

namespace mcts_thing {

class gui_state {
	public:
		gui_state();
		~gui_state();
		int run(void);

		void clear(void);
		void present(void);
		void redraw(void);

		void draw_overlays(void);
		void draw_grid(void);
		void draw_stones(void);

	private:
		SDL_Window   *window;
		SDL_Renderer *renderer;
		TTF_Font     *font;
		bool running;

		board game = board(9);
		std::unique_ptr<mcts> search_tree;
};

gui_state::gui_state() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		throw "SDL_Init()";
	}

	if (TTF_Init() == -1) {
		throw "TTF_Init()";
	}

	window = SDL_CreateWindow("Budgie SDL",
	                          SDL_WINDOWPOS_UNDEFINED,
	                          SDL_WINDOWPOS_UNDEFINED,
	                          1024, 600,
	                          SDL_WINDOW_SHOWN);

	if (!window) {
		throw "SDL_CreateWindow()";
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer) {
		throw "SDL_CreateRenderer()";
	}

	font = TTF_OpenFont("assets/fonts/LiberationSans-Regular.ttf", 12);

	if (!font) {
		throw "TTF_OpenFont()";
	}

	// XXX: need to make some sort of AI instance class
	pattern_dbptr db = pattern_dbptr(new pattern_db("patterns.txt"));
	tree_policy *tree_pol = new uct_rave_tree_policy(db);
	playout_policy *playout_pol = new random_playout(db);

	search_tree = std::unique_ptr<mcts>(new mcts(tree_pol, playout_pol));
}

gui_state::~gui_state() {
	TTF_CloseFont(font);
	font = nullptr;
	TTF_Quit();
	SDL_Quit();

	// TODO: do we have to close the window here too?
	window = nullptr;
}

void gui_state::clear(void) {
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0x80, 0xb0, 0x80, 0);
	SDL_RenderFillRect(renderer, NULL);
}

void gui_state::draw_overlays(void) {
	double min_traversals = 1.0;
	double max_traversals = 0;

	double min_rave = 1.0;
	double max_rave = 0;

	double min_crit = 1.0;
	double max_crit = 0;

	for (const auto& move : search_tree->root->leaves) {
		double traversals = move.second->traversals / (1.*search_tree->root->traversals);
		double rave_est = (*search_tree->root->rave)[move.first].win_rate();
		double crit_est = (*search_tree->root->criticality)[move.first].win_rate();

		if (traversals < min_traversals) {
			min_traversals = traversals;
		}

		if (traversals > max_traversals) {
			max_traversals = traversals;
		}

		if (rave_est < min_rave) {
			min_rave = rave_est;
		}

		if (rave_est > max_rave) {
			max_rave = rave_est;
		}

		if (crit_est < min_crit) {
			min_crit = crit_est;
		}

		if (crit_est > max_crit) {
			max_crit = crit_est;
		}
	}

	for (const auto& move : search_tree->root->leaves) {
		coordinate foo = move.first;
		SDL_Rect rect;

		double meh = 500.0 / (game.dimension - 1);
		double asdf = 51 - (meh/2);

		rect.x = asdf + (foo.first  - 1) * meh;
		rect.y = asdf + (foo.second - 1) * meh;
		rect.h = rect.w = meh;

		double traversals = move.second->traversals / (1.*search_tree->root->traversals);

		traversals = (traversals - min_traversals) / (max_traversals - min_traversals);
		unsigned b = 0xff - 0x80 * traversals;

		double rave_est = (*search_tree->root->rave)[foo].win_rate();
		rave_est = (rave_est - min_rave) / (max_rave - min_rave);
		unsigned g = 0xff - 0x80 * rave_est;

		double crit_est = (*search_tree->root->criticality)[foo].win_rate();
		crit_est = (crit_est - min_crit) / (max_crit - min_crit);
		unsigned r = 0xff - 0x80 * crit_est;

		SDL_SetRenderDrawColor(renderer, r, g, b, 0);
		//SDL_SetRenderDrawColor(renderer, 0xdd, 0xbb, b, 0);
		SDL_RenderFillRect(renderer, &rect);
	}

}

void gui_state::draw_grid(void) {
	for (unsigned i = 0; i < game.dimension; i++) {
		SDL_Rect r;

		r.h = 2;
		//r.w = foo * (game.dimension - 1);
		r.w = 500;
		r.x = 50;
		r.y = 50 + (500.0 / (game.dimension - 1) * i);

		SDL_SetRenderDrawColor(renderer, 0x40, 0x40, 0x40, 0);
		SDL_RenderFillRect(renderer, &r);

		auto k = r.x;
		r.x = r.y;
		r.y = k;

		k = r.h;
		r.h = r.w;
		r.w = k;
		SDL_RenderFillRect(renderer, &r);
	}


}

void gui_state::draw_stones(void) {
	for (unsigned y = 1; y <= game.dimension; y++) {
		for (unsigned x = 1; x <= game.dimension; x++) {
			coordinate foo = {x, y};
			SDL_Rect r;

			double meh = 500.0 / (game.dimension - 1);
			double blarg = meh - (meh/8);
			double asdf = 51 - (blarg/2);

			r.y = asdf + (y - 1) * meh;
			r.x = asdf + (x - 1) * meh;
			r.h = r.w = blarg;

			switch (game.get_coordinate(foo)) {
				case point::color::Black:
					SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0);
					SDL_RenderFillRect(renderer, &r);
					break;

				case point::color::White:
					SDL_SetRenderDrawColor(renderer, 0xf0, 0xf0, 0xf0, 0);
					SDL_RenderFillRect(renderer, &r);
					break;

				default:
					break;
			}
		}
	}
}

void gui_state::redraw(void) {
	clear();

	// TODO: dynamic sizes

	SDL_Rect r;
	r.h = 550;
	r.w = 550;
	r.x = 25;
	r.y = 25;

	SDL_SetRenderDrawColor(renderer, 0xdd, 0xbb, 0x66, 0);
	SDL_RenderFillRect(renderer, &r);

	draw_overlays();
	draw_grid();
	draw_stones();

	present();
}

void gui_state::present(void) {
	SDL_RenderPresent(renderer);
}

int gui_state::run(void) {
	running = true;

	while (running) {
		search_tree->reset();
		coordinate coord;


		for (unsigned i = 250; i <= 10000; i += 250) {
			coord = search_tree->do_search(&game, i);
			redraw();
		}

		game.make_move(coord);

		while (true) {
			SDL_Event e;
			SDL_WaitEvent(&e);

			if (e.type == SDL_QUIT) {
				running = false;
				break;
			}

			if (e.type == SDL_KEYDOWN) {
				break;
			}
		}
	}

	return 0;
}

// namespace mcts_thing
}

using namespace mcts_thing;

int main(int argc, char *argv[]) {
	try {
		gui_state gui;
		return gui.run();

	} catch (const char *e) {
		std::cerr << argv[0] << ": error: " << e << std::endl;
		return 1;
	}
}
