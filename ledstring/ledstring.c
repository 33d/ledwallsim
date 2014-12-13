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
#include "ledstring.h"
#include "stdio.h"
#include "string.h"

#define ledstring_IRQ_COUNT 4
#define ledstring_COMMAND 0
#define ledstring_DATA 1
#define ledstring_ROWS 6
#define ledstring_COLS 84
#define STRINGIFY(x) #x
#define ledstring_ERROR(p, e) { (p)->error = (STRINGIFY(__LINE__) ": "  e) ; return; }

enum { LOW=1, HIGH=0 } state;

static const char* irq_names[ledstring_IRQ_COUNT] = {
	[IRQ_LEDSTRING_IN] = "ledstring.in"
};

static void add_bit(ledstring_t* const p, int high) {
	// buffer full?
	if (p->next > p->ram + sizeof(p->ram))
		return;

	*(p->next) <<= 1;
	if (high)
		*(p->next) |= 1;
	++(p->next_bit);
	if (p->next_bit > 7) {
		p->next_bit = 0;
		++(p->next);
	}
	p->updated = 1;
}

static void reset(ledstring_t* const p) {
	p->next = p->ram;
	p->next_bit = 0;
}

static void ledstring_in_hook(struct avr_irq_t * irq, uint32_t value, void * param) {
	ledstring_t* const p = (ledstring_t*) param;

	uint32_t old = (~value) & 1;
	p->last_duration[old] = *p->timer - p->last_transition[old];
	p->last_transition[value] = *p->timer;

	// After a transition HIGH, check for a command
	if (value) {
		if ((p->last_duration[0] >= p->t0.low.min && p->last_duration[0] <= p->t0.low.max)
				&& (p->last_duration[1] >= p->t1.low.min && p->last_duration[1] <= p->t1.low.max))
			add_bit(p, 0);
		else if ((p->last_duration[0] >= p->t0.high.min && p->last_duration[0] <= p->t0.high.max)
				&& (p->last_duration[1] >= p->t1.high.min && p->last_duration[1] <= p->t1.high.max))
			add_bit(p, 1);
		else if (p->last_duration[0] >= p->tr_min)
			reset(p);
	}
}

void ledstring_init(struct avr_t* avr, struct ledstring_t* p) {
	p->irq = avr_alloc_irq(&avr->irq_pool, 0, ledstring_IRQ_COUNT, irq_names);
	avr_irq_register_notify(p->irq + IRQ_LEDSTRING_IN, ledstring_in_hook, p);
	p->next = p->ram;
	p->timer = &(avr->cycle);
	p->updated = 0;

	// Calculate the pulse limits
	unsigned int cycle_ns = 1000000000 / avr->frequency;
	p->t0.high.min = (400 - 150) / cycle_ns + 1;
	p->t0.high.max = (400 + 150) / cycle_ns;
	p->t0.low.min = (850 - 150) / cycle_ns + 1;
	p->t0.low.max = (850 + 150) / cycle_ns;
	p->t1.high.min = (800 - 150) / cycle_ns + 1;
	p->t1.high.max = (800 + 150) / cycle_ns;
	p->t1.low.min = (450 - 150) / cycle_ns + 1;
	p->t1.low.max = (450 + 150) / cycle_ns;
	p->tr_min = 50000/cycle_ns + 1;
}
