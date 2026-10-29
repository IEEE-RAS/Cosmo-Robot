#ifndef BOTAO_H
#define BOTAO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void botao_init(unsigned pino);
bool botao_esperar(unsigned ms);

#ifdef __cplusplus
}
#endif

#endif
