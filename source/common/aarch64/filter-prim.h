#ifndef _FILTER_PRIM_ARM64_H__
#define _FILTER_PRIM_ARM64_H__


#include "common.h"
#include "slicetype.h"      // LOWRES_COST_MASK
#include "primitives.h"
#include "x265.h"


namespace X265_NS
{


void setupFilterPrimitives_neon(EncoderPrimitives &p);

};


#endif

