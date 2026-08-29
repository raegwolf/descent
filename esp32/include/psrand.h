#ifndef DESCENT_ESP32_PSRAND_H
#define DESCENT_ESP32_PSRAND_H
int psrand(void);
void pssrand(unsigned int seed);
#define PSRAND_MAX 32768
#endif
