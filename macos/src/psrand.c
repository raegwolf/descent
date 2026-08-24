#include "psrand.h"
static unsigned int d_rand_seed;
int psrand(void) { return ((d_rand_seed = d_rand_seed * 0x41c64e6dU + 0x3039U) >> 16) & 0x7fff; }
void pssrand(unsigned int seed) { d_rand_seed = seed; }
