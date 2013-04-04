#include "UnitTest.h"
#include "primitives.h"
#include "pixel.CPP"
#include "primitives.CPP"

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

/* buf1, buf2: initialised to random data and shouldn't write into them */
unsigned short *buf1, *buf2;
/* buf3, buf4: used to store output */
unsigned short *buf3, *buf4;
/* pbuf1, pbuf2: initialised to random pixel data and shouldn't write into them. */
unsigned short *pbuf1, *pbuf2;
/* pbuf3, pbuf4: point to buf3, buf4, just for type convenience */
unsigned short *pbuf3, *pbuf4;

int quiet = 0;

#define BENCH_RUNS 100  // tradeoff between accuracy and speed
#define BENCH_ALIGNS 16 // number of stack+heap data alignments (another accuracy vs speed tradeoff)
#define MAX_FUNCS 1000  // just has to be big enough to hold all the existing functions
#define MAX_CPUS 30     // number of different combinations of cpu flags
#define PIXEL_MAX ((1 << 8)-1)

int do_bench = 0;
int bench_pattern_len = 0;
const char *bench_pattern = "";
char func_name[100];

#if _MSC_VER
#pragma warning(disable: 4505) // static function unused, has been removed
#endif

/* detect when callee-saved regs aren't saved
 * needs an explicit asm check because it only sometimes crashes in normal use. */

#if ARCH_X86 || ARCH_X86_64
int x265_stack_pagealign(int (*func)(), int align);
intptr_t x265_checkasm_call(intptr_t (*func)(), int *ok, ...);
#else
#define x265_stack_pagealign( func, align ) func()
#endif

#define call_c1(func,...) func(__VA_ARGS__)

#if ARCH_X86_64
void x265_checkasm_stack_clobber(uint64_t clobber, ...);
#define call_a1(func,...) ({ \
    unsigned int r = (rand() & 0xffff) * 0x0001000100010001ULL; \
    x265_checkasm_stack_clobber( r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r ); /* max_args+6 */ \
    x265_checkasm_call(( intptr_t(*)())func, &ok, 0, 0, 0, 0, __VA_ARGS__ ); })
#elif ARCH_X86
#define call_a1(func,...) x265_checkasm_call( (intptr_t(*)())func, &ok, __VA_ARGS__ )
#else
#define call_a1 call_c1
#endif

//Sample Testing for satdx*x
static int check_satd(int cpu_ref, int cpu_new)
{
 	int ret = 0, ok = 1, used_asm = 0;

    //Set the Bench Mark Function name
	printf("satd_*x* Functons");
	unsigned long int sse2 = 0, vector = 2;
	unsigned short strt = 0, end = 0, func = 0, vecf = 0;
	int j = 0, i = 0;
	int var_v[100], var_c[1000];
	int primitives = 0;
	struct timeb tb;

	//Do the bench for 16 - Number of Partions
	while(primitives < NUM_PARTITIONS)
	{
		//if the satd is not available for vector no need to test bench
		if(primitives_vectorized_sse2.satd[primitives] != NULL)
		{
			//run the Vectorised functions 100 times with random pixel and store the output
			j = 0;
			for(i = 0; i<=100; i++)
			{

				var_v[i] = primitives_vectorized_sse2.satd[primitives](pbuf1+j, 16, pbuf2, 16);
				j += 16;
			}

			//run the c primitives 100 times and store the output with randome pixel
			j = 0;
			for(i = 0; i<= 100; i++)
			{
				var_c[i] = primitives_c.satd[primitives]( pbuf1+j, 16, pbuf2, 16 );
				j += 16;
			}

			//compare both the output
			i = 0;
			while(i != 100)
			{
				if(var_c[i] != var_v[i])
				{
					printf("FAILED COMPARISON for Primitives - %d \n", primitives);
					return -1;
				}
				i++;
			}

			primitives ++;
		}else //if there is no vectorised function for satd then need not to do test bench
			primitives ++;
	}

    return ret;
}

static int check_all_funcs(int cpu_ref, int cpu_new)
{
    //The Functions which needs to do Unit Bench
    //here is the place to check the functions  for bench marking
    return check_satd(cpu_ref, cpu_new);
}

static int add_flags(int *cpu_ref, int *cpu_new, int flags, const char *name)
{
    *cpu_ref = *cpu_new;
    *cpu_new |= flags;
#if BROKEN_STACK_ALIGNMENT
    *cpu_new |= X264_CPU_STACK_MOD4;
#endif
    if (*cpu_new & 0x0200000)
        *cpu_new &= ~0x0100000;
    if (!quiet)
        fprintf(stderr, "x265: %s\n", name);

    return check_all_funcs(*cpu_ref, *cpu_new);
}

static int check_all_flags(void)
{
    int ret = 0;
    int cpu0 = 0, cpu1 = 0;

    // Find the CPU Based on the ARC and PASS that into for bench marking
    //Default it has been initialised for SSE4.2 - Need More Implementations in Feature

    ret |= add_flags(&cpu0, &cpu1, 0x0000100, "SSE4.2");

    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;

    if (argc > 1 && !strncmp(argv[1], "--bench", 7))
    {
        do_bench = 1;
    }

	do_bench = 1;

//    int seed = (argc > 1) ? atoi(argv[1]) : ((unsigned long)tb.time * 1000 + (unsigned long)tb.millitm) * 1000;
  //  fprintf(stderr, "x265: using random seed %u\n", seed);
  //  srand(seed);

    pbuf1 = (unsigned short *) malloc(0x1e00 * sizeof(unsigned short) + 16 * BENCH_ALIGNS);
	pbuf2 = (unsigned short *) malloc(0x1e00 * sizeof(unsigned short) + 16 * BENCH_ALIGNS);

    if (!pbuf1 || !pbuf2)
    {
        fprintf(stderr, "malloc failed, unable to initiate tests!\n");
        return -1;
    }

    //INIT_POINTER_OFFSETS;
    for( int i = 0; i < 0x1e00; i++ )
    {
		//Generate the Randome Buffer for Testing
        pbuf1[i] = rand() & PIXEL_MAX;
		pbuf2[i] = rand() & PIXEL_MAX;
    }

    /* 16-byte alignment is guaranteed whenever it's useful, but some functions also vary in speed depending on %64 */
    if (do_bench)
        for (int i = 0; i < BENCH_ALIGNS && !ret; i++)
        {
			ret |= x265_stack_pagealign(check_all_flags, i * 16);
            pbuf1 += 16;
            quiet = 1;
            fprintf(stderr, "%d/%d\r", i + 1, BENCH_ALIGNS);
        }
    else
        ret = check_all_flags();

    if (ret)
    {
        fprintf(stderr, "x265: at least one test has failed. Go and fix that Right Now!\n");
        return -1;
    }
    fprintf(stderr, "x265: All tests passed Yeah :)\n");

    return 0;
}


