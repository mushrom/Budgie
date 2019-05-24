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
		void handle_events(void);

		void clear(void);
		void present(void);
		void redraw(void);

		void draw_board(unsigned x, unsigned y, unsigned width);
		void draw_stats(unsigned x, unsigned y, unsigned width, unsigned height);

		void draw_overlays(unsigned x, unsigned y, unsigned width);
		void draw_grid(unsigned x, unsigned y, unsigned width);
		void draw_stones(unsigned x, unsigned y, unsigned width);
		void draw_circle(unsigned x, unsigned y, int radius);
		void draw_text(unsigned x, unsigned y, std::string str);

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
	                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

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
	//playout_policy *playout_pol = new random_playout(db);
	playout_policy *playout_pol = new local_weighted_playout(db);

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

void gui_state::draw_overlays(unsigned x, unsigned y, unsigned width) {
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

		double meh = (1.*width) / (game.dimension - 1);
		double asdf = (x + 1) - (meh/2);

		unsigned off = 0x40;
		unsigned range = (0xff - off);

		rect.x = asdf + (foo.first  - 1) * meh;
		rect.y = asdf + (foo.second - 1) * meh;
		rect.h = rect.w = meh + 1;

		double traversals = move.second->traversals / (1.*search_tree->root->traversals);

		traversals = (traversals - min_traversals) / (max_traversals - min_traversals);
		unsigned b = off + range * traversals;

		double rave_est = (*search_tree->root->rave)[foo].win_rate();
		rave_est = (rave_est - min_rave) / (max_rave - min_rave);
		unsigned g = off + range * rave_est;

		double crit_est = (*search_tree->root->criticality)[foo].win_rate();
		crit_est = (crit_est - min_crit) / (max_crit - min_crit);
		unsigned r = off + range * crit_est;

		/*
		auto& x = (*search_tree->root->criticality)[foo];
		double r = off + range * (x.black_owns / (1. * x.traversals));
		double g = off + range * (x.white_owns / (1. * x.traversals));
		unsigned b = 0x66;
		*/

		SDL_SetRenderDrawColor(renderer, r, g, b, 0);
		SDL_RenderFillRect(renderer, &rect);
	}
}

void gui_state::draw_grid(unsigned x, unsigned y, unsigned width) {
	for (unsigned i = 0; i < game.dimension; i++) {
		SDL_Rect r;

		r.h = 1;
		//r.w = foo * (game.dimension - 1);
		r.w = width;
		r.x = x;
		r.y = y + ((1.*width) / (game.dimension - 1) * i);

		SDL_SetRenderDrawColor(renderer, 0x40, 0x40, 0x30, 0);
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

void gui_state::draw_stones(unsigned x, unsigned y, unsigned width) {
	double meh = (1.*width) / (game.dimension - 1);
	double blarg = meh - (meh/12);
	double x_off = (x + 1) - (blarg/2);
	double y_off = (y + 1) - (blarg/2);

	for (unsigned y = 1; y <= game.dimension; y++) {
		for (unsigned x = 1; x <= game.dimension; x++) {
			coordinate foo = {x, y};
			SDL_Rect r;

			r.x = x_off + (x - 1) * meh;
			r.y = y_off + (y - 1) * meh;
			r.h = r.w = blarg;

			switch (game.get_coordinate(foo)) {
				case point::color::Black:
					SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0);
					draw_circle(r.x, r.y, r.h/2);
					break;

				case point::color::White:
					SDL_SetRenderDrawColor(renderer, 0xf8, 0xf8, 0xf8, 0);
					draw_circle(r.x, r.y, r.h/2);

					break;

				default:
					break;
			}
		}
	}
}

void gui_state::draw_circle(unsigned x, unsigned y, int radius) {
	for (int i = -radius; i <= radius; i++) {
		double a = i/(1.*radius);
		double c = radius * sqrt(1 - (a * a));

		int x1 = x + radius - c;
		int x2 = x + radius + c;
		int y1 = y + radius + i;

		SDL_RenderDrawLine(renderer, x1, y1, x2, y1);
	}
}

void gui_state::draw_text(unsigned x, unsigned y, std::string str) {
	SDL_Color color = {0xff, 0xff, 0xff};
	SDL_Surface *text_surface;

	if (!(text_surface = TTF_RenderText_Blended(font, str.c_str(), color))) {
		throw "TTF_RenderText_Blended()";
	}

	SDL_Texture *text = SDL_CreateTextureFromSurface(renderer, text_surface);
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = text_surface->w;
	rect.h = text_surface->h;

	SDL_RenderCopy(renderer, text, NULL, &rect);

	// free stuff
	SDL_DestroyTexture(text);
	SDL_FreeSurface(text_surface);
}

void gui_state::draw_board(unsigned x, unsigned y, unsigned width) {
	unsigned padding = 25;
	unsigned sub_width = width - (padding * 2);

	// draw blank rectangle
	SDL_Rect r;
	r.h = r.w = width;
	r.x = x;
	r.y = y;

	SDL_SetRenderDrawColor(renderer, 0xdd, 0xbb, 0x66, 0);
	SDL_RenderFillRect(renderer, &r);

	draw_overlays(x + padding, y + padding, sub_width);
	draw_grid(x + padding, y + padding, sub_width);
	draw_stones(x + padding, y + padding, sub_width);
}

void gui_state::draw_stats(unsigned x, unsigned y, unsigned width, unsigned height) {
	SDL_Rect r;
	r.w = width;
	r.h = height;
	r.x = x;
	r.y = y;

	SDL_SetRenderDrawColor(renderer, 0x30, 0x30, 0x30, 0);
	SDL_RenderFillRect(renderer, &r);

	draw_text(x, y, "testing");
	draw_text(x, y + height/2, "this");
}

void gui_state::redraw(void) {
	clear();

	int x, y;
	SDL_GetWindowSize(window, &x, &y);

	if (x < y) {
		unsigned padding = x / 20;
		unsigned width = x - 2*padding;

		draw_board(padding, padding, width);

		// TODO: draw stats below
		if (y - width > 4*padding) {
			draw_stats(padding, x, x - 2*padding, (y - width) - 3*padding);
		}
	}

	else {
		unsigned padding = y / 20;
		unsigned width = y - 2*padding;

		draw_board(padding, padding, width);

		// TODO: draw stats on right
		if (x - width > 4*padding) {
			draw_stats(y, padding, (x - width) - 3*padding, y - 2*padding);
		}
	}

	present();
}

void gui_state::present(void) {
	SDL_RenderPresent(renderer);
}

void gui_state::handle_events(void) {
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
			running = false;
			break;
		}

		/*
		   if (e.type == SDL_KEYDOWN) {
		   break;
		   }
		   */
	}
}

int gui_state::run(void) {
	running = true;

	while (running) {
		search_tree->reset();
		coordinate coord;

		for (unsigned i = 100; running && i <= 10000; i += 100) {
			coord = search_tree->do_search(&game, i);
			handle_events();
			redraw();
		}

		game.make_move(coord);
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
