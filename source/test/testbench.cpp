#include "unittest.h"
#include "primitives.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if _MSC_VER
#define snprintf _snprintf
#define strdup _strdup
#endif

using namespace x265;

/* pbuf1, pbuf2: initialised to random pixel data and shouldn't write into them. */
pixel *pbuf1, *pbuf2;
uint16_t quiet = 0;
uint16_t do_bench = 0;
uint16_t do_singleprimitivecheck = 0;
uint16_t numofprim = 0;

#define PIXEL_MAX ((1 << 8) - 1)
#define BENCH_ALIGNS 16

#if _MSC_VER
#pragma warning(disable: 4505)
#endif

/* detect when callee-saved regs aren't saved
 * needs an explicit asm check because it only sometimes crashes in normal use. */

#if ARCH_X86 || ARCH_X86_64
int x265_stack_pagealign(int (*func)(), int align);
intptr_t x265_checkasm_call(intptr_t (*func)(), int *ok, ...);
#else
#define x265_stack_pagealign(func, align) func()
#endif

static int do_bench_mark()
{
    return 0;
}

//Sample Testing for satdx*x
static int check_pixelprimitives(void)
{
    uint32_t ret = 0;
    uint32_t j = 0, i = 0;
    uint32_t var_v[100], var_c[1000];
    
    x265::EncoderPrimitives cprimitives;
    x265::EncoderPrimitives vectorprimitives;

    int cpuid;

    cpuid = x265::CpuIDDetect();
    memset(&vectorprimitives, 0, sizeof(vectorprimitives));

#if defined(__GNUC__) || defined(_MSC_VER)
    if (cpuid > 1) Setup_Vec_Primitives_sse2(vectorprimitives);

    if (cpuid > 2) Setup_Vec_Primitives_sse3(vectorprimitives);

    if (cpuid > 3) Setup_Vec_Primitives_ssse3(vectorprimitives);

    if (cpuid > 4) Setup_Vec_Primitives_sse41(vectorprimitives);

    if (cpuid > 5) Setup_Vec_Primitives_sse42(vectorprimitives);

#endif // if defined(__GNUC__) || defined(_MSC_VER)
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
    if (cpuid > 6) Setup_Vec_Primitives_avx(vectorprimitives);

#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
    if (cpuid > 7) Setup_Vec_Primitives_avx2(vectorprimitives);

#endif

    //Initialise the default c_Primitives
    x265::Setup_C_PixelPrimitives(cprimitives);

    //option to check for any single primitives

    //Do the bench for 16 - Number of Partions
    while (numofprim < x265::NUM_PARTITIONS)
    {
        //if the satd is not available for vector no need to test bench
        if (vectorprimitives.satd[tprimitives[numofprim]])
        {
            //run the Vectorised functions 100 times with random pixel and store the output
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                var_v[i] = vectorprimitives.satd[tprimitives[numofprim]](pbuf1 + j, 16, pbuf2, 16);
                j += 16;
            }

            //run the c primitives 100 times and store the output with randome pixel
            j = 0;
            for (i = 0; i <= 100; i++)
            {
                var_c[i] = cprimitives.satd[tprimitives[numofprim]](pbuf1 + j, 16, pbuf2, 16);
                j += 16;
            }

            //compare both the output
            i = 0;
            while (i != 100)
            {
                if (var_c[i] != var_v[i])
                {
                    printf("FAILED COMPARISON for Primitives - %d \n", numofprim);
                    return -1;
                }

                i++;
            }

            numofprim++;
        }
        else //if there is no vectorised function for satd then need not to do test bench
            numofprim++;

        if (do_singleprimitivecheck == 1)
            break;
    }

    return ret;
}

static int check_all_funcs()
{
    //The Functions which needs to do Unit Bench
    //here is the place to check the functions  for bench marking
    return check_pixelprimitives();
}

int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc > 1 && !strncmp(argv[1], "--bench", 7))
    {
        do_bench = 1;
    }

    if (argc > 1 && !strncmp(argv[1], "--primitive", 11))
    {
        do_singleprimitivecheck = 1;
        numofprim = atoi(argv[2]);
    }

    pbuf1 = (unsigned short*)malloc(0x1e00 * sizeof(unsigned short) + 16 * BENCH_ALIGNS);
    pbuf2 = (unsigned short*)malloc(0x1e00 * sizeof(unsigned short) + 16 * BENCH_ALIGNS);
    if (!pbuf1 || !pbuf2)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        return -1;
    }

    for (int i = 0; i < 0x1e00; i++)
    {
        //Generate the Randome Buffer for Testing
        pbuf1[i] = rand() & PIXEL_MAX;
        pbuf2[i] = rand() & PIXEL_MAX;
    }

    /* 16-byte alignment is guaranteed whenever it's useful, but some functions also vary in speed depending on %64 */
    if (do_bench)
        ret = do_bench_mark(); //Do the bench marking for all the primitives
    else
        ret = check_all_funcs(); //do the output validation for c and vector primitives

    if (ret)
    {
        fprintf(stderr, "x265: at least one test has failed. Go and fix that Right Now!\n");
        return -1;
    }

    fprintf(stderr, "x265: All tests passed Yeah :)\n");

    return 0;
}
