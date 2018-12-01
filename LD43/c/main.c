#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "stretchy_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define UI_INGRED_LEFT    35
#define UI_INGRED_TOP     486
#define UI_INGRED_SIZE    98
#define UI_INGRED_SPACING 20

typedef struct {
	int x;
	int y;
} Vector2;

Vector2 make_Vector2(int x, int y)
{
	return (Vector2) { x, y };
}

SDL_Rect make_SDL_Rect(int x, int y, int w, int h)
{
	return (SDL_Rect) { x, y, w, h };
}

typedef enum {
	INGRED_NONE = -1,
	INGRED_GOLDFISH,
	INGRED_PHEASANT,
	INGRED_TURKEY,
	INGRED_LAMB,
	INGRED_GOAT,
	INGRED_ISAAC,
	INGRED_COUNT,
} Ingredient;

char * ingredient_texture_paths[INGRED_COUNT] = {
	"resources/goldfish.png",
	"resources/pheasant.png",
	"resources/turkey.png",
	"resources/lamb.png",
	"resources/goat.png",
	"resources/isaac.png",
};

typedef struct {
	SDL_Texture * bg_texture;
	SDL_Texture * ingredient_textures[INGRED_COUNT];
	Ingredient transient_ingredient;
} State_Playing;

enum Game_State {
	STATE_PLAYING,
};

typedef struct {
	enum Game_State type;
	union {
		State_Playing state_playing;
	};
} Game_State;

typedef struct {
	SDL_Renderer * renderer;
} SDL_State;

SDL_Rect ingredient_box(Ingredient ingredient)
{
	return make_SDL_Rect(UI_INGRED_LEFT + (UI_INGRED_SIZE + UI_INGRED_SPACING) * ingredient,
						 UI_INGRED_TOP, UI_INGRED_SIZE, UI_INGRED_SIZE);
}

static SDL_State sdl_state;

SDL_Texture * load_texture_from_path(char * path, SDL_TextureAccess access)
{
	int w, h, n;
	unsigned char * data = stbi_load(path, &w, &h, &n, 4);
	SDL_Surface * surface = SDL_CreateRGBSurfaceFrom(data, w, h, 4 * 8, w * 4,
													 0x000000ff, 0x0000ff00,
													 0x00ff0000, 0xff000000);
	SDL_Texture * texture = SDL_CreateTextureFromSurface(sdl_state.renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}

void state_playing_init(State_Playing * state)
{
	state->bg_texture = load_texture_from_path("resources/bg.png", SDL_TEXTUREACCESS_STATIC);
	state->transient_ingredient = INGRED_NONE;
	for (int i = 0; i < INGRED_COUNT; i++) {
		state->ingredient_textures[i] =
			load_texture_from_path(ingredient_texture_paths[i], SDL_TEXTUREACCESS_STATIC);
	}
}

Ingredient generator_click(Vector2 pos)
{
	for (int i = 0; i < INGRED_COUNT; i++) {
		SDL_Point point = (SDL_Point) { pos.x, pos.y };
		SDL_Rect rect = ingredient_box(i);
		if (SDL_PointInRect(&point, &rect)) {
			return i;
		}
	}
	return INGRED_NONE;
}

void state_playing_mbdown(State_Playing * state, Vector2 mpos)
{
	// Check ingredient generators
	if (generator_click(mpos) != INGRED_NONE) {
		Ingredient new_ingredient = generator_click(mpos);
		state->transient_ingredient = new_ingredient;
	}
}

void state_playing_mbup(State_Playing * state, Vector2 mpos)
{
	state->transient_ingredient = INGRED_NONE;
}

void state_playing_event(State_Playing * state, SDL_Event event)
{
	switch (event.type) {
	case SDL_MOUSEBUTTONDOWN:
		state_playing_mbdown(state, make_Vector2(event.button.x, event.button.y));
		break;
	case SDL_MOUSEBUTTONUP:
		state_playing_mbup(state, make_Vector2(event.button.x, event.button.y));
		break;
	}
}

void state_playing_update(State_Playing * state)
{
	
}

void state_playing_render(State_Playing * state)
{
	// Background
	SDL_RenderCopy(sdl_state.renderer, state->bg_texture, NULL, NULL);
	// Generators
	for (int i = 0; i < INGRED_COUNT; i++) {
		SDL_Rect rect = ingredient_box(i);
		SDL_RenderCopy(sdl_state.renderer,
					   state->ingredient_textures[i],
					   NULL, &rect);
	}
	// Transient ingredient
	if (state->transient_ingredient != INGRED_NONE) {
		int mx, my;
		SDL_GetMouseState(&mx, &my);
		SDL_Rect rect = make_SDL_Rect(mx - UI_INGRED_SIZE / 2, my - UI_INGRED_SIZE / 2,
									  UI_INGRED_SIZE, UI_INGRED_SIZE);
		SDL_RenderCopy(sdl_state.renderer,
					   state->ingredient_textures[state->transient_ingredient],
					   NULL, &rect);
	}
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window * window = SDL_CreateWindow(
		"LD43",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN);
	SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);
	sdl_state.renderer = renderer;
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	Game_State * game_state_stack = NULL;

	{
		State_Playing state;
		sb_push(game_state_stack,
			((Game_State) { .type = STATE_PLAYING, .state_playing = state }));
	}

	for (int i = 0; i < sb_count(game_state_stack); i++) {
		Game_State * state = game_state_stack + i;
		switch (state->type) {
		case STATE_PLAYING:
			state_playing_init(&(state->state_playing));
			break;
		default:
			assert(false);
			break;
		}
	}
	
	SDL_Event event;
	bool running = true;
	while (running && sb_count(game_state_stack) > 0) {
		Game_State * game_state = &(sb_last(game_state_stack));
		
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				running = false;
			} else {
				switch (game_state->type) {
				case STATE_PLAYING:
					state_playing_event(&(game_state->state_playing), event);
					break;
				default:
					assert(false);
					break;
				}
			}
		}

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
		SDL_RenderClear(renderer);
		
		switch (sb_last(game_state_stack).type) {
		case STATE_PLAYING:
			state_playing_update(&(game_state->state_playing));
			state_playing_render(&(game_state->state_playing));
			break;
		default:
			assert(false);
			break;
		}
		
		SDL_RenderPresent(renderer);
	}
	
	return 0;
}
