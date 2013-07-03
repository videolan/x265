#ifndef _UTILS_H_
#define _UTILS_H_ 1
#include <assert.h>

#define PASTER(name, val) name ## _ ## val
#define EVALUATOR(x, y)   PASTER(x, y)
#define NAME(func)        EVALUATOR(func, ARCH)

#endif
