#pragma once

#ifndef _MSC_VER
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif

#include <crtdefs.h> // for intptr_t
#if !defined(UINT64_MAX)
#include <limits.h>
#define UINT64_MAX _UI64_MAX
#define INT16_MAX  _I16_MAX
#endif

/* a minimal set of C99 types for use with MSVC (VC9) */

typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

