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

#if !defined(__LEDSTRING_H__)
#define __LEDSTRING_H__

#include "sim_avr.h"
#include "sim_irq.h"
#include <stdint.h>

#define LEDSTRING_ROWS 8
#define LEDSTRING_COLS 12

enum {
	IRQ_LEDSTRING_IN = 0,
};

typedef struct ledstring_minmax_t {
	unsigned int min;
	unsigned int max;
} ledstring_minmax_t;

typedef struct ledstring_hl {
	struct ledstring_minmax_t low;
	struct ledstring_minmax_t high;
} ledstring_hl;

typedef struct ledstring_t {
	avr_irq_t* irq;
	uint8_t ram[LEDSTRING_ROWS * LEDSTRING_COLS * 3];
	uint8_t buf[LEDSTRING_ROWS * LEDSTRING_COLS * 3];
	uint8_t* next;
	int next_bit;
	int updated;

	avr_cycle_count_t last_transition[2];
	avr_cycle_count_t last_duration[2];

	struct ledstring_hl t0;
	struct ledstring_hl t1;
	unsigned int tr_min;

	avr_cycle_count_t *timer;
	const char* error;
} ledstring_t;

void ledstring_init(struct avr_t* avr, ledstring_t* p);

#endif
