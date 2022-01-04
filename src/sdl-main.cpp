#include <budgie/budgie.hpp>
#include <budgie/args_parser.hpp>
#include <budgie/game.hpp>
#include <budgie/mcts.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <time.h>

namespace mcts_thing {

class gui_state {
	public:
		gui_state(budgie& b);
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
		void draw_predicted(unsigned x, unsigned y, unsigned width);
		void draw_circle(unsigned x, unsigned y, int radius);
		void draw_text(unsigned x, unsigned y, std::string str);

	private:
		enum modes {
			Statistics,
			Ownership,
			Mcts,
			Traversals,
			Rave,
			Criticality,
			Score,
			End,
		};

		SDL_Window   *window;
		SDL_Renderer *renderer;
		TTF_Font     *font;
		bool running;
		enum modes mode;

		/*
		board game = board(9);
		std::unique_ptr<mcts> search_tree;
		*/
		budgie& bot;
		std::vector<float> winrates;
		bool relative_stats = false;
};

gui_state::gui_state(budgie& b) : bot(b) {
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

	font = TTF_OpenFont("assets/fonts/LiberationSans-Regular.ttf", 16);

	if (!font) {
		throw "TTF_OpenFont()";
	}

	/*
	// XXX: need to make some sort of AI instance class
	pattern_dbptr db = pattern_dbptr(new pattern_db("patterns.txt"));
	tree_policy *tree_pol = new uct_rave_tree_policy(db);
	playout_policy *playout_pol = new random_playout(db);
	//playout_policy *playout_pol = new local_weighted_playout(db);

	search_tree = std::unique_ptr<mcts>(new mcts(tree_pol, playout_pol));
	*/
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

	double min_score = bot.boardsize*bot.boardsize;
	double max_score = -bot.boardsize*bot.boardsize;

	double min_mcts = 1.0;
	double max_mcts = 0;

	for (int dx = 1; dx <= bot.boardsize; dx++) {
		for (int dy = 1; dy <= bot.boardsize; dy++) {
			coordinate coord = {dx, dy};
			unsigned hash = coord_hash_v2(coord);
			if (bot.game.get_coordinate_unsafe(dx, dy) != point::color::Empty)
				continue;

			mcts_node *leaf = bot.tree->root->leaves[hash];
			//auto& stat = bot.tree->root->nodestats[hash];

			double traversals = (leaf == nullptr)
				? 0
				: leaf->traversals / (1.*bot.tree->root->traversals);

			double rave_est = bot.tree->root->rave[hash].win_rate();
			//double crit_est = (*bot.tree->root->criticality)[coord].win_rate();
			double crit_est = (*bot.tree->root->criticality)[hash].win_rate();
			float score_est = (bot.tree->root->expected_score[hash] / (bot.tree->root->traversals));
			double mcts_est = leaf? leaf->win_rate() : 0;

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

			if (score_est < min_score) {
				min_score = score_est;
			}

			if (score_est > max_score) {
				max_score = score_est;
			}

			if (mcts_est > max_mcts) {
				max_mcts = mcts_est;
			}

			if (mcts_est < min_mcts) {
				min_mcts = mcts_est;
			}
		}
	}

	for (int dx = 1; dx <= bot.boardsize; dx++) {
		for (int dy = 1; dy <= bot.boardsize; dy++) {
			if (bot.game.get_coordinate_unsafe(dx, dy) != point::color::Empty)
				continue;

			coordinate foo = {dx, dy};
			unsigned hash = coord_hash_v2(foo);
			mcts_node *leaf = bot.tree->root->leaves[hash];
			SDL_Rect rect;

			double meh = (1.*width) / (bot.game.dimension - 1);
			double asdf = (x + 1) - (meh/2);

			unsigned off = 0x20;
			unsigned range = (0xff - off);

			rect.x = asdf + (foo.first  - 1) * meh;
			rect.y = asdf + (foo.second - 1) * meh;
			rect.h = rect.w = meh + 1;

			unsigned r=0, g=0, b=0;

			if (mode == modes::Traversals || mode == modes::Statistics) {
				if (leaf) {
					// TODO: config option to toggle ownership/statistic heatmaps
					double traversals = leaf->traversals / (1.*bot.tree->root->traversals);
					if (relative_stats) {
						traversals = (traversals - min_traversals) / (max_traversals - min_traversals);
					}
					b = off + range * traversals;
				}
			}

			if (mode == modes::Rave || mode == modes::Statistics) {
				double rave_est = bot.tree->root->rave[hash].win_rate();
				if (relative_stats) {
					rave_est = (rave_est - min_rave) / (max_rave - min_rave);
				}
				g = off + range * rave_est;
			}

			if (mode == modes::Criticality || mode == modes::Statistics) {
				//double crit_est = (*bot.tree->root->criticality)[foo].win_rate();
				double crit_est = (*bot.tree->root->criticality)[hash].win_rate();
				if (relative_stats) {
					crit_est = (crit_est - min_crit) / (max_crit - min_crit);
				}
				r = off + range * crit_est;
			}

			if (mode == modes::Mcts) {
				mcts_node *leaf = bot.tree->root->leaves[hash];
				//auto& stat = bot.tree->root->nodestats[hash];
				double mcts_est = leaf? leaf->win_rate() : 0;

				if (relative_stats) {
					mcts_est = (mcts_est - min_mcts) / (max_mcts - min_mcts);
				}

				r = off + range * mcts_est;
				b = off + range * mcts_est;
				//g = off;
				//g = off + range * bot.tree->root->nodestats[hash].traversals /
				//	(1.*bot.tree->root->traversals);
			}

			if (mode == modes::Ownership) {
				auto& x = (*bot.tree->root->criticality)[hash];
				//auto& x = (*bot.tree->root->criticality)[foo];
				r = off + range * (x.black_owns / (1. * x.traversals));
				g = off + range * (x.white_owns / (1. * x.traversals));
				b = off;
			}

			if (mode == modes::Score) {
				unsigned count = bot.tree->root->score_counts[hash];
				if (count == 0) {
					r = g = b = off;
				} else {
					float scoreest = float(bot.tree->root->expected_score[hash]) / count;
					scoreest /= (bot.boardsize * bot.boardsize);
					bool lt = scoreest > 0;

					if (relative_stats) {
						scoreest = (scoreest - min_score) / (max_score - min_score);
					} else {
						scoreest = fabs(scoreest);
					}

					//auto& x = (*bot.tree->root->criticality)[foo];
					if (lt) r = off + range*scoreest;
					else r = off;
					//r = off;
					g = off + range*scoreest;
					if (!lt) b = off + range*scoreest;
					else b = off;
				}
			}

			// gamma correction
			r = 0xff * pow(r/255.0, 1/2.2);
			g = 0xff * pow(g/255.0, 1/2.2);
			b = 0xff * pow(b/255.0, 1/2.2);

			SDL_SetRenderDrawColor(renderer, r, g, b, 0);
			SDL_RenderFillRect(renderer, &rect);
		}
	}
}

void gui_state::draw_grid(unsigned x, unsigned y, unsigned width) {
	for (unsigned i = 0; i < bot.game.dimension; i++) {
		SDL_Rect r;

		r.h = 1;
		//r.w = foo * (game.dimension - 1);
		r.w = width;
		r.x = x;
		r.y = y + ((1.*width) / (bot.game.dimension - 1) * i);

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
	double meh = (1.*width) / (bot.game.dimension - 1);
	double blarg = meh - (meh/12);
	double x_off = (x + 1) - (blarg/2);
	double y_off = (y + 1) - (blarg/2);

	for (unsigned y = 1; y <= bot.game.dimension; y++) {
		for (unsigned x = 1; x <= bot.game.dimension; x++) {
			coordinate foo = {x, y};
			SDL_Rect r;

			r.x = x_off + (x - 1) * meh;
			r.y = y_off + (y - 1) * meh;
			r.h = r.w = blarg;

			switch (bot.game.get_coordinate(foo)) {
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

void gui_state::draw_predicted(unsigned x, unsigned y, unsigned width) {
	double meh = (1.*width) / (bot.game.dimension - 1);
	double blarg = meh - (meh/12);
	double x_off = (x + 1) - (blarg/3);
	double y_off = (y + 1) - (blarg/3);

	unsigned i = 1;
	for (mcts_node *ptr = bot.tree->root.get(); ptr; i++) {
		coordinate coord = ptr->best_move();
		SDL_Rect r;

		if (coord == coordinate {0, 0}) {
			break;
		}

		r.x = x_off + (coord.first-1) * meh;
		r.y = y_off + (coord.second-1) * meh;
		r.h = r.w = blarg;

		switch (ptr->color) {
			// Colors are reversed here, black nodes track possible moves for white
			// and vice versa
			case point::color::White:
				SDL_SetRenderDrawColor(renderer, 0x30, 0x30, 0x30, 0);
				draw_circle(r.x, r.y, r.h/3);
				draw_text(r.x + r.h/2, r.y+r.h/2, std::to_string(i));
				break;

			case point::color::Black:
				SDL_SetRenderDrawColor(renderer, 0xd0, 0xd0, 0xd0, 0);
				draw_circle(r.x, r.y, r.h/3);
				draw_text(r.x + r.h/2, r.y+r.h/2, std::to_string(i));
				break;

			default:
				break;
		}

		//printf("got here\n");
		ptr = ptr->leaves[coord_hash_v2(coord)];
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
	draw_predicted(x + padding, y + padding, sub_width);
}

void gui_state::draw_stats(unsigned x, unsigned y, unsigned width, unsigned height) {
	SDL_Rect r;
	r.w = width;
	r.h = height;
	r.x = x;
	r.y = y;

	SDL_SetRenderDrawColor(renderer, 0x30, 0x30, 0x30, 0);
	SDL_RenderFillRect(renderer, &r);

	float score_black = 0, score_white = bot.game.komi;

	for (unsigned x = 1; x <= bot.game.dimension; x++) {
		for (unsigned y = 1; y <= bot.game.dimension; y++) {
			//auto& crit = (*bot.tree->root->criticality)[coordinate(x, y)];
			auto& crit = (*bot.tree->root->criticality)[(x << 5) | y];

			score_black += crit.black_owns / (1. * crit.traversals);
			score_white += crit.white_owns / (1. * crit.traversals);
		}
	}

	draw_text(x, y, std::to_string(bot.tree->root->traversals) + " playouts");
	draw_text(x, y+16, "estimated score:");
	draw_text(x+16, y+32, "black: " + std::to_string(score_black));
	draw_text(x+16, y+48, "white: " + std::to_string(score_white));

	mcts_node *leaf = bot.tree->root->leaves[0];
	double passpct = leaf? leaf->win_rate() : 0;
	draw_text(x + 16, y+64, "pass%: " + std::to_string(passpct));

	draw_text(x, y + height/2, "Winrate:");
	draw_text(x + width/2 - 28, y + height/2 + 16, "white");
	draw_text(x + width/2 - 28, y + height-16, "black");

	SDL_SetRenderDrawColor(renderer, 0xd0, 0xd0, 0xd0, 0);
	for (unsigned i = 0; i < winrates.size(); i++) {
		SDL_Rect rect;
		rect.x = x + 16 + 4*(i+1);
		rect.y = y + height/2 + 32;
		rect.h = winrates[i] * (height/2 - 48);
		rect.w = 4;

		SDL_RenderFillRect(renderer, &rect);
	}

	// line at 50% winrate for each side
	SDL_SetRenderDrawColor(renderer, 0xa0, 0xc0, 0xa0, 0);
	SDL_RenderDrawLine(renderer,
		x+16,       y+height/2+32 + height/4 - 24,
		x+width-16, y+height/2+32 + height/4 - 24);
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

		if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
				case SDLK_TAB:
					mode = (enum modes)(((int)mode + 1) % (int)(modes::End));
					break;

				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					relative_stats = !relative_stats;
					break;

				case SDLK_ESCAPE:
					// TODO: reset the bot
					break;

				default:
					break;
			}
		}
	}
}

#include <unistd.h>
int gui_state::run(void) {
	running = true;

	while (running) {
		bot.tree->reset();
		//bot.reset();
		coordinate coord;

		for (int i = 100; running && i <= bot.playouts; i += 100) {
			coord = bot.tree->do_search(&bot.game, bot.pool, i);
			handle_events();
			redraw();
			usleep(166700);
		}

		winrates.push_back((bot.game.current_player == point::color::Black)
			? 1.0 - bot.tree->win_rate(coord)
			: bot.tree->win_rate(coord));

		//bot.game.make_move(coord);
		bot.make_move(coord);
	}

	return 0;
}

// namespace mcts_thing
}

using namespace mcts_thing;

int main(int argc, char *argv[]) {
	try {
		srand(time(NULL));
		args_parser args(argc, argv, budgie_options);
		budgie bot(args.options);
		gui_state gui(bot);
		return gui.run();

	} catch (const char *e) {
		std::cerr << argv[0] << ": error: " << e << std::endl;
		return 1;
	}
}
