#ifndef DESCENT_MACOS_PSRAND_H
#define DESCENT_MACOS_PSRAND_H
int psrand(void);
void pssrand(unsigned int seed);
#define PSRAND_MAX 32768
#endif
