#include <stdint.h>

#include "fix.h"
#include "timer.h"
#include "sdl_shim.h"

static uint64_t timer_epoch;

void timer_init(void) { timer_epoch = descent_sdl_ticks(); }
void timer_close(void) {}
void timer_set_rate(int count_val) { (void)count_val; }
void timer_set_function(void *function) { (void)function; }

fix timer_get_fixed_seconds(void)
{
	return (fix)(((descent_sdl_ticks() - timer_epoch) * (uint64_t)F1_0) / 1000);
}

fix timer_get_fixed_secondsX(void) { return timer_get_fixed_seconds(); }
fix timer_get_approx_seconds(void) { return timer_get_fixed_seconds(); }
