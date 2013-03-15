/****************************  vectorclass.h   ********************************
* Author:        Agner Fog
* Date created:  2012-05-30
* Last modified: 2012-08-01
* Version:       1.02 Beta
* Project:       vector classes
* Description:
* Header file defining vector classes as interface to 
* intrinsic functions in x86 microprocessors with SSE2 and later instruction
* sets up to AVX2.
*
* Instructions:
* Use Gnu, Intel or Microsoft C++ compiler. Compile for the desired 
* instruction set, which must be at least SSE2. Specify the supported 
* instruction set by a command line define, e.g. __SSE4_1__ if the 
* compiler does not automatically do so.
*
* Each vector object is represented internally in the CPU as a 128-bit or
* 256 bit register.
*
* This header file includes the appropriate header files depending on the
* supported instruction set
*
* For detailed instructions, see VectorClass.pdf
*
* (c) Copyright 2012 GNU General Public License 3.0 http://www.gnu.org/licenses
******************************************************************************/
#ifndef VECTORCLASS_H
#define VECTORCLASS_H  102


#include "instrset.h"        // Select supported instruction set

#if INSTRSET < 2             // SSE2 required
  #error Please compile for the SSE2 instruction set or higher
#else

#include "vectori128.h"      // 128-bit integer vectors
#include "vectorf128.h"      // 128-bit floating point vectors
#if INSTRSET >= 8
  #include "vectori256.h"    // 256-bit integer vectors, requires AVX2 instruction set
#else
  #include "vectori256e.h"   // 256-bit integer vectors, emulated
#endif  // INSTRSET >= 8
#if INSTRSET >= 7
  #include "vectorf256.h"    // 256-bit floating point vectors, requires AVX instruction set
#else
  #include "vectorf256e.h"   // 256-bit floating point vectors, emulated
#endif  // INSTRSET >= 7

#endif  // INSTRSET < 2


#endif  // VECTORCLASS_H
