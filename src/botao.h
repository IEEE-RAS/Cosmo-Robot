#ifndef COSMO_BOTAO_H
#define COSMO_BOTAO_H
#include <stdbool.h>
void botao_init(unsigned gpio);
bool botao_pressionado(void); // true apenas em uma nova pressao estavel
#endif
