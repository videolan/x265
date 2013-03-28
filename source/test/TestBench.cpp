#include <ctype.h>
#include "UnitTest.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>

#include "vectorclass.h"
#include "primitives.h"

#define snprintf _snprintf
using namespace x265;


// GCC doesn't align stack variables on ARM, so use .bss
#if ARCH_ARM
#undef ALIGNED_16
#define ALIGNED_16( var ) DECLARE_ALIGNED( static var, 16 )
#endif

/* buf1, buf2: initialised to random data and shouldn't write into them */
unsigned int *buf1, *buf2;
/* buf3, buf4: used to store output */
unsigned int *buf3, *buf4;
/* pbuf1, pbuf2: initialised to random pixel data and shouldn't write into them. */
unsigned int *pbuf1, *pbuf2;
/* pbuf3, pbuf4: point to buf3, buf4, just for type convenience */
unsigned int *pbuf3, *pbuf4;

int quiet = 0;

#define report( name ) { \
    if( used_asm && !quiet ) \
        fprintf( stderr, " - %-21s [%s]\n", name, ok ? "OK" : "FAILED" ); \
    if( !ok ) ret = -1; \
}

#define BENCH_RUNS 100  // tradeoff between accuracy and speed
#define BENCH_ALIGNS 16 // number of stack+heap data alignments (another accuracy vs speed tradeoff)
#define MAX_FUNCS 1000  // just has to be big enough to hold all the existing functions
#define MAX_CPUS 30     // number of different combinations of cpu flags

#define PIXEL_MAX ((1 << 8)-1)

typedef struct
{
    void *pointer; // just for detecting duplicates
    unsigned long  cpu;
    unsigned long cycles;
    unsigned long den;
} bench_t;

typedef struct
{
    char *name;
    bench_t vers[MAX_CPUS];
} bench_func_t;

int do_bench = 0;
int bench_pattern_len = 0;
const char *bench_pattern = "";
char func_name[100];
static bench_func_t benchs[MAX_FUNCS];

static const char *pixel_names[12] = { "16x16", "16x8", "8x16", "8x8", "8x4", "4x8", "4x4", "4x16", "4x2", "2x8", "2x4", "2x2" };
static const char *intra_predict_16x16_names[7] = { "v", "h", "dc", "p", "dcl", "dct", "dc8" };
static const char *intra_predict_8x8c_names[7] = { "dc", "h", "v", "p", "dcl", "dct", "dc8" };
static const char *intra_predict_4x4_names[12] = { "v", "h", "dc", "ddl", "ddr", "vr", "hd", "vl", "hu", "dcl", "dct", "dc8" };
static const char **intra_predict_8x8_names = intra_predict_4x4_names;
static const char **intra_predict_8x16c_names = intra_predict_8x8c_names;

#define set_func_name(...) snprintf( func_name, sizeof(func_name), __VA_ARGS__ )

// Used to get the Time for Doing Bench mark
static inline unsigned long read_time(void)
{
    unsigned long a = 0;
#if HAVE_X86_INLINE_ASM
    asm volatile( "rdtsc" :"=a"(a) ::"edx" );
#elif ARCH_PPC
    asm volatile( "mftb %0" : "=r" (a) );
#elif ARCH_ARM     // ARMv7 only
    asm volatile( "mrc p15, 0, %0, c9, c13, 0" : "=r"(a) );
#endif
    return a;
}

static bench_t* get_bench( const char *name, int cpu )
{
    int i, j;
    for( i = 0; benchs[i].name && strcmp(name, benchs[i].name); i++ )
        assert( i < MAX_FUNCS );
    if( !benchs[i].name )
        benchs[i].name = strdup( name );
    if( !cpu )
        return &benchs[i].vers[0];
    for( j = 1; benchs[i].vers[j].cpu && benchs[i].vers[j].cpu != cpu; j++ )
        assert( j < MAX_CPUS );
    benchs[i].vers[j].cpu = cpu;
    return &benchs[i].vers[j];
}

static int cmp_nop( const void *a, const void *b )
{
    return *(unsigned int*)a - *(unsigned int*)b;
}

static int cmp_bench( const void *a, const void *b )
{
    // asciibetical sort except preserving numbers
    const char *sa = ((bench_func_t*)a)->name;
    const char *sb = ((bench_func_t*)b)->name;
    for( ;; sa++, sb++ )
    {
        if( !*sa && !*sb )
            return 0;
        if( isdigit( *sa ) && isdigit( *sb ) && isdigit( sa[1] ) != isdigit( sb[1] ) )
            return isdigit( sa[1] ) - isdigit( sb[1] );
        if( *sa != *sb )
            return *sa - *sb;
    }
}

static void print_bench(void)
{
    unsigned int nops[10000] = {0};
    int nfuncs, nop_time=0;

    for( int i = 0; i < 10000; i++ )
    {
        int t = read_time();
        nops[i] = read_time() - t;
    }
    qsort( nops, 10000, sizeof(unsigned int), cmp_nop );
    for( int i = 500; i < 9500; i++ )
        nop_time += nops[i];
    nop_time /= 900;
    printf( "nop: %d\n", nop_time );

    for( nfuncs = 0; nfuncs < MAX_FUNCS && benchs[nfuncs].name; nfuncs++ );
    qsort( benchs, nfuncs, sizeof(bench_func_t), cmp_bench );
    for( int i = 0; i < nfuncs; i++ )
        for( int j = 0; j < MAX_CPUS && (!j || benchs[i].vers[j].cpu); j++ )
        {
            int k;
            bench_t *b = &benchs[i].vers[j];
            if( !b->den )
                continue;
            for( k = 0; k < j && benchs[i].vers[k].pointer != b->pointer; k++ );
            if( k < j )
                continue;
            //printf( "%s_%s%s: %"PRId64"\n", benchs[i].name,
#if HAVE_MMX
                    b->cpu&X264_CPU_AVX2 && b->cpu&X264_CPU_FMA3 ? "avx2_fma3" :
                    b->cpu&X264_CPU_AVX2 ? "avx2" :
                    b->cpu&X264_CPU_FMA3 ? "fma3" :
                    b->cpu&X264_CPU_FMA4 ? "fma4" :
                    b->cpu&X264_CPU_XOP ? "xop" :
                    b->cpu&X264_CPU_AVX ? "avx" :
                    b->cpu&X264_CPU_SSE4 ? "sse4" :
                    b->cpu&X264_CPU_SSSE3 ? "ssse3" :
                    b->cpu&X264_CPU_SSE3 ? "sse3" :
                    /* print sse2slow only if there's also a sse2fast version of the same func */
                    b->cpu&X264_CPU_SSE2_IS_SLOW && j<MAX_CPUS-1 && b[1].cpu&X264_CPU_SSE2_IS_FAST && !(b[1].cpu&X264_CPU_SSE3) ? "sse2slow" :
                    b->cpu&X264_CPU_SSE2 ? "sse2" :
                    b->cpu&X264_CPU_SSE ? "sse" :
                    b->cpu&X264_CPU_MMX ? "mmx" :
#elif ARCH_PPC
                    b->cpu&X264_CPU_ALTIVEC ? "altivec" :
#elif ARCH_ARM
                    b->cpu&X264_CPU_NEON ? "neon" :
                    b->cpu&X264_CPU_ARMV6 ? "armv6" :
#endif
                    "c",
#if HAVE_MMX
                    b->cpu&X264_CPU_CACHELINE_32 ? "_c32" :
                    b->cpu&X264_CPU_SLOW_ATOM && b->cpu&X264_CPU_CACHELINE_64 ? "_c64_atom" :
                    b->cpu&X264_CPU_CACHELINE_64 ? "_c64" :
                    b->cpu&X264_CPU_SLOW_SHUFFLE ? "_slowshuffle" :
                    b->cpu&X264_CPU_SSE_MISALIGN ? "_misalign" :
                    b->cpu&X264_CPU_LZCNT ? "_lzcnt" :
                    b->cpu&X264_CPU_BMI2 ? "_bmi2" :
                    b->cpu&X264_CPU_BMI1 ? "_bmi1" :
                    b->cpu&X264_CPU_SLOW_CTZ ? "_slow_ctz" :
                    b->cpu&X264_CPU_SLOW_ATOM ? "_atom" :
#elif ARCH_ARM
                    b->cpu&X264_CPU_FAST_NEON_MRC ? "_fast_mrc" :
#endif
                    "",
                    ((int)10*b->cycles/b->den - nop_time)/4;
        }
}


/* detect when callee-saved regs aren't saved
 * needs an explicit asm check because it only sometimes crashes in normal use. */

#if ARCH_X86 || ARCH_X86_64
int x265_stack_pagealign( int (*func)(), int align );
intptr_t x265_checkasm_call( intptr_t (*func)(), int *ok, ... );
#else
#define x265_stack_pagealign( func, align ) func()
#endif

#define call_c1(func,...) func(__VA_ARGS__)

#if ARCH_X86_64
void x265_checkasm_stack_clobber( uint64_t clobber, ... );
#define call_a1(func,...) ({ \
    unsigned int r = (rand() & 0xffff) * 0x0001000100010001ULL; \
    x265_checkasm_stack_clobber( r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r ); /* max_args+6 */ \
    x265_checkasm_call(( intptr_t(*)())func, &ok, 0, 0, 0, 0, __VA_ARGS__ ); })
#elif ARCH_X86
#define call_a1(func,...) x265_checkasm_call( (intptr_t(*)())func, &ok, __VA_ARGS__ )
#else
#define call_a1 call_c1
#endif

#define call_bench(func,cpu,...)\
    if( do_bench && !strncmp(func_name, bench_pattern, bench_pattern_len) )\
    {\
        unsigned int tsum = 0;\
        int tcount = 0;\
        call_a1(func, __VA_ARGS__);\
        for( int ti = 0; ti < (cpu?BENCH_RUNS:BENCH_RUNS/4); ti++ )\
        {\
            unsigned int t = read_time();\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            func(__VA_ARGS__);\
            t = read_time() - t;\
            if( t*tcount <= tsum*4 && ti > 0 )\
            {\
                tsum += t;\
                tcount++;\
            }\
        }\
        bench_t *b = get_bench( func_name, cpu );\
        b->cycles += tsum;\
        b->den += tcount;\
        b->pointer = func;\
    }

#define call_a(func,...) ({ call_a2(func,__VA_ARGS__); call_a1(func,__VA_ARGS__); })
#define call_c(func,...) ({ call_c2(func,__VA_ARGS__); call_c1(func,__VA_ARGS__); })
#define call_a2(func,...) ({ call_bench(func,cpu_new,__VA_ARGS__); })
#define call_c2(func,...) ({ call_bench(func,0,__VA_ARGS__); })

//Sample Testing for Bench Marking
static int check_vector(int cpu_ref, int cpu_new)
{
		int ret = 0, ok = 1, used_asm = 0;
	    int size = 0x4000;
		short *Orig = (short *)malloc(size+100);
        short *Cur = (short *) malloc(size*100);

		//Set the Bench Mark Function name 
        set_func_name( "xGetSAD8" );
				
		int size1 = 0x4000;
        uint8_t *input = (uint8_t *)malloc(size+100);
        uint8_t *output = (uint8_t *)malloc(size*100);

		EncoderPrimitives primitives;
		       
		//Generates the Randome i/p for Benchmark and run the function 100 times for time predictions
		for( int i = 0; i < 100; i++ )
        {
            /* Test corner-case sizes */
            int test_size = i < 10 ? i+1 : rand() & 0x3fff;
            /* Test 8 different probability distributions of zeros */
            for( int j = 0; j < test_size+32; j++ )
                input[j] = (rand()&((1 << ((i&7)+1)) - 1)) * rand();

			for( int j = 0; j < test_size+32; j++ )
                output[j] = (rand()&((1 << ((i&7)+1)) - 1)) * rand();

			//At this poing Function table is not ready
			//Once the Functions for Vector Primitives are ready call those methodes and 
			//Call Actual Methods for bench marking

		}
		
		//Dump the Repoprt for Log message
		report( "check_vector:" );
					
	return 0;
}

static int check_all_funcs( int cpu_ref, int cpu_new )
{
	//The Functions which needs to do Unit Bench
	//here is the place to check the functions  for bench marking
    return check_vector( cpu_ref, cpu_new );      
}

static int add_flags( int *cpu_ref, int *cpu_new, int flags, const char *name )
{
    *cpu_ref = *cpu_new;
    *cpu_new |= flags;
#if BROKEN_STACK_ALIGNMENT
    *cpu_new |= X264_CPU_STACK_MOD4;
#endif
    if( *cpu_new & 0x0200000 )
        *cpu_new &= ~0x0100000;
    if( !quiet )
        fprintf( stderr, "x265: %s\n", name );
    return check_all_funcs( *cpu_ref, *cpu_new );
}

static int check_all_flags( void )
{
    int ret = 0;
    int cpu0 = 0, cpu1 = 0;

	// Find the CPU Based on the ARC and PASS that into for bench marking
	//Default it has been initialised for SSE4.2

	ret |= add_flags( &cpu0, &cpu1, 0x0000100, "SSE4.2" );

	return ret;
}


int main(int argc, char *argv[])
{
    int ret = 0;

    if( argc > 1 && !strncmp( argv[1], "--bench", 7 ) )
    {
        do_bench = 1;
    }

	struct timeb tb;
    ftime( &tb );
     
    int seed = ( argc > 1 ) ? atoi(argv[1]) : ((unsigned long)tb.time * 1000 + (unsigned long)tb.millitm) * 1000;
    fprintf( stderr, "x264: using random seed %u\n", seed );
    srand( seed );

    buf1 =(unsigned int *) malloc( 0x1e00 + 0x2000*sizeof(unsigned int) + 16*BENCH_ALIGNS );
    pbuf1 = (unsigned int *) malloc( 0x1e00*sizeof(unsigned int) + 16*BENCH_ALIGNS );
    if( !buf1 || !pbuf1 )
    {
        fprintf( stderr, "malloc failed, unable to initiate tests!\n" );
        return -1;
    }

    for( int i = 0; i < 0x1e00; i++ )
    {
        buf1[i] = rand() & 0xFF;
        pbuf1[i] = rand() & PIXEL_MAX;
    }
    memset( buf1+0x1e00, 0, 0x2000*sizeof(unsigned int) );

    /* 16-byte alignment is guaranteed whenever it's useful, but some functions also vary in speed depending on %64 */
    if( do_bench )
        for( int i = 0; i < BENCH_ALIGNS && !ret; i++ )
        {
               //INIT_POINTER_OFFSETS;
			    buf2 = buf1 + 0xf00;
				buf3 = buf2 + 0xf00;
				buf4 = buf3 + 0x1000*sizeof(unsigned int);
				pbuf2 = pbuf1 + 0xf00;
				pbuf3 = (unsigned int*)buf3;
				pbuf4 = (unsigned int*)buf4;

            ret |= x265_stack_pagealign( check_all_flags, i*16 );
            buf1 += 16;
            pbuf1 += 16;
            quiet = 1;
            fprintf( stderr, "%d/%d\r", i+1, BENCH_ALIGNS );
        }
    else
        ret = check_all_flags();

    if( ret )
    {
        fprintf( stderr, "x265: at least one test has failed. Go and fix that Right Now!\n" );
        return -1;
    }
    fprintf( stderr, "x265: All tests passed Yeah :)\n" );
    if( do_bench )
        print_bench();
    return 0;
}


