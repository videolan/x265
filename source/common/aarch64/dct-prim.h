#ifndef __DCT_PRIM_NEON_H__
#define __DCT_PRIM_NEON_H__


#include "common.h"
#include "primitives.h"
#include "contexts.h"   // costCoeffNxN_c
#include "threading.h"  // CLZ

namespace X265_NS
{
// x265 private namespace
void setupDCTPrimitives_neon(EncoderPrimitives &p);
};



#endif

