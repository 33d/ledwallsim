/*
		gbsim is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 3 of the License, or
		(at your option) any later version.

		gbsim is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with gbsim.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdint.h>

#define DISPLAY_SCALE_X 16
#define DISPLAY_SCALE_Y 16

#define LEDSTRING_ROWS 8
#define LEDSTRING_COLS 12

#include "numbers.h"

struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	SDL_Texture* numbers_texture;
	char rate[6];
	uint8_t fb[LEDSTRING_ROWS * LEDSTRING_COLS * 3];
} display;

int display_init() {
	memset(&display, 0, sizeof(display));

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		return 0;

	display.window = SDL_CreateWindow("WS2812",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			LEDSTRING_COLS*DISPLAY_SCALE_X,
			LEDSTRING_ROWS*DISPLAY_SCALE_Y, 0);
	if (display.window == NULL)
		return 0;

	display.renderer = SDL_CreateRenderer(display.window, -1,
			SDL_RENDERER_PRESENTVSYNC);

	display.texture = SDL_CreateTexture(display.renderer,
			SDL_PIXELFORMAT_RGB24,
			SDL_TEXTUREACCESS_STREAMING, LEDSTRING_COLS, LEDSTRING_ROWS);
	if (display.texture == NULL)
		return 0;

	SDL_Surface* numbers_surface = SDL_CreateRGBSurfaceFrom((void*) numbers_bitmap,
			numbers_bitmap_width, numbers_bitmap_height, 1, numbers_bitmap_width / 8,
			0x80000000, 0x80000000, 0x80000000, 0xFFFFFFFF);
	display.numbers_texture = SDL_CreateTextureFromSurface(display.renderer, numbers_surface);
	if (display.numbers_texture == 0)
		return 0;
	SDL_FreeSurface(numbers_surface);

	return 1;
}

int display_update(const uint8_t ram[LEDSTRING_ROWS * LEDSTRING_COLS * 3]) {
	const uint8_t* src = ram;
	uint8_t* dest = display.fb;
	for (int y = 0; y < LEDSTRING_ROWS; y++) {
		for (int x = 0; x < LEDSTRING_COLS; x += 2) {
			src = ram + ((x * LEDSTRING_ROWS + y) * 3);
			dest = display.fb + ((y * LEDSTRING_COLS + x) * 3);
			*(dest + 1) = *src;
			*(dest + 0) = *(src + 1);
			*(dest + 2) = *(src + 2);
		}
	}
	for (int y = 0; y < LEDSTRING_ROWS; y++) {
		for (int x = 1; x < LEDSTRING_COLS; x += 2) {
			src = ram + (((x+1) * LEDSTRING_ROWS - y) * 3);
			dest = display.fb + ((y * LEDSTRING_COLS + x) * 3);
			*(dest + 1) = *src;
			*(dest + 0) = *(src + 1);
			*(dest + 2) = *(src + 2);
		}
	}
	return 1;
}

static void draw_rate() {
	SDL_Rect src_rect = { 0, 0, numbers_bitmap_width / 10, numbers_bitmap_height};
	SDL_Rect dest_rect = { 5, 5, numbers_bitmap_width / 10, numbers_bitmap_height};
	for (int i = 0; i < sizeof(display.rate); i++) {
		char c = display.rate[i];
		if (((c - '0') & 0x80) == 0) {
			src_rect.x = (c - '0') * (numbers_bitmap_width / 10);
			SDL_RenderCopy(display.renderer, display.numbers_texture, &src_rect, &dest_rect);
		}
		dest_rect.x += (numbers_bitmap_width / 10);
	}
}

int display_render() {
	SDL_UpdateTexture(display.texture, NULL, display.fb,
			LEDSTRING_COLS * 3);

	SDL_RenderClear(display.renderer);
	SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);

	draw_rate();

	SDL_RenderPresent(display.renderer);
	return 1;
}

void display_rate(int rate) {
	snprintf(display.rate, sizeof(display.rate), "%5d", rate);
}

int display_destroy() {
	if (display.texture != NULL)
		SDL_DestroyTexture(display.texture);
	if (display.renderer != NULL)
		SDL_DestroyRenderer(display.renderer);
	if (display.window != NULL)
		SDL_DestroyWindow(display.window);
	return 1;
}

const char* display_error_message() {
	return SDL_GetError();
}
