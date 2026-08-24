#ifndef DESCENT_ARDUINO_PSRAND_H
#define DESCENT_ARDUINO_PSRAND_H
int psrand(void);
void pssrand(unsigned int seed);
#define PSRAND_MAX 32768
#endif
