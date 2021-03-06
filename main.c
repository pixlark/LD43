#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "stretchy_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 600

#define PI 3.14159265

// //
// Ingredients
#define UI_INGRED_X         89
#define UI_INGRED_Y        486
#define UI_INGRED_SIZE      98
#define UI_INGRED_SPACING   20
#define UI_GENERATOR_COUNT   6
// Fire
#define UI_FIRE_X       438
#define UI_FIRE_Y       177
#define UI_FIRE_W       200
#define UI_FIRE_H       279
#define UI_FIRE_FRAMES    2
#define UI_FIRE_FPS     0.5
#define UI_FIRE_SHELF_X 492
#define UI_FIRE_SHELF_Y 310
#define UI_FIRE_COUNT     2
#define UI_FIRE_SPACING  27
// Tables
#define UI_TABLE_COUNT   4
#define UI_TABLE_W     190
#define UI_TABLE_H     220
// Orders
#define UI_ORDER_SIZE     (UI_INGRED_SIZE / 2)
#define UI_ORDER_Y_OFFSET (UI_TABLE_H - UI_ORDER_SIZE)
// Trashcan
#define UI_TRASH_X    712
#define UI_TRASH_Y     37
#define UI_TRASH_SIZE 106
// Clock
#define UI_CLOCK_X      537
#define UI_CLOCK_Y       88
#define UI_CLOCK_RADIUS  39
// Main Menu
#define UI_PLAY_RECT ((SDL_Rect) {330, 143, 453, 141})
#define UI_QUIT_RECT ((SDL_Rect) {263, 313, 557, 260})

#define UI_SLIDER_X        148
#define UI_SLIDER_Y_TOP    127
#define UI_SLIDER_Y_BOTTOM 365
#define UI_SLIDER_SIZE      50

#define UI_DIFF_TEXT_X 115 
#define UI_DIFF_TEXT_Y 476
#define UI_FONT_SIZE    48

#define DEATH_TEXT_X   7
#define DEATH_TEXT_Y 535

#define MUSIC_SWITCH_RECT ((SDL_Rect) { 262, 544, 30, 30 });
#define SOUND_SWITCH_RECT ((SDL_Rect) { 306, 544, 30, 30 });

#define SWITCH_ESCAPE_BOX ((SDL_Rect) { 252, 534, 92, 51 });
// //

static float difficulty;
#define DIFFICULTY_MULT       2
#define SUB_BASE_MULT      0.95
#define SUB_MULT_DIV       20.0
#define MINIMUM_SPAWN_TIME  6.0
#define MST_DIV            1.00

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
	SOUND_TSS,
	SOUND_TSCH,
	SOUND_TABLED,
	SOUND_EAT_LOW,
	SOUND_EAT_MED,
	SOUND_EAT_HIGH,
	SOUND_THUNDER,
	SOUND_COUNT
} Sound;

char * sound_paths[SOUND_COUNT] = {
	"resources/tss.ogg",
	"resources/tsch.ogg",
	"resources/tabled.ogg",
	"resources/eat-low.ogg",
	"resources/eat-med.ogg",
	"resources/eat-high.ogg",
	"resources/thunder.ogg",
};

typedef enum {
	MUSIC_MENU,
	MUSIC_PLAYING,
	MUSIC_COUNT
} Music;

char * music_paths[MUSIC_COUNT] = {
	"resources/menu.ogg",
	"resources/music.ogg",
};

typedef struct {
	Mix_Chunk * sounds[SOUND_COUNT];
	Mix_Music * music[MUSIC_COUNT];
} Sound_State;

static Sound_State sound_state;

static TTF_Font * default_font;

void sound_init()
{
	for (int i = 0; i < SOUND_COUNT; i++) {
		sound_state.sounds[i] = Mix_LoadWAV(sound_paths[i]);
	}
	for (int i = 0; i < MUSIC_COUNT; i++) {
		sound_state.music[i] = Mix_LoadMUS(music_paths[i]);
	}
}

void play_music(Music music)
{
	Mix_PlayMusic(sound_state.music[music], -1);
}

void play_sound(Sound sound)
{
	Mix_PlayChannel(-1, sound_state.sounds[sound], 0);
}

typedef struct {
	SDL_Renderer * renderer;
	float delta_time;
	uint64_t last_count;
} SDL_State;

static SDL_State sdl_state;

#define COOK_TIME 3.0
typedef enum {
	INGRED_NONE = -1,
	
	INGRED_GOLDFISH,
	INGRED_PHEASANT,
	INGRED_TURKEY,
	INGRED_LAMB,
	INGRED_GOAT,
	INGRED_ISAAC,
	INGRED_UNCOOKED_COUNT,
	
	INGRED_GOLDFISH_BURNT = INGRED_UNCOOKED_COUNT,
	INGRED_PHEASANT_BURNT,
	INGRED_TURKEY_BURNT,
	INGRED_LAMB_BURNT,
	INGRED_GOAT_BURNT,
	INGRED_ISAAC_BURNT,
	INGRED_COUNT,
} Ingredient;

char * ingredient_texture_paths[INGRED_COUNT] = {
	"resources/goldfish.png",
	"resources/pheasant.png",
	"resources/turkey.png",
	"resources/lamb.png",
	"resources/goat.png",
	"resources/isaac.png",
	"resources/goldfish-burnt.png",
	"resources/pheasant-burnt.png",
	"resources/turkey-burnt.png",
	"resources/lamb-burnt.png",
	"resources/goat-burnt.png",
	"resources/isaac-burnt.png",
};

typedef enum {
	GOD_NONE = -1,
	GOD_ZEUS,
	GOD_POSEIDON,
	GOD_JESUS,
	GOD_THOR,
	GOD_ANUBIS,
	GOD_RA,
	GOD_ODIN,
	GOD_VENUS,
	GOD_ANANSI,
	GOD_COUNT,
} God;

char * god_texture_paths[GOD_COUNT] = {
	"resources/zeus.png",
	"resources/poseidon.png",
	"resources/jesus.png",
	"resources/thor.png",
	"resources/anubis.png",
	"resources/ra.png",
	"resources/odin.png",
	"resources/venus.png",
	"resources/anansi.png",
};

void god_eating_sound(God god)
{
	switch (god) {
	case GOD_ZEUS:
	case GOD_THOR:
	case GOD_ANUBIS:
	case GOD_ODIN:
	case GOD_RA:
		play_sound(SOUND_EAT_LOW);
		break;
	case GOD_POSEIDON:
	case GOD_JESUS:
	case GOD_ANANSI:
		play_sound(SOUND_EAT_MED);
		break;
	case GOD_VENUS:
		play_sound(SOUND_EAT_HIGH);
		break;
	}
}

SDL_Texture * load_texture_from_path(char * path)
{
	int w, h, n;
	unsigned char * data = stbi_load(path, &w, &h, &n, 4);
	assert(data);
	SDL_Surface * surface = SDL_CreateRGBSurfaceFrom(data, w, h, 4 * 8, w * 4,
													 0x000000ff, 0x0000ff00,
													 0x00ff0000, 0xff000000);
	SDL_Texture * texture = SDL_CreateTextureFromSurface(sdl_state.renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}

typedef enum {
	MAIN_MENU_NOTHING,
	MAIN_MENU_PLAY,
	MAIN_MENU_QUIT,
} Main_Menu_Msg;

static bool music_on = true;
static bool sound_on = true;

typedef struct {
	SDL_Texture * bg;
	SDL_Texture * slider_texture;

	SDL_Texture * music_on_texture;
	SDL_Texture * music_off_texture;
	SDL_Texture * sound_on_texture;
	SDL_Texture * sound_off_texture;

	float slider;
	bool clicked_this_frame;
	bool sliding;
} State_Main_Menu;

SDL_Rect slider_box(State_Main_Menu * state)
{
	return (SDL_Rect) {
		UI_SLIDER_X, state->slider * (UI_SLIDER_Y_BOTTOM - UI_SLIDER_Y_TOP) + UI_SLIDER_Y_TOP,
		UI_SLIDER_SIZE, UI_SLIDER_SIZE,
	};
}

void state_main_menu_init(State_Main_Menu * state)
{
	play_music(MUSIC_MENU);
	state->bg = load_texture_from_path("resources/title.png");
	state->slider_texture = load_texture_from_path("resources/slider.png");

	state->music_on_texture = load_texture_from_path("resources/music.png");
	state->music_off_texture = load_texture_from_path("resources/music-off.png");
	state->sound_on_texture = load_texture_from_path("resources/sound.png");
	state->sound_off_texture = load_texture_from_path("resources/sound-off.png");

	state->slider = 1.0;
	state->clicked_this_frame = false;
	state->sliding = false;
}

void state_main_menu_event(State_Main_Menu * state, SDL_Event event)
{
	switch (event.type) {
	case SDL_MOUSEBUTTONDOWN: {
		state->clicked_this_frame = true;
	} break;
	case SDL_MOUSEBUTTONUP: {
		state->sliding = false;
	} break;
	}
}

float fclamp(float k, float lower, float upper)
{
	return fmin(upper, fmax(lower, k));
}

Main_Menu_Msg state_main_menu_update(State_Main_Menu * state)
{
	int mx, my;
	uint32_t mask = SDL_GetMouseState(&mx, &my);
	SDL_Point point = (SDL_Point) {mx, my};
	if (state->clicked_this_frame) {
		// Play/Quit buttons
		state->clicked_this_frame = false;
		SDL_Rect play_rect = UI_PLAY_RECT;
		if (SDL_PointInRect(&point, &play_rect)) {
			return MAIN_MENU_PLAY;
		}
		SDL_Rect quit_rect = UI_QUIT_RECT;
		if (SDL_PointInRect(&point, &quit_rect)) {
			// Kluge to deal with music/sound switches
			SDL_Rect escape_box = SWITCH_ESCAPE_BOX;
			if (!SDL_PointInRect(&point, &escape_box)) {
				return MAIN_MENU_QUIT;
			}
		}
		// Difficulty slider
		SDL_Rect slider_rect = slider_box(state);
		if (SDL_PointInRect(&point, &slider_rect)) {
			state->sliding = true;
		}
		// Sound / Music
		SDL_Rect music_rect = MUSIC_SWITCH_RECT;
		if (SDL_PointInRect(&point, &music_rect)) {
			if (music_on) {
				Mix_VolumeMusic(0);
				music_on = false;
			} else {
				Mix_VolumeMusic(MIX_MAX_VOLUME);
				music_on = true;
			}
		}
		SDL_Rect sound_rect = SOUND_SWITCH_RECT;
		if (SDL_PointInRect(&point, &sound_rect)) {
			int volume;
			if (sound_on) {
				volume = 0;
				sound_on = false;
			} else {
				volume = MIX_MAX_VOLUME;
				sound_on = true;
			}
			int channels = Mix_AllocateChannels(-1);
			for (int i = 0; i < channels; i++) {
				Mix_Volume(i, volume);
			}
		}
	}
	if (state->sliding) {
		state->slider = fclamp((float) (my - UI_SLIDER_SIZE / 2 - UI_SLIDER_Y_TOP) / 
								        (UI_SLIDER_Y_BOTTOM - UI_SLIDER_Y_TOP),
								0.0, 1.0);
		state->slider = roundf(state->slider * 10) / 10;
	}
	// Update difficulty through slider
	difficulty = (1.0 - state->slider) * (DIFFICULTY_MULT - 1.0) + 1.0;
	return MAIN_MENU_NOTHING;
}

SDL_Texture * render_text(char * text, SDL_Color color, int * w, int * h) 
{
	SDL_Surface * surface = TTF_RenderText_Solid(default_font, text, color);
	*w = surface->w;
	*h = surface->h;
	SDL_Texture * texture = SDL_CreateTextureFromSurface(sdl_state.renderer, surface);
	SDL_FreeSurface(surface);
	return texture;
}

void state_main_menu_render(State_Main_Menu * state)
{
	SDL_RenderCopy(sdl_state.renderer, state->bg, NULL, NULL);
	SDL_Rect slider_rect = slider_box(state);
	SDL_RenderCopy(sdl_state.renderer, state->slider_texture, NULL, &slider_rect);
	// Render difficulty text
	{
		char buffer[512];
		sprintf(buffer, "%.1f", difficulty);
		int w, h;
		SDL_Texture * texture = render_text(buffer, (SDL_Color) { 0xff, 0xff, 0xff, 0xff }, &w, &h);
		SDL_Rect rect = (SDL_Rect) { UI_DIFF_TEXT_X, UI_DIFF_TEXT_Y, w, h };
		SDL_RenderCopy(sdl_state.renderer, texture, NULL, &rect);
		SDL_DestroyTexture(texture);
	}
	// Sound/music switches
	{
		SDL_Texture * music_tex = music_on ? state->music_on_texture : state->music_off_texture;
		SDL_Rect music_rect = MUSIC_SWITCH_RECT;
		SDL_RenderCopy(sdl_state.renderer, music_tex, NULL, &music_rect);

		SDL_Texture * sound_tex = sound_on ? state->sound_on_texture : state->sound_off_texture;
		SDL_Rect sound_rect = SOUND_SWITCH_RECT;
		SDL_RenderCopy(sdl_state.renderer, sound_tex, NULL, &sound_rect);
	}
}

typedef struct {
	int frame;
	float frame_timer;
	Ingredient in_fire;
	float cook_time;
	bool cooking;
} Fire;

typedef enum {
	PLAYING_OK,
	PLAYING_LOST,
} Playing_Msg;

typedef struct {
	// Textures
	SDL_Texture * bg_texture;
	SDL_Texture * death_texture;
	SDL_Texture * ingredient_textures[INGRED_COUNT];
	SDL_Texture * logs_texture;
	SDL_Texture * fire_textures[UI_FIRE_FRAMES];
	SDL_Texture * god_textures[GOD_COUNT];
	// Fires
	Fire fires[UI_FIRE_COUNT];
	// Ingredients
	Ingredient transient_ingredient;
	Ingredient * transient_previous;
	// Gods
	God tables[UI_TABLE_COUNT];
	Ingredient * table_orders[UI_TABLE_COUNT];
	float god_spawn_reset;
	float god_spawn_this_reset;
	float god_spawn_timer;
	// Win?
	bool lost;
	float death_timer;
	float time_spent;
} State_Playing;

SDL_Rect ingredient_box(Ingredient ingredient)
{
	return make_SDL_Rect(UI_INGRED_X + (UI_INGRED_SIZE + UI_INGRED_SPACING) * ingredient,
						 UI_INGRED_Y, UI_INGRED_SIZE, UI_INGRED_SIZE);
}

SDL_Rect table_box(int table)
{
	assert(table >= 0 && table < UI_TABLE_COUNT);
	int x, y;
	if (table == 0 || table == 2) {
		x = 25;
	} else {
		x = 227;
	}
	if (table == 0 || table == 1) {
		y = 15;
	} else {
		y = 247;
	}
	return make_SDL_Rect(x, y, UI_TABLE_W, UI_TABLE_H);
}

SDL_Rect order_box(int table, int order)
{
	SDL_Rect tb = table_box(table);
	tb.x += UI_ORDER_SIZE * order;
	tb.y += UI_ORDER_Y_OFFSET;
	tb.w = UI_ORDER_SIZE;
	tb.h = UI_ORDER_SIZE;
	return tb;
}

SDL_Rect fire_box(int fire)
{
	return make_SDL_Rect(UI_FIRE_X + (UI_FIRE_W + UI_FIRE_SPACING) * fire,
						 UI_FIRE_Y, UI_FIRE_W, UI_FIRE_H);
}

SDL_Rect fire_shelf_box(int fire)
{
	return make_SDL_Rect(UI_FIRE_SHELF_X + (UI_FIRE_W + UI_FIRE_SPACING) * fire,
						 UI_FIRE_SHELF_Y, UI_INGRED_SIZE, UI_INGRED_SIZE);
}

void state_playing_init(State_Playing * state)
{
	// Play music
	play_music(MUSIC_PLAYING);

	// Transient init
	state->transient_ingredient = INGRED_NONE;
	state->transient_previous = NULL;

	// Fire init
	for (int i = 0; i < UI_FIRE_COUNT; i++) {
		state->fires[i].frame = 0;
		state->fires[i].frame_timer = UI_FIRE_FPS;
		state->fires[i].in_fire = INGRED_NONE;
		state->fires[i].cook_time = COOK_TIME;
		state->fires[i].cooking = false;
	}
	
	// Gods
	for (int i = 0; i < UI_TABLE_COUNT; i++) {
		state->tables[i] = GOD_NONE;
		state->table_orders[i] = NULL;
	}
	state->god_spawn_reset = 10.0;
	state->god_spawn_this_reset = state->god_spawn_reset;
	state->god_spawn_timer = 0.0;
	
	// Death timer
	state->death_timer = 5.0;

	// Background texture
	state->bg_texture = load_texture_from_path("resources/bg.png");

	// Death screen texture
	state->death_texture = load_texture_from_path("resources/death.png");
	
	// Ingredient textures
	for (int i = 0; i < INGRED_COUNT; i++) {
		state->ingredient_textures[i] =
			load_texture_from_path(ingredient_texture_paths[i]);
	}

	// Bonfire textures
	state->logs_texture = load_texture_from_path("resources/logs.png");
	for (int i = 0; i < UI_FIRE_FRAMES; i++) {
		char buffer[512];
		sprintf(buffer, "resources/fire%d.png", i);
		state->fire_textures[i] = load_texture_from_path(buffer);
	}

	// God textures
	for (int i = 0; i < GOD_COUNT; i++) {
		state->god_textures[i] = load_texture_from_path(god_texture_paths[i]);
	}

	// Win?
	state->lost = false;
	state->time_spent = 0.0;
}

Ingredient generator_click(Vector2 pos)
{
	for (int i = 0; i < INGRED_UNCOOKED_COUNT; i++) {
		SDL_Point point = (SDL_Point) { pos.x, pos.y };
		SDL_Rect rect = ingredient_box(i);
		if (SDL_PointInRect(&point, &rect)) {
			return i;
		}
	}
	return INGRED_NONE;
}

int over_fire(Vector2 pos)
{
	SDL_Point point = (SDL_Point) { pos.x, pos.y };
	for (int i = 0; i < UI_FIRE_COUNT; i++) {
		SDL_Rect rect = fire_box(i);
		if (SDL_PointInRect(&point, &rect)) {
			return i;
		}
	}
	return -1;
}

int over_table(Vector2 pos)
{
	SDL_Point point = (SDL_Point) { pos.x, pos.y };
	for (int i = 0; i < UI_TABLE_COUNT; i++) {
		SDL_Rect rect = table_box(i);
		if (SDL_PointInRect(&point, &rect)) {
			return i;
		}
	}
	return -1;
}

bool over_trashcan(Vector2 pos)
{
	SDL_Point point = (SDL_Point) { pos.x, pos.y };
	SDL_Rect rect = (SDL_Rect) { UI_TRASH_X, UI_TRASH_Y, UI_TRASH_SIZE, UI_TRASH_SIZE };
	return SDL_PointInRect(&point, &rect);
}

void state_playing_mbdown(State_Playing * state, Vector2 mpos)
{
	// Check ingredient generators
	if (generator_click(mpos) != INGRED_NONE) {
		Ingredient new_ingredient = generator_click(mpos);
		state->transient_ingredient = new_ingredient;
	}
	// Check fire
	if (over_fire(mpos) != -1) {
		Fire * fire = &state->fires[over_fire(mpos)];
		if (fire->in_fire != INGRED_NONE && !fire->cooking) {
			state->transient_ingredient = fire->in_fire;
			state->transient_previous = &fire->in_fire;
			fire->in_fire = INGRED_NONE;
		}
	}
}

void state_playing_mbup(State_Playing * state, Vector2 mpos)
{
	if (state->transient_ingredient == INGRED_NONE) {
		return;
	}
	
	// Check fires
	if (over_fire(mpos) != -1) {
		Fire * fire = &state->fires[over_fire(mpos)];
		if (fire->in_fire == INGRED_NONE && state->transient_ingredient < INGRED_UNCOOKED_COUNT) {
			play_sound(SOUND_TSCH);
			fire->in_fire = state->transient_ingredient;
			state->transient_previous = NULL;
			fire->cook_time = COOK_TIME;
			fire->cooking = true;
		}
	}
	
	// Check tables
	if (over_table(mpos) != -1) {
		int table = over_table(mpos);
		if (state->tables[table] != GOD_NONE &&
			sb_last(state->table_orders[table]) == state->transient_ingredient) {
			god_eating_sound(state->tables[table]);
			sb_pop(state->table_orders[table]);
			state->transient_previous = NULL;
			if (sb_count(state->table_orders[table]) == 0) {
				state->tables[table] = GOD_NONE;
			}
		}
	}

	// Check trashcan
	if (over_trashcan(mpos)) {
		state->transient_ingredient = INGRED_NONE;
		state->transient_previous = NULL;
	}
	
	if (state->transient_previous) {
		*state->transient_previous = state->transient_ingredient;
	}
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
	case SDL_KEYDOWN:
		if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
			state->lost = true;
		}
		break;
	}
}

Ingredient * generate_order()
{
	Ingredient * list = NULL;
	int amt = (rand() % 3) + 1;
	for (int i = 0; i < amt; i++) {
		sb_push(list, (rand() % INGRED_UNCOOKED_COUNT) + INGRED_UNCOOKED_COUNT);
	}
	return list;
}

Playing_Msg state_playing_update(State_Playing * state)
{
	// Death screen
	if (state->lost) {
		if (state->death_timer < 0) {
			return PLAYING_LOST;
		}
		state->death_timer -= sdl_state.delta_time;
		return PLAYING_OK;
	}

	// Cook food
	for (int i = 0; i < UI_FIRE_COUNT; i++) {
		Fire * fire = &state->fires[i];
		if (fire->cooking) {
			fire->cook_time -= sdl_state.delta_time;
			if (fire->cook_time <= 0) {
				play_sound(SOUND_TSS);
				fire->cooking = false;
				fire->in_fire += INGRED_UNCOOKED_COUNT;
			}
		}
	}
	// Spawn gods
	if (state->god_spawn_timer <= 0.0) {
		state->god_spawn_timer = state->god_spawn_reset;
		state->god_spawn_this_reset = state->god_spawn_reset;
		state->god_spawn_reset *= SUB_BASE_MULT - (difficulty / SUB_MULT_DIV);
		state->god_spawn_reset = fmax(state->god_spawn_reset, MINIMUM_SPAWN_TIME - (difficulty / MST_DIV));
		bool full = true;
		for (int i = 0; i < UI_TABLE_COUNT; i++) {
			if (state->tables[i] == GOD_NONE) {
				God g;
				while (true) {
					g = rand() % GOD_COUNT;
					bool repeat = false;
					for (int i = 0; i < UI_TABLE_COUNT; i++) {
						if (state->tables[i] == g) repeat = true;
					}
					if (!repeat) break;
				}
				play_sound(SOUND_TABLED);
				state->tables[i] = g;
				state->table_orders[i] = generate_order();
				full = false;
				break;
			}
		}
		if (full) {
			state->lost = true;
			play_sound(SOUND_THUNDER);
			Mix_HaltMusic();
		}
	}
	state->god_spawn_timer -= sdl_state.delta_time;

	state->time_spent += sdl_state.delta_time;

	return PLAYING_OK;
}

void state_playing_render(State_Playing * state)
{
	// Death screen
	if (state->lost) {
		SDL_RenderCopy(sdl_state.renderer, state->death_texture, NULL, NULL);
		char buffer[512];
		sprintf(buffer, "You lasted %.0f seconds", state->time_spent);
		int w, h;
		SDL_Texture * texture = render_text(buffer, (SDL_Color) { 0xff, 0xff, 0xff, 0xff }, &w, &h);
		SDL_Rect rect = (SDL_Rect) { DEATH_TEXT_X, DEATH_TEXT_Y, w, h };
		SDL_RenderCopy(sdl_state.renderer, texture, NULL, &rect);
		SDL_DestroyTexture(texture);
		return;
	}

	// Background
	SDL_RenderCopy(sdl_state.renderer, state->bg_texture, NULL, NULL);
	
	// Generators
	for (int i = 0; i < INGRED_UNCOOKED_COUNT; i++) {
		SDL_Rect rect = ingredient_box(i);
		SDL_RenderCopy(sdl_state.renderer,
					   state->ingredient_textures[i],
					   NULL, &rect);
	}
	
	// Fire
	for (int i = 0; i < UI_FIRE_COUNT; i++) {
		Fire * fire = &state->fires[i];
		SDL_Rect rect = fire_box(i);
		SDL_RenderCopy(sdl_state.renderer, state->logs_texture, NULL, &rect);
		SDL_RenderCopy(sdl_state.renderer, state->fire_textures[fire->frame], NULL, &rect);
		if (fire->in_fire != INGRED_NONE) {
			SDL_Rect ingred_rect = fire_shelf_box(i);
			SDL_RenderCopy(sdl_state.renderer, state->ingredient_textures[fire->in_fire], NULL, &ingred_rect);
		}
		
		// Update fire animation
		fire->frame_timer -= sdl_state.delta_time * (((float) rand() / (float) RAND_MAX) * 1.2 - 0.1);
		if (fire->frame_timer < 0) {
			fire->frame = (fire->frame + 1) % UI_FIRE_FRAMES;
			fire->frame_timer = UI_FIRE_FPS;
		}
	}

	// Gods
	for (int i = 0; i < UI_TABLE_COUNT; i++) {
		God seated = state->tables[i];
		SDL_Rect rect = table_box(i);
		if (seated != GOD_NONE) {
			SDL_RenderCopy(sdl_state.renderer,
						   state->god_textures[seated],
						   NULL, &rect);
		}
	}

	// Orders
	for (int t = 0; t < UI_TABLE_COUNT; t++) {
		if (state->tables[t] == GOD_NONE) continue;
		for (int i = 0; i < sb_count(state->table_orders[t]); i++) {
			SDL_Rect rect = order_box(t, i);
			SDL_RenderCopy(sdl_state.renderer,
						   state->ingredient_textures[state->table_orders[t][i]],
						   NULL, &rect);
		}
	}

	// Clock
	{
		SDL_SetRenderDrawColor(sdl_state.renderer, 0xff, 0xff, 0xff, 0xff);
		int ox = UI_CLOCK_X, oy = UI_CLOCK_Y;
		float theta = (2.0 * PI) - (state->god_spawn_timer / state->god_spawn_this_reset) * 2.0 * PI;
		theta -= (PI / 2.0);
		int rx = ox + (UI_CLOCK_RADIUS * cos(theta));
		int ry = oy + (UI_CLOCK_RADIUS * sin(theta));
		SDL_RenderDrawLine(sdl_state.renderer, ox, oy, rx, ry);
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

enum Game_State {
	STATE_PLAYING,
	STATE_MAIN_MENU,
};

typedef struct {
	enum Game_State type;
	union {
		State_Playing   state_playing;
		State_Main_Menu state_main_menu;
	};
} Game_State;

int main()
{
	difficulty = 0.5;
	srand(time(0));
	SDL_Init(SDL_INIT_VIDEO);

	TTF_Init();
	default_font = TTF_OpenFont("resources/EBGaramond12-AllSC.ttf", UI_FONT_SIZE);

	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024);
	sound_init();

	sdl_state.last_count = SDL_GetPerformanceCounter();
	SDL_Window * window = SDL_CreateWindow(
		"LD43",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN);
	SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);
	sdl_state.renderer = renderer;
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	Game_State ** game_state_stack = NULL;

	{
		Game_State * gs = (Game_State*) malloc(sizeof(Game_State));
		gs->type = STATE_MAIN_MENU;
		sb_push(game_state_stack, gs);
	}

	bool new_frame = true;
	
	SDL_Event event;
	bool running = true;
	while (running && sb_count(game_state_stack) > 0) {
		Game_State * game_state = sb_last(game_state_stack);
		
		if (new_frame) {
			new_frame = false;
			switch (game_state->type) {
			case STATE_PLAYING:
				state_playing_init(&(game_state->state_playing));
				break;
			case STATE_MAIN_MENU:
				state_main_menu_init(&(game_state->state_main_menu));
				break;
			default:
				assert(false);
				break;
			}
		}

		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				running = false;
			} else {
				switch (game_state->type) {
				case STATE_PLAYING:
					state_playing_event(&(game_state->state_playing), event);
					break;
				case STATE_MAIN_MENU:
					state_main_menu_event(&(game_state->state_main_menu), event);
					break;
				default:
					assert(false);
					break;
				}
			}
		}

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
		SDL_RenderClear(renderer);
		
		switch (sb_last(game_state_stack)->type) {
		case STATE_PLAYING: {
			Playing_Msg msg = state_playing_update(&(game_state->state_playing));
			switch (msg) {
			case PLAYING_OK:
				state_playing_render(&(game_state->state_playing));
				break;
			case PLAYING_LOST:
				sb_pop(game_state_stack);
				new_frame = true;
				break;
			}
		} break;
		case STATE_MAIN_MENU: {
			Main_Menu_Msg msg = state_main_menu_update(&(game_state->state_main_menu));
			switch (msg) {
			case MAIN_MENU_NOTHING:
				state_main_menu_render(&(game_state->state_main_menu));
				break;
			case MAIN_MENU_PLAY: {
				Game_State * gs = (Game_State*) malloc(sizeof(Game_State));
				gs->type = STATE_PLAYING;
				sb_push(game_state_stack, gs);
				new_frame = true;
			} break;
			case MAIN_MENU_QUIT:
				sb_pop(game_state_stack);
				new_frame = true;
				break;
			}
		} break;
		default:
			assert(false);
			break;
		}

		//printf("%f\r", difficulty);
		//fflush(stdout);
							   
		SDL_RenderPresent(renderer);
		
		uint64_t frame_end = SDL_GetPerformanceCounter();
		sdl_state.delta_time =
			(float) (frame_end - sdl_state.last_count) / SDL_GetPerformanceFrequency();
		sdl_state.last_count = frame_end;
	}
	
	return 0;
}
