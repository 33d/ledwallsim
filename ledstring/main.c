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

#include <SDL2/SDL.h>
#include <SDL2/SDL_assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_avr.h"
#include "sim_gdb.h"
#include "sim_elf.h"
#include "avr_ioport.h"
#include "avr_spi.h"

#include "ledstring.h"
#include "sdl_display.h"

static ledstring_t ledstring;
static uint8_t lcd_ram[84*6];
static int lcd_updated;
static SDL_mutex* lcd_ram_mutex;
static int quit_flag;
static struct {
	uint32_t last_timer;
	avr_cycle_count_t last_avr_ticks;
} rate;

void check_simulation_rate(const avr_t* avr) {
	uint32_t timer = SDL_GetTicks();
	uint32_t elapsed = timer - rate.last_timer;
	// How many cycles *should* have elapsed
	int32_t calculated = elapsed * 16000;
	int32_t actual = (avr->cycle - rate.last_avr_ticks);
	// Don't bother sleeping for <10 ms
	if (actual - 320000 > calculated) {
		SDL_Delay((actual - calculated) / 16000);
		rate.last_timer = SDL_GetTicks();
		rate.last_avr_ticks = avr->cycle;
	}
}

int avr_run_thread(void *ptr) {
	avr_t* avr = (avr_t*) ptr;
	int state = cpu_Running;
	rate.last_avr_ticks = avr->cycle;
	rate.last_timer = SDL_GetTicks();
	uint32_t last_display_timer = rate.last_timer;
	avr_cycle_count_t last_display_avr_ticks = avr->cycle;

	do {
		int count = 10000;
		while (( state != cpu_Done ) && ( state != cpu_Crashed ) && --count > 0 )
			state = avr_run(avr);

		check_simulation_rate(avr);

		// Update the display rate if necessary
		uint32_t timer = SDL_GetTicks();
		if (timer - last_display_timer > 1000) {
			// How many kcycles should have elapsed
			uint32_t calculated = (timer - last_display_timer) * 16000;
			uint32_t actual = avr->cycle - last_display_avr_ticks;
			int percent_rate = 100 * actual / calculated;
			display_rate(percent_rate);
			last_display_timer = timer;
			last_display_avr_ticks = avr->cycle;
		}

		SDL_LockMutex(lcd_ram_mutex);

		if (ledstring.updated) {
			memcpy(lcd_ram, ledstring.ram, sizeof(lcd_ram));
			lcd_updated = 1;
			ledstring.updated = 0;
		}

		if (quit_flag) {
			SDL_UnlockMutex(lcd_ram_mutex);
			break;
		}

		SDL_UnlockMutex(lcd_ram_mutex);
	} while (state != cpu_Done && state != cpu_Crashed);

	return 0;
}

void main_loop() {
	int quit = 0;
	SDL_Event e;

	while (!quit) {
		SDL_LockMutex(lcd_ram_mutex);

		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			} /* switch */
		}

		int my_lcd_updated = lcd_updated;
		if (my_lcd_updated) {
			display_update(lcd_ram);
			lcd_updated = 0;
		}

		SDL_UnlockMutex(lcd_ram_mutex);

		if (my_lcd_updated)
			display_render();

		SDL_Delay(10);
	}
}

int main(int argc, char* argv[]) {
	int r = 0;

	char* elf_file = NULL;
	int gdb_port = 0;

	for (char** arg = &argv[1]; *arg != NULL; ++arg) {
		if (strcmp("-d", *arg) == 0)
			gdb_port = atoi(*(++arg));
		else
			elf_file = *arg;
	}
	if (elf_file == NULL) {
		fprintf(stderr, "Give me a .elf file to load\n");
		return 1;
	}

	elf_firmware_t f;
	elf_read_firmware(elf_file, &f);
	avr_t* avr = avr_make_mcu_by_name("atmega328p");
	if (!avr) {
		fprintf(stderr, "Unsupported cpu atmega328p\n");
		return 1;
	}

	avr_init(avr);
	if (gdb_port != 0) {
		avr->gdb_port = gdb_port;
		avr_gdb_init(avr);
	}
	avr->frequency = 16000000;
	avr_load_firmware(avr, &f);

	ledstring_init(avr, &ledstring);
	avr_connect_irq(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 6),
			ledstring.irq + IRQ_LEDSTRING_IN);

	if (display_init()) {
		lcd_ram_mutex = SDL_CreateMutex();
		SDL_Thread* avr_thread = SDL_CreateThread(avr_run_thread, "avr-thread", avr);
		main_loop();
		SDL_LockMutex(lcd_ram_mutex);
		quit_flag = 1;
		SDL_UnlockMutex(lcd_ram_mutex);
		int avr_thread_return;
		SDL_WaitThread(avr_thread, &avr_thread_return);
		SDL_DestroyMutex(lcd_ram_mutex);
	} else {
		r = 1;
		fprintf(stderr, "%s\n", display_error_message());
	}

	display_destroy();

	return r;
}
