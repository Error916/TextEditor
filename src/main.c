#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "./la.h"
#include "./editor.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH  (FONT_WIDTH  / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

void scc(int code){ //sdl check code
	if(code < 0){
		fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
		exit(1);
	}
}

void* scp(void *ptr){ //sdl check pointer
	if(ptr == NULL){
		fprintf(stderr, "SDL ERROR: %s\n", SDL_GetError());
		exit(1);
	}
	return ptr;
}

SDL_Surface* surface_from_file(const char *file_path){
	int w, h, n;
	unsigned char *pixels = stbi_load(file_path, &w, &h, &n, STBI_rgb_alpha);
	if(pixels == NULL){
		fprintf(stderr, "ERROR: could not load file %s: %s\n", file_path, stbi_failure_reason());
		exit(1);
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const Uint32 rmask = 0xff000000;
	const Uint32 gmask = 0x00ff0000;
	const Uint32 bmask = 0x0000ff00;
	const Uint32 amask = 0x000000ff;
#else
	const Uint32 rmask = 0x000000ff;
	const Uint32 gmask = 0x0000ff00;
	const Uint32 bmask = 0x00ff0000;
	const Uint32 amask = 0xff000000;
#endif

	const int depth = 32;
	const int pitch = 4*w;
	return scp(SDL_CreateRGBSurfaceFrom((void*)pixels, w, h, depth, pitch, rmask, gmask, bmask, amask));
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
	SDL_Texture *spritesheet;
	SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path){
	Font font = {0};
	SDL_Surface *font_surface = surface_from_file(file_path);
	scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF000000));
	font.spritesheet = scp(SDL_CreateTextureFromSurface(renderer, font_surface));
	SDL_FreeSurface(font_surface);

	for(size_t ascii = ASCII_DISPLAY_LOW; ascii < ASCII_DISPLAY_HIGH; ++ascii){
		const size_t index = ascii - ASCII_DISPLAY_LOW; // remove the ascii value for space
		const size_t col = index % FONT_COLS;
		const size_t row = index / FONT_COLS;
		font.glyph_table[index] = (SDL_Rect) {
			.x = (int) col * FONT_CHAR_WIDTH,
			.y = (int) row * FONT_CHAR_HEIGHT,
			.w = FONT_CHAR_WIDTH,
			.h = FONT_CHAR_HEIGHT,
		};
	}

	return font;
}

void render_char(SDL_Renderer *render, const Font *font, char c, Vec2f pos, float scale){
	const SDL_Rect dst = {
		.x = (int) floorf(pos.x),
		.y = (int) floorf(pos.y),
		.w = (int) floorf(FONT_CHAR_WIDTH * scale),
		.h = (int) floorf(FONT_CHAR_HEIGHT * scale),
	};

	size_t index = '?' - ASCII_DISPLAY_LOW;
	if(ASCII_DISPLAY_LOW <= c && c <= ASCII_DISPLAY_HIGH){
		index = c - ASCII_DISPLAY_LOW;
	}

	scc(SDL_RenderCopy(render, font->spritesheet, &font->glyph_table[index], &dst));
}

void set_texture_color(SDL_Texture *texture, Uint32 color){
	scc(SDL_SetTextureColorMod(texture, (color >> (8 * 0)) & 0xff, (color >> (8 * 1)) & 0xff, (color >> (8 * 2)) & 0xff));
	scc(SDL_SetTextureAlphaMod(texture, (color >> (8 * 3)) & 0xff));
}

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, size_t text_size, Vec2f pos, Uint32 color, float scale){

	set_texture_color(font->spritesheet, color);

	Vec2f pen = pos;
	for(size_t i = 0; i < text_size; ++i){
		render_char(renderer, font, text[i], pen, scale);
		pen.x += FONT_CHAR_WIDTH * scale;
	}
}

Editor editor = {0};

#define UNHEX(color) \
	((color) >> (8 * 0) & 0xFF), \
	((color) >> (8 * 1) & 0xFF), \
	((color) >> (8 * 2) & 0xFF), \
	((color) >> (8 * 3) & 0xFF)

void render_cursor(SDL_Renderer *renderer, const Font *font){
	const Vec2f pos = vec2f((float) editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
				(float) editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE);
	const SDL_Rect rect = {
		.x = (int) floorf(pos.x),
		.y = (int) floorf(pos.y),
		.w = FONT_CHAR_WIDTH * FONT_SCALE,
		.h = FONT_CHAR_HEIGHT * FONT_SCALE,
	};

	scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
	scc(SDL_RenderFillRect(renderer, &rect));

	const char *c = editor_char_under_cursor(&editor);
	if(c){
		set_texture_color(font->spritesheet, 0xFF000000);
		render_char(renderer, font, *c, pos, FONT_SCALE);
	}
}

// TODO: jump back/forward by a word
// TODO: delite a word
// TODO: make the cursor blink
// TODO: Save and load files
// TODO: Delite line
// TODO: insert line between chars

int main(int argc, char **argv){
	(void) argc;
	(void) argv;

	scc(SDL_Init(SDL_INIT_VIDEO));

	SDL_Window *window = scp(SDL_CreateWindow("Texteditor", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE));
	SDL_Renderer *renderer = scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));
	Font font = font_load_from_file(renderer, "./font/charmap-oldschool_white.png");

	editor_insert_text_before_cursor(&editor, ""); // some strange inizialization beviour

	bool quit = false;
	while(!quit){
		SDL_Event event = {0};
		while(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_QUIT: {
					quit = true;
				} break;

				case SDL_KEYDOWN: {
					switch(event.key.keysym.sym){
						case SDLK_BACKSPACE: {
							editor_backspace_before_cursor(&editor);
						} break;

						case SDLK_F2: {
                    					editor_save_to_file(&editor, "output");
                				} break;

						case SDLK_DELETE: {
							editor_delete_over_cursor(&editor);
						} break;

						case SDLK_RETURN: {
							editor_insert_new_line(&editor);
						} break;

						case SDLK_UP: {
							if(editor.cursor_row > 0){
								editor.cursor_row -= 1;
							}
						} break;

						case SDLK_DOWN: {
							editor.cursor_row += 1;
						} break;

						case SDLK_LEFT: {
							if(editor.cursor_col > 0){
								editor.cursor_col -= 1;
							}
						} break;

						case SDLK_RIGHT: {
							editor.cursor_col += 1;
						} break;
					}
				} break;

				case SDL_TEXTINPUT: {
					editor_insert_text_before_cursor(&editor, event.text.text);
				} break;
			}
		}

		scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
		scc(SDL_RenderClear(renderer));

		for(size_t row = 0; row < editor.size; ++row){
			const Line *line = editor.lines + row;
			render_text_sized(renderer, &font, line->chars, line->size, vec2f(0.0f , (float) row * FONT_CHAR_HEIGHT * FONT_SCALE), 0xFFFFFFFF, FONT_SCALE);
		}
		render_cursor(renderer, &font);

		SDL_RenderPresent(renderer);
	}

	SDL_Quit();

	return 0;
}
