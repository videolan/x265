/***************************  complexvec.h   **********************************
| Author:        Agner Fog
| Date created:  2012-07-24
| Last modified: 2012-08-01
| Version:       1.02 Beta
| Project:       vector classes
| Description:
| Classes for complex number math:
| Complex2f:  One complex number consisting of two single precision floats
| Complex4f:  A vector of 2 complex numbers made from 4 single precision floats
| Complex8f:  A vector of 4 complex numbers made from 8 single precision floats
| Complex2d:  One complex number consisting of two double precision floats
| Complex4d:  A vector of 2 complex numbers made from 4 double precision floats
|
| (c) Copyright 2012 GNU General Public License http://www.gnu.org/licenses
\*****************************************************************************/

// The Comples classes do not inherit from the corresponding vector classes
// because that would lead to undesired implicit conversions when calling
// functions that are defined only for the vector class. For example, if the
// sin function is defined for Vec2d but not for Complex2d then an attempt to
// call sin(Complex2d) would in fact call sin(Vec2d) which would be mathematically
// wrong. This is avoided by not using inheritance. Explicit conversion between
// these classes is still possible.

#ifndef COMPLEXVEC_H
#define COMPLEXVEC_H  102

#include "vectorclass.h"
#include <math.h>          // define math library functions


/*****************************************************************************
*
*               Class Complex2f: one single precision complex number
*
*****************************************************************************/

class Complex2f {
protected:
    __m128 xmm; // vector of 4 single precision floats. Only the first two are used
public:
    // default constructor
    Complex2f() {
    }
    // construct from real and imaginary part
    Complex2f(float re, float im) {
        xmm = Vec4f(re, im, 0.f, 0.f);
    }
    // construct from real, no imaginary part
    Complex2f(float re) {
        xmm = _mm_load_ss(&re);
    }
    // Constructor to convert from type __m128 used in intrinsics:
    Complex2f(__m128 const & x) {
        xmm = x;
    }
    // Assignment operator to convert from type __m128 used in intrinsics:
    Complex2f & operator = (__m128 const & x) {
        xmm = x;
        return *this;
    }
    // Type cast operator to convert to __m128 used in intrinsics
    operator __m128() const {
        return xmm;
    }
    // Member function to convert to vector
    Vec4f to_vector() const {
        return xmm;
    }
    // Member function to load from array (unaligned)
    Complex2f & load(float const * p) {
        xmm = Vec4f().load_partial(2, p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Complex2f const & load_a(float const * p) {
        load(p);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(float * p) const {
        Vec4f(xmm).store_partial(2, p);
    }
    // Member function to store into array, aligned by 16
    void store_a(float * p) const {
        store(p);
    }
    // get real part
    float real() const {
        return _mm_cvtss_f32(xmm);
    }
    // get imaginary part
    float imag() const {
        Vec4f t = permute4f<1,-256,-256,-256>(Vec4f(xmm));
        return Complex2f(t).real();
    }
    // Member function to extract real or imaginary part
    float extract(uint32_t index) const {
        float x[4];
        Vec4f(xmm).store(x);
        return x[index & 1];
    }
};

/*****************************************************************************
*
*          Operators for Complex2f
*
*****************************************************************************/

// operator + : add
static inline Complex2f operator + (Complex2f const & a, Complex2f const & b) {
    return Complex2f(Vec4f(a) + Vec4f(b));
}

// operator += : add
static inline Complex2f & operator += (Complex2f & a, Complex2f const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex2f operator - (Complex2f const & a, Complex2f const & b) {
    return Complex2f(Vec4f(a) - Vec4f(b));
}

// operator - : unary minus
static inline Complex2f operator - (Complex2f const & a) {
    return Complex2f(- Vec4f(a));
}

// operator -= : subtract
static inline Complex2f & operator -= (Complex2f & a, Complex2f const & b) {
    a = a - b;
    return a;
}

// operator * : complex multiply is defined as
// (a.re * b.re - a.im * b.im,  a.re * b.im + b.re * a.im)
static inline Complex2f operator * (Complex2f const & a, Complex2f const & b) {
    __m128 b_flip = _mm_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m128 a_im   = _mm_shuffle_ps(a,a,0xF5);   // Imag. part of a in both
    __m128 a_re   = _mm_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m128 aib    = _mm_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
#ifdef __FMA__      // FMA3
    return  _mm_fmaddsub_ps(a_re, b, aib);      // a_re * b +/- aib
#elif defined (__FMA4__)  // FMA4
    return  _mm_maddsub_ps(a_re, b, aib);       // a_re * b +/- aib
#elif  INSTRSET >= 3  // SSE3
    __m128 arb    = _mm_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    return _mm_addsub_ps(arb, aib);             // subtract/add
#else
    __m128 arb    = _mm_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m128 aib_m  = change_sign<1,0,1,0>(Vec4f(aib)); // change sign of low part
    return _mm_add_ps(arb, aib_m);              // add
#endif
}

// operator *= : multiply
static inline Complex2f & operator *= (Complex2f & a, Complex2f const & b) {
    a = a * b;
    return a;
}

// operator / : complex divide is defined as
// (a.re * b.re + a.im * b.im, b.re * a.im - a.re * b.im) / (b.re * b.re + b.im * b.im)
static inline Complex2f operator / (Complex2f const & a, Complex2f const & b) {
    // The following code is made similar to the operator * to enable common 
    // subexpression elimination in code that contains both operator * and 
    // operator / where one or both operands are the same
    __m128 a_re   = _mm_shuffle_ps(a,a,0);      // Real part of a in both
    __m128 arb    = _mm_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m128 b_flip = _mm_shuffle_ps(b,b,1);      // Swap b.re and b.im
    __m128 a_im   = _mm_shuffle_ps(a,a,5);      // Imag. part of a in both
#ifdef __FMA__      // FMA3
    __m128 n      = _mm_fmsubadd_ps(a_im, b_flip, arb); 
#elif defined (__FMA4__)  // FMA4
    __m128 n      = _mm_msubadd_ps (a_im, b_flip, arb);
#else
    __m128 aib    = _mm_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    // The parameters for change_sign are made identical to the ones used in Complex4f so that the constant mask will share the same memory position
    __m128 arbm   = change_sign<0,1,0,1>(Vec4f(arb)); // change sign of high part
    __m128 n      = _mm_add_ps(aib, arbm);      // arbm + aib
#endif  // FMA
    __m128 bb     = _mm_mul_ps(b, b);           // (b.re*b.re, b.im*b.im)
#if INSTRSET >= 3  // SSE3
    __m128 bb2    = _mm_hadd_ps(bb,bb);         // (b.re*b.re + b.im*b.im) 
#else
    __m128 bb1    = _mm_shuffle_ps(bb,bb,1);
    __m128 bb2    = _mm_add_ss(bb,bb1);
#endif
    __m128 bb3    = _mm_shuffle_ps(bb2,bb2,0);  // copy into all positions to avoid division by zero
    return          _mm_div_ps(n, bb3);         // n / bb3
}

// operator /= : divide
static inline Complex2f & operator /= (Complex2f & a, Complex2f const & b) {
    a = a / b;
    return a;
}

// operator ~ : complex conjugate
// ~(a,b) = (a,-b)
static inline Complex2f operator ~ (Complex2f const & a) {
    // The parameters for change_sign are made identical to the ones used in Complex4f so that the constant mask will share the same memory position
    return Complex2f(change_sign<0,1,0,1>(Vec4f(a))); // change sign of high part
}

// operator == : returns true if a == b
static inline bool operator == (Complex2f const & a, Complex2f const & b) {
    Vec4fb t1 = Vec4f(a) == Vec4f(b);
#if INSTRSET >= 5   // SSE4.1 supported. Using PTEST
    Vec4fb t2 = _mm_shuffle_ps(t1, t1, 0x44);
    return horizontal_and(t2);
#else
    __m128i t2 = _mm_srli_epi64(_mm_castps_si128(t1),32);  // get 32 bits down
    __m128i t3 = _mm_and_si128(t2,_mm_castps_si128(t1));   // and 32 bits
    int     t4 = _mm_cvtsi128_si32(t3);                    // transfer 32 bits to integer
    return  t4 == -1;
#endif
}

// operator != : returns true if a != b
static inline bool operator != (Complex2f const & a, Complex2f const & b) {
    return !(a == b);
}

/*****************************************************************************
*
*          Operators mixing Complex2f and float
*
*****************************************************************************/

// operator + : add
static inline Complex2f operator + (Complex2f const & a, float b) {
    return _mm_add_ss(a, _mm_set_ss(b));
}
static inline Complex2f operator + (float a, Complex2f const & b) {
    return b + a;
}
static inline Complex2f & operator += (Complex2f & a, float & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex2f operator - (Complex2f const & a, float b) {
    return _mm_sub_ss(a, _mm_set_ss(b));
}
static inline Complex2f operator - (float a, Complex2f const & b) {
    return Complex2f(a) - b;
}
static inline Complex2f & operator -= (Complex2f & a, float & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Complex2f operator * (Complex2f const & a, float b) {
    return _mm_mul_ps(a, _mm_set1_ps(b));
}
static inline Complex2f operator * (float a, Complex2f const & b) {
    return b * a;
}
static inline Complex2f & operator *= (Complex2f & a, float & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Complex2f operator / (Complex2f const & a, float b) {
    return _mm_div_ps(a, _mm_set1_ps(b));
}

static inline Complex2f operator / (float a, Complex2f const & b) {
    Vec4f b2(b);
    Vec4f b3 = b2 * b2;
#if  INSTRSET >= 3  // SSE3
    __m128 t2 = _mm_hadd_ps(b3,b3);
#else
    __m128 t1 = _mm_shuffle_ps(b3,b3,1);
    __m128 t2 = _mm_add_ss(t1,b3);
#endif
    float  b4 = _mm_cvtss_f32(t2);
    return ~b * (a / b4);
}

static inline Complex2f & operator /= (Complex2f & a, float b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Complex2f
*
*****************************************************************************/

// function abs: absolute value
// abs(a,b) = sqrt(a*a+b*b);
static inline float abs(Complex2f const & a) {
    Vec4f a1 = Vec4f(a);
    Vec4f a2 = a1 * a1;
#if  INSTRSET >= 3  // SSE3
    __m128 t2 = _mm_hadd_ps(a2,a2);
#else
    __m128 t1 = _mm_shuffle_ps(a2,a2,1);
    __m128 t2 = _mm_add_ss(t1,a2);
#endif
    float  a3 = _mm_cvtss_f32(t2);
    return sqrtf(a3);
}

// function sqrt: square root
static inline Complex2f sqrt(Complex2f const & a) {
    __m128 t1  = _mm_mul_ps(a,a);             // r*r, i*i
    __m128 t2  = _mm_shuffle_ps(t1,t1,0xB1);  // swap real and imaginary parts
    __m128 t3  = _mm_add_ps(t1,t2);           // pairwise horizontal sum
    __m128 t4  = _mm_sqrt_ps(t3);             // n = sqrt(r*r+i*i)
    __m128 t5  = _mm_shuffle_ps(a,a,0xA0);    // copy real part of a
    __m128 sbithi = _mm_castsi128_ps(constant4i<0,0x80000000,0,0x80000000>());  // 0.0, -0.0, 0.0, -0.0
    __m128 t6  = _mm_xor_ps(t5, sbithi);      // r, -r
    __m128 t7  = _mm_add_ps(t4,t6);           // n+r, n-r
    __m128 t8  = _mm_sqrt_ps(t7);             // sqrt(n+r), sqrt(n-r)
    __m128 t9  = _mm_and_ps(a,sbithi);        // 0, signbit of i
    __m128 t10 = _mm_xor_ps(t8, t9);          // sqrt(n+r), sign(i)*sqrt(n-r)
    __m128 t11 = Vec4f(0.7071067811865f, 0.7071067811865f, 0.f, 0.f);  // 1/sqrt(2)
    return _mm_mul_ps(t10, t11);
}

// function select
static inline Complex2f select (bool s, Complex2f const & a, Complex2f const & b) {
    return s ? a : b;
}


/*****************************************************************************
*
*               Class Complex4f: two single precision complex numbers
*
*****************************************************************************/

class Complex4f {
protected:
    __m128 xmm; // vector of 4 single precision floats
public:
    // default constructor
    Complex4f() {
    }
    // construct from real and imaginary parts
    Complex4f(float re0, float im0, float re1, float im1) {
        xmm = Vec4f(re0, im0, re1, im1);
    }
    // construct from real, no imaginary part
    Complex4f(float re) {
        xmm = Vec4f(re, 0.f, re, 0.f);
    }
    // construct from real and imaginary part, broadcast to all
    Complex4f(float re, float im) {
        xmm = Vec4f(re, im, re, im);
    }
    // construct from two Complex2f
    Complex4f(Complex2f const & a0, Complex2f const & a1) {
        xmm = _mm_movelh_ps(a0, a1);
    }
    // Constructor to convert from type __m128 used in intrinsics:
    Complex4f(__m128 const & x) {
        xmm = x;
    }
    // Assignment operator to convert from type __m128 used in intrinsics:
    Complex4f & operator = (__m128 const & x) {
        xmm = x;
        return *this;
    }
    // Type cast operator to convert to __m128 used in intrinsics
    operator __m128() const {
        return xmm;
    }
    // Member function to convert to vector
    Vec4f to_vector() const {
        return xmm;
    }
    // Member function to load from array (unaligned)
    Complex4f & load(float const * p) {
        xmm = Vec4f().load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Complex4f const & load_a(float const * p) {
        xmm = Vec4f().load_a(p);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(float * p) const {
        Vec4f(xmm).store(p);
    }
    // Member function to store into array, aligned by 16
    void store_a(float * p) const {
        Vec4f(xmm).store_a(p);
    }
    // Member functions to split into two Complex2f:
    Complex2f get_low() const {
        return Complex2f(Vec4f(xmm).cutoff(2));
    }
    Complex2f get_high() const {
        __m128 t = _mm_movehl_ps(_mm_setzero_ps(), xmm);
        return Complex2f(t);
    }
    // Member function to extract one real or imaginary part
    float extract(uint32_t index) const {
        float x[4];
        Vec4f(xmm).store(x);
        return x[index & 3];
    }
};

/*****************************************************************************
*
*          Operators for Complex4f
*
*****************************************************************************/

// operator + : add
static inline Complex4f operator + (Complex4f const & a, Complex4f const & b) {
    return Complex4f(Vec4f(a) + Vec4f(b));
}

// operator += : add
static inline Complex4f & operator += (Complex4f & a, Complex4f const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex4f operator - (Complex4f const & a, Complex4f const & b) {
    return Complex4f(Vec4f(a) - Vec4f(b));
}

// operator - : unary minus
static inline Complex4f operator - (Complex4f const & a) {
    return Complex4f(- Vec4f(a));
}

// operator -= : subtract
static inline Complex4f & operator -= (Complex4f & a, Complex4f const & b) {
    a = a - b;
    return a;
}

// operator * : complex multiply is defined as
// (a.re * b.re - a.im * b.im,  a.re * b.im + b.re * a.im)
static inline Complex4f operator * (Complex4f const & a, Complex4f const & b) {
    __m128 b_flip = _mm_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m128 a_im   = _mm_shuffle_ps(a,a,0xF5);   // Imag. part of a in both
    __m128 a_re   = _mm_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m128 aib    = _mm_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
#ifdef __FMA__      // FMA3
    return  _mm_fmaddsub_ps(a_re, b, aib);      // a_re * b +/- aib
#elif defined (__FMA4__)  // FMA4
    return  _mm_maddsub_ps (a_re, b, aib);      // a_re * b +/- aib
#elif  INSTRSET >= 3  // SSE3
    __m128 arb    = _mm_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    return _mm_addsub_ps(arb, aib);             // subtract/add
#else
    __m128 arb     = _mm_mul_ps(a_re, b);       // (a.re*b.re, a.re*b.im)
    __m128 aib_m   = change_sign<1,0,1,0>(Vec4f(aib)); // change sign of low part
    return _mm_add_ps(arb, aib_m);              // add
#endif
}

// operator *= : multiply
static inline Complex4f & operator *= (Complex4f & a, Complex4f const & b) {
    a = a * b;
    return a;
}

// operator / : complex divide is defined as
// (a.re * b.re + a.im * b.im, b.re * a.im - a.re * b.im) / (b.re * b.re + b.im * b.im)
static inline Complex4f operator / (Complex4f const & a, Complex4f const & b) {
    // The following code is made similar to the operator * to enable common 
    // subexpression elimination in code that contains both operator * and 
    // operator / where one or both operands are the same
    __m128 a_re   = _mm_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m128 arb    = _mm_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m128 b_flip = _mm_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m128 a_im   = _mm_shuffle_ps(a,a,0xF5);   // Imag. part of a in both
#ifdef __FMA__      // FMA3
    __m128 n      = _mm_fmsubadd_ps(a_im, b_flip, arb); 
#elif defined (__FMA4__)  // FMA4
    __m128 n      = _mm_msubadd_ps (a_im, b_flip, arb);
#else
    __m128 aib    = _mm_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    __m128 arbm   = change_sign<0,1,0,1>(Vec4f(arb)); // change sign of high part
    __m128 n      = _mm_add_ps(arbm, aib);      // arbm + aib
#endif  // FMA
    __m128 bb     = _mm_mul_ps(b, b);           // (b.re*b.re, b.im*b.im)
    __m128 bb1    = _mm_shuffle_ps(bb,bb,0xB1); // Swap bb.re and bb.im
    __m128 bb2    = _mm_add_ps(bb,bb1);         // add pairwise into both positions
    return          _mm_div_ps(n, bb2);         // n / bb3
}

// operator /= : divide
static inline Complex4f & operator /= (Complex4f & a, Complex4f const & b) {
    a = a / b;
    return a;
}

// operator ~ : complex conjugate
// ~(a,b) = (a,-b)
static inline Complex4f operator ~ (Complex4f const & a) {
    return Complex4f(change_sign<0,1,0,1>(Vec4f(a))); // change sign of high part
}

// operator == : returns true if a == b
static inline Vec2db operator == (Complex4f const & a, Complex4f const & b) {
    Vec4fb t1 = Vec4f(a) == Vec4f(b);
    Vec4fb t2 = _mm_shuffle_ps(t1, t1, 0xB1); // swap real and imaginary parts
    return _mm_castps_pd(t1 & t2);
}

// operator != : returns true if a != b
static inline Vec2db operator != (Complex4f const & a, Complex4f const & b) {
    Vec4fb t1 = Vec4f(a) != Vec4f(b);
    Vec4fb t2 = _mm_shuffle_ps(t1, t1, 0xB1); // swap real and imaginary parts
    return _mm_castps_pd(t1 | t2);
}

/*****************************************************************************
*
*          Operators mixing Complex4f and float
*
*****************************************************************************/

// operator + : add
static inline Complex4f operator + (Complex4f const & a, float b) {
    return a + Complex4f(b);
}
static inline Complex4f operator + (float a, Complex4f const & b) {
    return b + a;
}
static inline Complex4f & operator += (Complex4f & a, float & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex4f operator - (Complex4f const & a, float b) {
    return a - Complex4f(b);
}
static inline Complex4f operator - (float a, Complex4f const & b) {
    return Complex4f(a) - b;
}
static inline Complex4f & operator -= (Complex4f & a, float & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Complex4f operator * (Complex4f const & a, float b) {
    return _mm_mul_ps(a, _mm_set1_ps(b));
}
static inline Complex4f operator * (float a, Complex4f const & b) {
    return b * a;
}
static inline Complex4f & operator *= (Complex4f & a, float & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Complex4f operator / (Complex4f const & a, float b) {
    return _mm_div_ps(a, _mm_set1_ps(b));
}

static inline Complex4f operator / (float a, Complex4f const & b) {
    Vec4f b2(b);
    Vec4f b3 = b2 * b2;
    Vec4f t1 = _mm_shuffle_ps(b3,b3,0xB1); // swap real and imaginary parts
    Vec4f t2 = t1 + b3;
    Vec4f t3 = Vec4f(a) / t2;
    Vec4f t4 = Vec4f(~b) * t3;
    return Complex4f(t4);
}

static inline Complex4f & operator /= (Complex4f & a, float b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Complex4f
*
*****************************************************************************/

// function abs: absolute value
// abs(a,b) = sqrt(a*a+b*b);
static inline Complex4f abs(Complex4f const & a) {
    Vec4f a1 = Vec4f(a);
    Vec4f a2 = a1 * a1;
    Vec4f t1 = _mm_shuffle_ps(a2,a2,0xB1); // swap real and imaginary parts
    Vec4f t2 = t1 + a2;
    Vec4f mask = _mm_castsi128_ps(constant4i<-1,0,-1,0>());
    Vec4f t3 = t2 & mask;                  // set imaginary parts to zero
    Vec4f t4 = sqrt(t3);
    return Complex4f(t4);
}

// function sqrt: square root
static inline Complex4f sqrt(Complex4f const & a) {
    __m128 t1  = _mm_mul_ps(a,a);             // r*r, i*i
    __m128 t2  = _mm_shuffle_ps(t1,t1,0xB1);  // swap real and imaginary parts
    __m128 t3  = _mm_add_ps(t1,t2);           // pairwise horizontal sum
    __m128 t4  = _mm_sqrt_ps(t3);             // n = sqrt(r*r+i*i)
    __m128 t5  = _mm_shuffle_ps(a,a,0xA0);    // copy real part of a
    __m128 sbithi = _mm_castsi128_ps(constant4i<0,0x80000000,0,0x80000000>());  // 0.0, -0.0, 0.0, -0.0
    __m128 t6  = _mm_xor_ps(t5, sbithi);      // r, -r
    __m128 t7  = _mm_add_ps(t4,t6);           // n+r, n-r
    __m128 t8  = _mm_sqrt_ps(t7);             // sqrt(n+r), sqrt(n-r)
    __m128 t9  = _mm_and_ps(a,sbithi);        // 0, signbit of i
    __m128 t10 = _mm_xor_ps(t8, t9);          // sqrt(n+r), sign(i)*sqrt(n-r)
    __m128 t11 = Vec4f(0.7071067811865f);  // 1/sqrt(2)
    return _mm_mul_ps(t10, t11);
}

// function select
static inline Complex4f select (Vec2db const & s, Complex4f const & a, Complex4f const & b) {
    return Complex4f(select(_mm_castpd_ps(s), Vec4f(a), Vec4f(b)));
}


/*****************************************************************************
*
*               Class Complex8f: eight single precision complex numbers
*
*****************************************************************************/

class Complex8f {
protected:
    Vec8f y; // vector of 8 floats
public:
    // default constructor
    Complex8f() {
    }
    // construct from real and imaginary parts
    Complex8f(float re0, float im0, float re1, float im1, float re2, float im2, float re3, float im3) 
        : y(re0, im0, re1, im1, re2, im2, re3, im3) {}
    // construct from one Complex2f, broadcast to all
    Complex8f(Complex2f const & a) {
#if INSTRSET >= 7  // AVX
        y = _mm256_castpd_ps(_mm256_broadcast_sd((double*)&a));
#else
        Complex4f a2(a, a);        
        y = Vec8f(Vec4f(a2), Vec4f(a2));
#endif
    }
    // construct from two Complex4f
    Complex8f(Complex4f const & a, Complex4f const & b) {
        y = Vec8f(Vec4f(a), Vec4f(b));
    }
    // construct from four Complex2f
    Complex8f(Complex2f const & a0, Complex2f const & a1, Complex2f const & a2, Complex2f const & a3) {
        *this = Complex8f(Complex4f(a0,a1), Complex4f(a2,a3));
    }
    // construct from real, no imaginary part
    Complex8f(float re) {
        Complex4f a2(re);
        *this = Complex8f(a2, a2);
    }
    // construct from real and imaginary part, broadcast to all
    Complex8f(float re, float im) {
        Complex4f t(re, im);
        *this = Complex8f(t, t);
    }
    // Constructor to convert from type __m256 used in intrinsics or Vec256fe used in emulation
#if INSTRSET >= 7  // AVX
    Complex8f(__m256 const & x) {
        y = x;
#else
    Complex8f(Vec256fe const & x) {
        y = x;
#endif
    }
    // Assignment operator to convert from type __m256 used in intrinsics or Vec256fe used in emulation
#if INSTRSET >= 7  // AVX
    Complex8f & operator = (__m256 const & x) {
#else
    Complex8f & operator = (Vec256fe const & x) {
#endif
        *this = Complex8f(x);
        return *this;
    }
    // Type cast operator to convert to __m256 used in intrinsics or Vec256fe used in emulation
#if INSTRSET >= 7  // AVX
    operator __m256() const {
#else
    operator Vec256fe() const {
#endif
        return y;
    }
    // Member function to convert to vector
    Vec8f to_vector() const {
        return y;
    }
    // Member function to load from array (unaligned)
    Complex8f & load(float const * p) {
        y = Vec8f().load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Complex8f const & load_a(float const * p) {
        y = Vec8f().load_a(p);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(float * p) const {
        y.store(p);
    }
    // Member function to store into array, aligned by 16
    void store_a(float * p) const {
        y.store_a(p);
    }
    // Member functions to split into two Complex2f:
    Complex4f get_low() const {
        return Complex4f(y.get_low());
    }
    Complex4f get_high() const {
        return Complex4f(y.get_high());
    }
    // Member function to extract a single real or imaginary part
    float extract(uint32_t index) const {
        return y.extract(index);
    }
};


/*****************************************************************************
*
*          Operators for Complex8f
*
*****************************************************************************/

// operator + : add
static inline Complex8f operator + (Complex8f const & a, Complex8f const & b) {
    return Complex8f(Vec8f(a) + Vec8f(b));
}

// operator += : add
static inline Complex8f & operator += (Complex8f & a, Complex8f const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex8f operator - (Complex8f const & a, Complex8f const & b) {
    return Complex8f(Vec8f(a) - Vec8f(b));
}

// operator - : unary minus
static inline Complex8f operator - (Complex8f const & a) {
    return Complex8f(- Vec8f(a));
}

// operator -= : subtract
static inline Complex8f & operator -= (Complex8f & a, Complex8f const & b) {
    a = a - b;
    return a;
}

// operator * : complex multiply is defined as
// (a.re * b.re - a.im * b.im,  a.re * b.im + b.re * a.im)
static inline Complex8f operator * (Complex8f const & a, Complex8f const & b) {
#if INSTRSET >= 7  // AVX
    __m256 b_flip = _mm256_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m256 a_im   = _mm256_shuffle_ps(a,a,0xF5);   // Imag part of a in both
    __m256 a_re   = _mm256_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m256 aib    = _mm256_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
#ifdef __FMA__      // FMA3
    return  _mm256_fmaddsub_ps(a_re, b, aib);      // a_re * b +/- aib
#elif defined (__FMA4__)  // FMA4
    return  _mm256_maddsub_ps (a_re, b, aib);      // a_re * b +/- aib
#else
    __m256 arb    = _mm256_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    return          _mm256_addsub_ps(arb, aib);    // subtract/add
#endif  // FMA
#else   // AVX
    return Complex8f(a.get_low()*b.get_low(), a.get_high()*b.get_high());
#endif
}

// operator *= : multiply
static inline Complex8f & operator *= (Complex8f & a, Complex8f const & b) {
    a = a * b;
    return a;
}

// operator / : complex divide is defined as
// (a.re * b.re + a.im * b.im, b.re * a.im - a.re * b.im) / (b.re * b.re + b.im * b.im)
static inline Complex8f operator / (Complex8f const & a, Complex8f const & b) {
#if INSTRSET >= 7  // AVX
    // The following code is made similar to the operator * to enable common 
    // subexpression elimination in code that contains both operator * and 
    // operator / where one or both operands are the same
    __m256 a_re   = _mm256_shuffle_ps(a,a,0xA0);   // Real part of a in both
    __m256 arb    = _mm256_mul_ps(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m256 b_flip = _mm256_shuffle_ps(b,b,0xB1);   // Swap b.re and b.im
    __m256 a_im   = _mm256_shuffle_ps(a,a,0xF5);   // Imag part of a in both
#ifdef __FMA__      // FMA3
    __m256 n      = _mm256_fmsubadd_ps(a_im, b_flip, arb); 
#elif defined (__FMA4__)  // FMA4
    __m256 n      = _mm256_msubadd_ps (a_im, b_flip, arb);
#else
    __m256 aib    = _mm256_mul_ps(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    __m256 arbm   = change_sign<0,1,0,1,0,1,0,1>(Vec8f(arb)); // (a.re*b.re,-a.re*b.im)
    __m256 n      = _mm256_add_ps(arbm, aib);      // arbm + aib
#endif  // FMA
    __m256 bb     = _mm256_mul_ps(b, b);           // (b.re*b.re, b.im*b.im)
    __m256 bb2    = _mm256_shuffle_ps(bb,bb,0xB1); // Swap bb.re and bb.im
    __m256 bsq    = _mm256_add_ps(bb,bb2);         // (b.re*b.re + b.im*b.im) dublicated
    return          _mm256_div_ps(n, bsq);         // n / bsq
#else
    return Complex8f(a.get_low()/b.get_low(), a.get_high()/b.get_high());
#endif
}

// operator /= : divide
static inline Complex8f & operator /= (Complex8f & a, Complex8f const & b) {
    a = a / b;
    return a;
}

// operator ~ : complex conjugate
// ~(a,b) = (a,-b)
static inline Complex8f operator ~ (Complex8f const & a) {
    return Complex8f(change_sign<0,1,0,1,0,1,0,1>(Vec8f(a)));
}

// operator == : returns true if a == b
static inline Vec4db operator == (Complex8f const & a, Complex8f const & b) {
    Vec8fb eq0 = Vec8f(a) == Vec8f(b);
    Vec8fb eq1 = (Vec8fb)permute8f<1,0,3,2,5,4,7,6>(Vec8f(eq0));
    return reinterpret_d(Vec8f(eq0 & eq1));
}

// operator != : returns true if a != b
static inline Vec4db operator != (Complex8f const & a, Complex8f const & b) {
    return ~(a == b);
}

/*****************************************************************************
*
*          Operators mixing Complex8f and float
*
*****************************************************************************/

// operator + : add
static inline Complex8f operator + (Complex8f const & a, float b) {
    return a + Complex8f(b);
}
static inline Complex8f operator + (float a, Complex8f const & b) {
    return b + a;
}
static inline Complex8f & operator += (Complex8f & a, float & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex8f operator - (Complex8f const & a, float b) {
    return a - Complex8f(b);
}
static inline Complex8f operator - (float a, Complex8f const & b) {
    return Complex8f(a) - b;
}
static inline Complex8f & operator -= (Complex8f & a, float & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Complex8f operator * (Complex8f const & a, float b) {
    return Complex8f(Vec8f(a) * Vec8f(b));
}
static inline Complex8f operator * (float a, Complex8f const & b) {
    return b * a;
}
static inline Complex8f & operator *= (Complex8f & a, float & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Complex8f operator / (Complex8f const & a, float b) {
    return Complex8f(Vec8f(a) / Vec8f(b));
}

static inline Complex8f operator / (float a, Complex8f const & b) {
#if INSTRSET >= 7  // AVX
    Vec8f b2(b);
    Vec8f b3 = b2 * b2;
    Vec8f t1 = _mm256_shuffle_ps(b3,b3,0xB1); // swap real and imaginary parts
    Vec8f t2 = t1 + b3;
    Vec8f t3 = Vec8f(a) / t2;
    Vec8f t4 = Vec8f(~b) * t3;
    return Complex8f(t4);
#else
    return Complex8f(a / b.get_low(), a / b.get_high());
#endif
}
static inline Complex8f & operator /= (Complex8f & a, float b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Complex8f
*
*****************************************************************************/

// function abs: absolute value
// abs(a,b) = sqrt(a*a+b*b);
static inline Complex8f abs(Complex8f const & a) {
#if INSTRSET >= 7  // AVX
    Vec8f a1 = Vec8f(a);
    Vec8f a2 = a1 * a1;
    Vec8f t1 = _mm256_shuffle_ps(a2,a2,0xB1); // swap real and imaginary parts
    Vec8f t2 = t1 + a2;
    Vec8f mask = constant8f<-1,0,-1,0,-1,0,-1,0>();
    Vec8f t3 = t2 & mask;                  // set imaginary parts to zero
    Vec8f t4 = sqrt(t3);
    return Complex8f(t4);
#else
    return Complex8f(abs(a.get_low()), abs(a.get_high()));
#endif
}

// function sqrt: square root
static inline Complex8f sqrt(Complex8f const & a) {
#if INSTRSET >= 7  // AVX
    __m256 t1  = _mm256_mul_ps(a,a);             // r*r, i*i
    __m256 t2  = _mm256_shuffle_ps(t1,t1,0xB1);  // swap real and imaginary parts
    __m256 t3  = _mm256_add_ps(t1,t2);           // pairwise horizontal sum
    __m256 t4  = _mm256_sqrt_ps(t3);             // n = sqrt(r*r+i*i)
    __m256 t5  = _mm256_shuffle_ps(a,a,0xA0);    // copy real part of a
    __m256 sbithi = constant8f<0,0x80000000,0,0x80000000,0,0x80000000,0,0x80000000> ();
    __m256 t6  = _mm256_xor_ps(t5, sbithi);      // r, -r
    __m256 t7  = _mm256_add_ps(t4,t6);           // n+r, n-r
    __m256 t8  = _mm256_sqrt_ps(t7);             // sqrt(n+r), sqrt(n-r)
    __m256 t9  = _mm256_and_ps(a,sbithi);        // 0, signbit of i
    __m256 t10 = _mm256_xor_ps(t8, t9);          // sqrt(n+r), sign(i)*sqrt(n-r)
    __m256 t11 = Vec8f(0.7071067811865f);  // 1/sqrt(2)
    return _mm256_mul_ps(t10, t11);
#else
    return Complex8f(sqrt(a.get_low()), sqrt(a.get_high()));
#endif
}

// function select.
// Note: s must contain a vector of flor booleans of 64 bits each. 
static inline Complex8f select (Vec4db const & s, Complex8f const & a, Complex8f const & b) {
    return Complex8f(select(Vec8fb(reinterpret_f(Vec4d(s))), Vec8f(a), Vec8f(b)));
}


/*****************************************************************************
*
*               Class Complex2d: one double precision complex number
*
*****************************************************************************/

class Complex2d {
protected:
    __m128d xmm; // double vector
public:
    // default constructor
    Complex2d() {
    }
    // construct from real and imaginary part
    Complex2d(double re, double im) {
        xmm = Vec2d(re, im);
    }
    // construct from real, no imaginary part
    Complex2d(double re) {
        xmm = _mm_load_sd(&re);
    }
    // Constructor to convert from type __m128d used in intrinsics:
    Complex2d(__m128d const & x) {
        xmm = x;
    }
    // Assignment operator to convert from type __m128d used in intrinsics:
    Complex2d & operator = (__m128d const & x) {
        xmm = x;
        return *this;
    }
    // Type cast operator to convert to __m128d used in intrinsics
    operator __m128d() const {
        return xmm;
    }
    // Member function to convert to vector
    Vec2d to_vector() const {
        return xmm;
    }
    // Member function to load from array (unaligned)
    Complex2d & load(double const * p) {
        xmm = Vec2d().load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Complex2d const & load_a(double const * p) {
        xmm = Vec2d().load_a(p);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(double * p) const {
        Vec2d(xmm).store(p);
    }
    // Member function to store into array, aligned by 16
    void store_a(double * p) const {
        Vec2d(xmm).store_a(p);
    }
    // get real part
    double real() const {
        return _mm_cvtsd_f64(xmm);
    }
    // get imaginary part
    double imag() const {
        double t;
        _mm_storeh_pd(&t, xmm);
        return t;
    }
    // Member function to extract real or imaginary part
    double extract(uint32_t index) const {
        double x[2];
        store(x);
        return x[index & 1];
    }
};


/*****************************************************************************
*
*          Operators for Complex2d
*
*****************************************************************************/

// operator + : add
static inline Complex2d operator + (Complex2d const & a, Complex2d const & b) {
    return Complex2d(Vec2d(a) + Vec2d(b));
}

// operator += : add
static inline Complex2d & operator += (Complex2d & a, Complex2d const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex2d operator - (Complex2d const & a, Complex2d const & b) {
    return Complex2d(Vec2d(a) - Vec2d(b));
}

// operator - : unary minus
static inline Complex2d operator - (Complex2d const & a) {
    return Complex2d(- Vec2d(a));
}

// operator -= : subtract
static inline Complex2d & operator -= (Complex2d & a, Complex2d const & b) {
    a = a - b;
    return a;
}

// operator * : complex multiply is defined as
// (a.re * b.re - a.im * b.im,  a.re * b.im + b.re * a.im)
static inline Complex2d operator * (Complex2d const & a, Complex2d const & b) {
    __m128d b_flip = _mm_shuffle_pd(b,b,1);      // Swap b.re and b.im
    __m128d a_im   = _mm_shuffle_pd(a,a,3);      // Imag. part of a in both
    __m128d a_re   = _mm_shuffle_pd(a,a,0);      // Real part of a in both
    __m128d aib    = _mm_mul_pd(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
#ifdef __FMA__      // FMA3
    return  _mm_fmaddsub_pd(a_re, b, aib);       // a_re * b +/- aib
#elif defined (__FMA4__)  // FMA4
    return  _mm_maddsub_pd (a_re, b, aib);       // a_re * b +/- aib
#elif  INSTRSET >= 3  // SSE3
    __m128d arb    = _mm_mul_pd(a_re, b);        // (a.re*b.re, a.re*b.im)
    return _mm_addsub_pd(arb, aib);              // subtract/add
#else
    __m128d arb    = _mm_mul_pd(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m128d aib_m  = change_sign<1,0>(Vec2d(aib)); // change sign of low part
    return _mm_add_pd(arb, aib_m);               // add
#endif
}

// operator *= : multiply
static inline Complex2d & operator *= (Complex2d & a, Complex2d const & b) {
    a = a * b;
    return a;
}

// operator / : complex divide is defined as
// (a.re * b.re + a.im * b.im, b.re * a.im - a.re * b.im) / (b.re * b.re + b.im * b.im)
static inline Complex2d operator / (Complex2d const & a, Complex2d const & b) {
    // The following code is made similar to the operator * to enable common 
    // subexpression elimination in code that contains both operator * and 
    // operator / where one or both operands are the same
    __m128d a_re   = _mm_shuffle_pd(a,a,0);      // Real part of a in both
    __m128d arb    = _mm_mul_pd(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m128d b_flip = _mm_shuffle_pd(b,b,1);      // Swap b.re and b.im
    __m128d a_im   = _mm_shuffle_pd(a,a,3);      // Imag. part of a in both
#ifdef __FMA__      // FMA3
    __m128d n      = _mm_fmsubadd_pd(a_im, b_flip, arb); 
#elif defined (__FMA4__)  // FMA4
    __m128d n      = _mm_msubadd_pd (a_im, b_flip, arb);
#else
    __m128d aib    = _mm_mul_pd(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    __m128d arbm   = change_sign<0,1>(Vec2d(arb));
    __m128d n      = _mm_add_pd(arbm, aib);      // arbm + aib
#endif  // FMA
    __m128d bb     = _mm_mul_pd(b, b);           // (b.re*b.re, b.im*b.im)
#if INSTRSET >= 3  // SSE3
    __m128d bsq    = _mm_hadd_pd(bb,bb);         // (b.re*b.re + b.im*b.im) dublicated
#else
    __m128d bb1    = _mm_shuffle_pd(bb,bb,1);
    __m128d bsq    = _mm_add_pd(bb,bb1);
#endif // SSE3
    return           _mm_div_pd(n, bsq);         // n / bsq
}

// operator /= : divide
static inline Complex2d & operator /= (Complex2d & a, Complex2d const & b) {
    a = a / b;
    return a;
}

// operator ~ : complex conjugate
// ~(a,b) = (a,-b)
static inline Complex2d operator ~ (Complex2d const & a) {
    return Complex2d(change_sign<0,1>(Vec2d(a)));
}

// operator == : returns true if a == b
static inline bool operator == (Complex2d const & a, Complex2d const & b) {
    return horizontal_and(Vec2d(a) == Vec2d(b));
}

// operator != : returns true if a != b
static inline bool operator != (Complex2d const & a, Complex2d const & b) {
    return horizontal_or(Vec2d(a) != Vec2d(b));
}

/*****************************************************************************
*
*          Operators mixing Complex2d and double
*
*****************************************************************************/

// operator + : add
static inline Complex2d operator + (Complex2d const & a, double b) {
    return _mm_add_sd(a, _mm_set_sd(b));
}
static inline Complex2d operator + (double a, Complex2d const & b) {
    return b + a;
}
static inline Complex2d & operator += (Complex2d & a, double & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex2d operator - (Complex2d const & a, double b) {
    return _mm_sub_sd(a, _mm_set_sd(b));
}
static inline Complex2d operator - (double a, Complex2d const & b) {
    return Complex2d(a) - b;
}
static inline Complex2d & operator -= (Complex2d & a, double & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Complex2d operator * (Complex2d const & a, double b) {
    return _mm_mul_pd(a, _mm_set1_pd(b));
}
static inline Complex2d operator * (double a, Complex2d const & b) {
    return b * a;
}
static inline Complex2d & operator *= (Complex2d & a, double & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Complex2d operator / (Complex2d const & a, double b) {
    return _mm_div_pd(a, _mm_set1_pd(b));
}
static inline Complex2d operator / (double a, Complex2d const & b) {
    Vec2d b2(b);
    double b3 = horizontal_add(b2 * b2);
    return ~b * (a / b3);
}
static inline Complex2d & operator /= (Complex2d & a, double b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Complex2d
*
*****************************************************************************/

// function abs: absolute value
// abs(a,b) = sqrt(a*a+b*b);
static inline double abs(Complex2d const & a) {
    Vec2d t = Vec2d(a);
    return sqrt(horizontal_add(t*t));
}

// function sqrt: square root
static inline Complex2d sqrt(Complex2d const & a) {
    __m128d t1  = _mm_mul_pd(a,a);             // r*r, i*i
    __m128d t2  = _mm_shuffle_pd(t1,t1,1);     // swap real and imaginary parts
    __m128d t3  = _mm_add_pd(t1,t2);           // pairwise horizontal sum
    __m128d t4  = _mm_sqrt_pd(t3);             // n = sqrt(r*r+i*i)
    __m128d t5  = _mm_shuffle_pd(a,a,0);       // copy real part of a
    __m128d sbithi = _mm_castsi128_pd(constant4i<0,0,0,0x80000000>());  // 0.0, -0.0
    __m128d t6  = _mm_xor_pd(t5, sbithi);      // r, -r
    __m128d t7  = _mm_add_pd(t4,t6);           // n+r, n-r
    __m128d t8  = _mm_sqrt_pd(t7);             // sqrt(n+r), sqrt(n-r)
    __m128d t9  = _mm_and_pd(a,sbithi);        // 0, signbit of i
    __m128d t10 = _mm_xor_pd(t8, t9);          // sqrt(n+r), sign(i)*sqrt(n-r)
    __m128d t11 = Vec2d(0.707106781186547524401);  // 1/sqrt(2)
    return        _mm_mul_pd(t10, t11);
}

// function select
static inline Complex2d select (bool s, Complex2d const & a, Complex2d const & b) {
    return s ? a : b;
}


/*****************************************************************************
*
*               Class Complex4d: one double precision complex number
*
*****************************************************************************/

class Complex4d {
protected:
    Vec4d y; // vector of 4 doubles
public:
    // default constructor
    Complex4d() {
    }
    // construct from real and imaginary parts
    Complex4d(double re0, double im0, double re1, double im1) 
        : y(re0, im0, re1, im1) {}
    // construct from one Complex2d
    Complex4d(Complex2d const & a) {
#if INSTRSET >= 7  // AVX
        __m128d aa = a;
        y = _mm256_broadcast_pd(&aa);  // VBROADCASTF128
#else
        y = Vec4d(Vec2d(a), Vec2d(a));
#endif
    }
    // construct from two Complex2d
    Complex4d(Complex2d const & a, Complex2d const & b) {
        y = Vec4d(Vec2d(a), Vec2d(b));
    }
    // construct from real, no imaginary part
    Complex4d(double re) {
        *this = Complex4d(Complex2d(re), Complex2d(re));
    }
    // construct from real and imaginary part, broadcast to all
    Complex4d(double re, double im) {
        *this = Complex4d(Complex2d(re, im));
    }
    // Constructor to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Complex4d(__m256d const & x) {
        y = x;
#else
    Complex4d(Vec256de const & x) {
        y = x;
#endif
    }
    // Assignment operator to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Complex4d & operator = (__m256d const & x) {
#else
    Complex4d & operator = (Vec256de const & x) {
#endif
        *this = Complex4d(x);
        return *this;
    }
    // Type cast operator to convert to __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    operator __m256d() const {
#else
    operator Vec256de() const {
#endif
        return y;
    }
    // Member function to convert to vector
    Vec4d to_vector() const {
        return y;
    }
    // Member function to load from array (unaligned)
    Complex4d & load(double const * p) {
        y = Vec4d().load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Complex4d const & load_a(double const * p) {
        y = Vec4d().load_a(p);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(double * p) const {
        y.store(p);
    }
    // Member function to store into array, aligned by 16
    void store_a(double * p) const {
        y.store_a(p);
    }
    // Member functions to split into two Complex2d:
    Complex2d get_low() const {
        return Complex2d(y.get_low());
    }
    Complex2d get_high() const {
        return Complex2d(y.get_high());
    }
    // Member function to extract a single real or imaginary part
    double extract(uint32_t index) const {
        return y.extract(index);
    }
};


/*****************************************************************************
*
*          Operators for Complex4d
*
*****************************************************************************/

// operator + : add
static inline Complex4d operator + (Complex4d const & a, Complex4d const & b) {
    return Complex4d(Vec4d(a) + Vec4d(b));
}

// operator += : add
static inline Complex4d & operator += (Complex4d & a, Complex4d const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex4d operator - (Complex4d const & a, Complex4d const & b) {
    return Complex4d(Vec4d(a) - Vec4d(b));
}

// operator - : unary minus
static inline Complex4d operator - (Complex4d const & a) {
    return Complex4d(- Vec4d(a));
}

// operator -= : subtract
static inline Complex4d & operator -= (Complex4d & a, Complex4d const & b) {
    a = a - b;
    return a;
}

// operator * : complex multiply is defined as
// (a.re * b.re - a.im * b.im,  a.re * b.im + b.re * a.im)
static inline Complex4d operator * (Complex4d const & a, Complex4d const & b) {
#if INSTRSET >= 7  // AVX
    __m256d b_flip = _mm256_shuffle_pd(b,b,5);      // Swap b.re and b.im
    __m256d a_im   = _mm256_shuffle_pd(a,a,0xF);    // Imag part of a in both
    __m256d a_re   = _mm256_shuffle_pd(a,a,0);      // Real part of a in both
    __m256d aib    = _mm256_mul_pd(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
#ifdef __FMA__      // FMA3
    return  _mm256_fmaddsub_pd(a_re, b, aib);       // a_re * b +/- aib
#elif defined (__FMA4__)  // FMA4
    return  _mm256_maddsub_pd (a_re, b, aib);       // a_re * b +/- aib
#else
    __m256d arb    = _mm256_mul_pd(a_re, b);        // (a.re*b.re, a.re*b.im)
    return           _mm256_addsub_pd(arb, aib);    // subtract/add
#endif // FMA
#else
    return Complex4d(a.get_low()*b.get_low(), a.get_high()*b.get_high());
#endif // AVX
}

// operator *= : multiply
static inline Complex4d & operator *= (Complex4d & a, Complex4d const & b) {
    a = a * b;
    return a;
}

// operator / : complex divide is defined as
// (a.re * b.re + a.im * b.im, b.re * a.im - a.re * b.im) / (b.re * b.re + b.im * b.im)
static inline Complex4d operator / (Complex4d const & a, Complex4d const & b) {
#if INSTRSET >= 7  // AVX
    // The following code is made similar to the operator * to enable common 
    // subexpression elimination in code that contains both operator * and 
    // operator / where one or both operands are the same
    __m256d a_re   = _mm256_shuffle_pd(a,a,0);      // Real part of a in both
    __m256d arb    = _mm256_mul_pd(a_re, b);        // (a.re*b.re, a.re*b.im)
    __m256d b_flip = _mm256_shuffle_pd(b,b,5);      // Swap b.re and b.im
    __m256d a_im   = _mm256_shuffle_pd(a,a,0xF);    // Imag part of a in both
#ifdef __FMA__      // FMA3
    __m256d n      = _mm256_fmsubadd_pd(a_im, b_flip, arb); 
#elif defined (__FMA4__)  // FMA4
    __m256d n      = _mm256_msubadd_pd (a_im, b_flip, arb);
#else
    __m256d aib    = _mm256_mul_pd(a_im, b_flip);   // (a.im*b.im, a.im*b.re)
    __m256d arbm   = change_sign<0,1,0,1>(Vec4d(arb));
    __m256d n      = _mm256_add_pd(arbm, aib);      // arbm + aib
#endif  // FMA
    __m256d bb     = _mm256_mul_pd(b, b);           // (b.re*b.re, b.im*b.im)
    __m256d bsq    = _mm256_hadd_pd(bb,bb);         // (b.re*b.re + b.im*b.im) dublicated
    return           _mm256_div_pd(n, bsq);         // n / bsq
#else
    return Complex4d(a.get_low()/b.get_low(), a.get_high()/b.get_high());
#endif
}

// operator /= : divide
static inline Complex4d & operator /= (Complex4d & a, Complex4d const & b) {
    a = a / b;
    return a;
}

// operator ~ : complex conjugate
// ~(a,b) = (a,-b)
static inline Complex4d operator ~ (Complex4d const & a) {
    return Complex4d(change_sign<0,1,0,1>(Vec4d(a)));
}

// operator == : returns true if a == b
static inline Vec4db operator == (Complex4d const & a, Complex4d const & b) {
    Vec4db eq0 = Vec4d(a) == Vec4d(b);
    Vec4db eq1 = (Vec4db)permute4d<1,0,3,2>(Vec4d(eq0));
    return eq0 & eq1;
}

// operator != : returns true if a != b
static inline Vec4db operator != (Complex4d const & a, Complex4d const & b) {
    return ~(a == b);
}

/*****************************************************************************
*
*          Operators mixing Complex4d and double
*
*****************************************************************************/

// operator + : add
static inline Complex4d operator + (Complex4d const & a, double b) {
    return a + Complex4d(b);
}
static inline Complex4d operator + (double a, Complex4d const & b) {
    return b + a;
}
static inline Complex4d & operator += (Complex4d & a, double & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Complex4d operator - (Complex4d const & a, double b) {
    return a - Complex4d(b);
}
static inline Complex4d operator - (double a, Complex4d const & b) {
    return Complex4d(a) - b;
}
static inline Complex4d & operator -= (Complex4d & a, double & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Complex4d operator * (Complex4d const & a, double b) {
    return Complex4d(Vec4d(a) * Vec4d(b));
}
static inline Complex4d operator * (double a, Complex4d const & b) {
    return b * a;
}
static inline Complex4d & operator *= (Complex4d & a, double & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Complex4d operator / (Complex4d const & a, double b) {
    return Complex4d(Vec4d(a) / Vec4d(b));
}

static inline Complex4d operator / (double a, Complex4d const & b) {
#if INSTRSET >= 7  // AVX
    Vec4d b2 = Vec4d(b) * Vec4d(b);
    Vec4d b3 = _mm256_hadd_pd(b2, b2);       // horizontal add each two elements
    return Complex4d(Vec4d(~b) * (Vec4d(a) / b3));
#else
    return Complex4d(a / b.get_low(), a / b.get_high());
#endif
}
static inline Complex4d & operator /= (Complex4d & a, double b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Complex4d
*
*****************************************************************************/

// function abs: absolute value
// abs(a,b) = sqrt(a*a+b*b);
static inline Complex4d abs(Complex4d const & a) {
#if INSTRSET >= 7  // AVX
    Vec4d a1 = Vec4d(a) * Vec4d(a);
    Vec4d a2 = _mm256_hadd_pd(a1, _mm256_setzero_pd());   // horizontal add each two elements
    return Complex4d(sqrt(a2));
#else
    return Complex4d(abs(a.get_low()), 0., abs(a.get_high()), 0.);
#endif
}

// function sqrt: square root
static inline Complex4d sqrt(Complex4d const & a) {
#if INSTRSET >= 7  // AVX
    __m256d t1  = _mm256_mul_pd(a,a);             // r*r, i*i
    __m256d t2  = _mm256_shuffle_pd(t1,t1,5);     // swap real and imaginary parts
    __m256d t3  = _mm256_add_pd(t1,t2);           // pairwise horizontal sum
    __m256d t4  = _mm256_sqrt_pd(t3);             // n = sqrt(r*r+i*i)
    __m256d t5  = _mm256_shuffle_pd(a,a,0);       // copy real part of a
    __m256d sbithi = _mm256_castps_pd (constant8f<0,0,0,0x80000000,0,0,0,0x80000000>()); // (0.,-0.,0.,-0.)
    __m256d t6  = _mm256_xor_pd(t5, sbithi);      // r, -r
    __m256d t7  = _mm256_add_pd(t4,t6);           // n+r, n-r
    __m256d t8  = _mm256_sqrt_pd(t7);             // sqrt(n+r), sqrt(n-r)
    __m256d t9  = _mm256_and_pd(a,sbithi);        // 0, signbit of i
    __m256d t10 = _mm256_xor_pd(t8, t9);          // sqrt(n+r), sign(i)*sqrt(n-r)
    __m256d t11 = Vec4d(0.707106781186547524401);  // 1/sqrt(2)
    return        _mm256_mul_pd(t10, t11);
#else
    return Complex4d(sqrt(a.get_low()), sqrt(a.get_high()));
#endif
}

// function select.
// Note: s must contain a vector of two booleans of 128 bits each. 
// False = all zeroes, True = all ones. No other values of s are allowed.
// s may come from comparing two Complex4d with operator == or !=.
// If s comes from any other source then make sure the values are consistent.
static inline Complex4d select (Vec4db const & s, Complex4d const & a, Complex4d const & b) {
    return Complex4d(select(s, Vec4d(a), Vec4d(b)));
}


/*****************************************************************************
*
*          Conversion functions
*
*****************************************************************************/

// function to_single: convert Complex2d to Complex2f
static inline Complex2f to_single (Complex2d const & a) {
    return _mm_cvtpd_ps(a);
}

// function to_single: convert Complex4d to Complex4f
static inline Complex4f to_single (Complex4d const & a) {
#if INSTRSET >= 7  // AVX
    //return _mm256_castps256_ps128(_mm256_cvtpd_ps(a));
    return _mm256_cvtpd_ps(a);
#else
    return Complex4f(to_single(a.get_low()), to_single(a.get_high()));
#endif
}

// function to_double: convert Complex2f to Complex2d
static inline Complex2d to_double (Complex2f const & a) {
    return _mm_cvtps_pd(a);
}

// function to_double: convert Complex4f to Complex4d
static inline Complex4d to_double (Complex4f const & a) {
#if INSTRSET >= 7  // AVX
    return _mm256_cvtps_pd(a);
#else
    return Complex4d(to_double(a.get_low()), to_double(a.get_high()));
#endif
}


/*****************************************************************************
*
*          Mathematical functions
*
*****************************************************************************/

// function cexp: complex exponential function
// cexp(re+i*im) = exp(re)*(cos(im)+i*sin(im))

#if defined(VECTORMATH_H) && defined(VECTORMATH) && VECTORMATH >= 2
// cexp available as library function

static inline Complex2f cexp (Complex2f const & a) {
    return Complex2f(__m128(cexp(Vec4f(a))));
}

static inline Complex4f cexp (Complex4f const & a) {
    return Complex4f(cexp(Vec4f(a)));
}

static inline Complex8f cexp (Complex8f const & a) {
    return Complex8f(cexp(a.get_low()), cexp(a.get_high()));
}

static inline Complex2d cexp (Complex2d const & a) {
    return Complex2d(cexp(Vec2d(a)));
}

static inline Complex4d cexp (Complex4d const & a) {
    return Complex4d(cexp(a.get_low()), cexp(a.get_high()));
}

#else   // cexp not available as library function

static inline Complex2f cexp (Complex2f const & a) {   // complex exponential function
    float xx[2], ee;
    a.store(xx);
    Vec4f z(cosf(xx[1]), sinf(xx[1]), 0.f, 0.f); 
    ee = expf(xx[0]);
    return Complex2f(__m128(z * ee));
}

static inline Complex4f cexp (Complex4f const & x) {   // complex exponential function
    float xx[4], ee[2];
    x.store(xx);
    Vec4f z(cosf(xx[1]), sinf(xx[1]), cosf(xx[3]), sinf(xx[3])); 
    ee[0] = expf(xx[0]);  ee[1] = expf(xx[2]);
    return Complex4f(z * Vec4f(ee[0], ee[0], ee[1], ee[1]));
}

static inline Complex8f cexp (Complex8f const & a) {
    return Complex8f(cexp(a.get_low()), cexp(a.get_high()));
}

static inline Complex2d cexp (Complex2d const & a) {
    double xx[2];
    a.store(xx);
    Complex2d z(cos(xx[1]), sin(xx[1]));
    return z * exp(xx[0]);
}

static inline Complex4d cexp (Complex4d const & a) {
    return Complex4d(cexp(a.get_low()), cexp(a.get_high()));
}

#endif  // VECTORMATH_H

#endif  // COMPLEXVEC_H
