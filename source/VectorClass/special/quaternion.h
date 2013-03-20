/***************************  quaternion.h   *********************************
| Author:        Agner Fog
| Date created:  2012-08-01
| Last modified: 2012-08-01
| Version:       1.02 Beta
| Project:       vector classes
| Description:
| Quaternions are used for mathematics and 3-D geometry.
| Classes for quaternions:
| Quaternion4f:  One quaternion consisting of four single precision floats
| Quaternion4d:  One quaternion consisting of four double precision floats
|
| (c) Copyright 2012 GNU General Public License http://www.gnu.org/licenses
\*****************************************************************************/


#ifndef QUATERNION_H
#define QUATERNION_H  102

#include "complexvec.h"  // complex numbers required



/*****************************************************************************
*
*                     Class Quaternion4f
*         One quaternion consisting of four single precision floats
*
*****************************************************************************/

class Quaternion4f {
protected:
    __m128 xmm; // vector of 4 single precision floats
public:
    // default constructor
    Quaternion4f() {
    }
    // construct from real and imaginary parts = re + im0*i + im1*j + im2*k
    Quaternion4f(float re, float im0, float im1, float im2) {
        xmm = Vec4f(re, im0, im1, im2);
    }
    // construct from real, no imaginary part
    Quaternion4f(float re) {
        xmm = _mm_load_ss(&re);
    }
    // construct from two Complex2f = a0 + a1 * j
    Quaternion4f(Complex2f const & a0, Complex2f const & a1) {
        xmm = _mm_movelh_ps(a0, a1);
    }
    // Constructor to convert from type __m128 used in intrinsics:
    Quaternion4f(__m128 const & x) {
        xmm = x;
    }
    // Assignment operator to convert from type __m128 used in intrinsics:
    Quaternion4f & operator = (__m128 const & x) {
        xmm = x;
        return *this;
    }
    // Constructor to convert from Vec4f
    Quaternion4f(Vec4f const & x) {
        xmm = x;
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
    Quaternion4f & load(float const * p) {
        xmm = Vec4f().load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Quaternion4f const & load_a(float const * p) {
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
    // q = q.get_low() + q.get_high()*j
    Complex2f get_low() const {
        return Complex2f(Vec4f(xmm).cutoff(2));
    }
    Complex2f get_high() const {
        __m128 t = _mm_movehl_ps(_mm_setzero_ps(), xmm);
        return Complex2f(t);
    }
    // Member function to extract real part
    float real() const {
        return _mm_cvtss_f32(xmm);
    }
    // Member function to extract imaginary parts, sets real part to 0
    Quaternion4f imag() const {
        return Quaternion4f(permute4f<-1,1,2,3>(Vec4f(xmm)));
    }
    // Member function to extract one real or imaginary part
    float extract(uint32_t index) const {
        float x[4];
        Vec4f(xmm).store(x);
        return x[index & 3];
    }
#ifdef VECTOR3D_H
    // Constructor to convert from Vec3f used in geometrics:
    Quaternion4f(Vec3f const & x) {
        xmm = permute4f<3,0,1,2>(Vec4f(x));  // rotate elements
    }

    // Type cast operator to convert to Vec3f used in geometrics:
    operator Vec3f() const {
        return Vec3f(permute4f<1,2,3,0>(Vec4f(xmm)));  // rotate elements
    }
#endif // VECTOR3D_H 
};


/*****************************************************************************
*
*          Operators for Quaternion4f
*
*****************************************************************************/

// operator + : add
static inline Quaternion4f operator + (Quaternion4f const & a, Quaternion4f const & b) {
    return Quaternion4f(a.to_vector() + b.to_vector());
}

// operator += : add
static inline Quaternion4f & operator += (Quaternion4f & a, Quaternion4f const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Quaternion4f operator - (Quaternion4f const & a, Quaternion4f const & b) {
    return Quaternion4f(a.to_vector() - b.to_vector());
}

// operator - : unary minus
static inline Quaternion4f operator - (Quaternion4f const & a) {
    return Quaternion4f(- a.to_vector());
}

// operator -= : subtract
static inline Quaternion4f & operator -= (Quaternion4f & a, Quaternion4f const & b) {
    a = a - b;
    return a;
}

// operator * : quaternion multiply
static inline Quaternion4f operator * (Quaternion4f const & a, Quaternion4f const & b) {
    __m128 a1123 = _mm_shuffle_ps(a,a,0xE5);
    __m128 a2231 = _mm_shuffle_ps(a,a,0x7A);
    __m128 b1000 = _mm_shuffle_ps(b,b,0x01);
    __m128 b2312 = _mm_shuffle_ps(b,b,0x9E);
    __m128 t1    = _mm_mul_ps(a1123, b1000);
    __m128 t2    = _mm_mul_ps(a2231, b2312);
    __m128 t12   = _mm_add_ps(t1, t2);
    __m128 t12m  = change_sign<1,0,0,0>(Vec4f(t12));
    __m128 a3312 = _mm_shuffle_ps(a,a,0x9F);
    __m128 b3231 = _mm_shuffle_ps(b,b,0x7B);
    __m128 a0000 = _mm_shuffle_ps(a,a,0x00);
    __m128 t3    = _mm_mul_ps(a3312, b3231);
    __m128 t0    = _mm_mul_ps(a0000, b);
    __m128 t03   = _mm_sub_ps(t0, t3);
    return         _mm_add_ps(t03, t12m);
}

// operator *= : multiply
static inline Quaternion4f & operator *= (Quaternion4f & a, Quaternion4f const & b) {
    a = a * b;
    return a;
}

// operator ~ : complex conjugate
// ~(a + b*i + c*j + d*k) = (a - b*i - c*j - d*k)
static inline Quaternion4f operator ~ (Quaternion4f const & a) {
    return Quaternion4f(change_sign<0,1,1,1>(a.to_vector()));
}

// function reciprocal: multiplicative inverse
static inline Quaternion4f reciprocal (Quaternion4f const & a) {
    Vec4f sq  = _mm_mul_ps(a,a);
    float nsq = horizontal_add(sq);
    return Quaternion4f((~a).to_vector() / Vec4f(nsq));
}

// operator / : quaternion divide is defined as
// a / b = a * reciprocal(b)
static inline Quaternion4f operator / (Quaternion4f const & a, Quaternion4f const & b) {
    return a * reciprocal(b);
}

// operator /= : divide
static inline Quaternion4f & operator /= (Quaternion4f & a, Quaternion4f const & b) {
    a = a / b;
    return a;
}

// operator == : returns true if a == b
static inline bool operator == (Quaternion4f const & a, Quaternion4f const & b) {
    Vec4fb t1 = a.to_vector() == b.to_vector();
    return horizontal_and(t1);
}

// operator != : returns true if a != b
static inline bool operator != (Quaternion4f const & a, Quaternion4f const & b) {
    Vec4fb t1 = a.to_vector() != b.to_vector();
    return horizontal_or(t1);
}


/*****************************************************************************
*
*          Operators mixing Quaternion4f and float
*
*****************************************************************************/

// operator + : add
static inline Quaternion4f operator + (Quaternion4f const & a, float b) {
    return _mm_add_ss(a, _mm_set_ss(b));
}

static inline Quaternion4f operator + (float a, Quaternion4f const & b) {
    return b + a;
}

static inline Quaternion4f & operator += (Quaternion4f & a, float & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Quaternion4f operator - (Quaternion4f const & a, float b) {
    return _mm_sub_ss(a, _mm_set_ss(b));
}

static inline Quaternion4f operator - (float a, Quaternion4f const & b) {
    return _mm_sub_ps(_mm_set_ss(a), b);
}

static inline Quaternion4f & operator -= (Quaternion4f & a, float & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Quaternion4f operator * (Quaternion4f const & a, float b) {
    return _mm_mul_ps(a, _mm_set1_ps(b));
}

static inline Quaternion4f operator * (float a, Quaternion4f const & b) {
    return b * a;
}

static inline Quaternion4f & operator *= (Quaternion4f & a, float & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Quaternion4f operator / (Quaternion4f const & a, float b) {
    return _mm_div_ps(a, _mm_set1_ps(b));
}

static inline Quaternion4f operator / (float a, Quaternion4f const & b) {
    return reciprocal(b) * a;
}

static inline Quaternion4f & operator /= (Quaternion4f & a, float b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Quaternion4f
*
*****************************************************************************/

// function abs: calculate the norm
// abs(a + b*i + c*j + d*k) = sqrt(a*a + b*B + c*c + d*d)
static inline Quaternion4f abs(Quaternion4f const & a) {
    Vec4f sq  = _mm_mul_ps(a,a);
    float nsq = horizontal_add(sq);
    return sqrtf(nsq);
}

// function select
static inline Quaternion4f select (bool s, Quaternion4f const & a, Quaternion4f const & b) {
    return Quaternion4f(s ? a : b);
}


/*****************************************************************************
*
*                     Class Quaternion4d
*         One quaternion consisting of four double precision floats
*
*****************************************************************************/

class Quaternion4d {
protected:
    Vec4d y; // vector of 4 doubles
public:
    // default constructor
    Quaternion4d() {
    }
    // construct from real and imaginary parts = re + im0*i + im1*j + im2*k
    Quaternion4d(double re, double im0, double im1, double im2) {
        y = Vec4d(re, im0, im1, im2);
    }
    // construct from real, no imaginary part
    Quaternion4d(double re) {
        y = Vec4d(re, 0., 0., 0.);
    }
    // construct from two Complex2d = a0 + a1 * j
    Quaternion4d(Complex2d const & a0, Complex2d const & a1) {
        y = Vec4d(Vec2d(a0), Vec2d(a1));
    }
    // Constructor to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Quaternion4d(__m256d const & x) {
#else
    Quaternion4d(Vec256de const & x) {
#endif
        y = x;
    }
    // Assignment operator to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Quaternion4d & operator = (__m256d const & x) {
#else
    Quaternion4d & operator = (Vec256de const & x) {
#endif
        y = x;
        return *this;
    }
    // Constructor to convert from Vec4d
    Quaternion4d(Vec4d const & x) {
        y = x;
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
    Quaternion4d & load(double const * p) {
        y.load(p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Quaternion4d const & load_a(double const * p) {
        y.load_a(p);
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
    // q = q.get_low() + q.get_high()*j
    Complex2d get_low() const {
        return Complex2d(y.get_low());
    }
    Complex2d get_high() const {
        return Complex2d(y.get_high());
    }
    // Member function to extract real part
    double real() const {
        return y.extract(0);
    }
    // Member function to extract imaginary parts, sets real part to 0
    Quaternion4d imag() const {
        return Quaternion4d(permute4d<-1,1,2,3>(Vec4d(y)));
    }
    // Member function to extract one real or imaginary part
    double extract(uint32_t index) const {
        return y.extract(index);
    }
#ifdef VECTOR3D_H
    // Constructor to convert from Vec3d used in geometrics:
    Quaternion4d(Vec3d const & x) {
        y = permute4d<3,0,1,2>(Vec4d(x));  // rotate elements
    }
    // Type cast operator to convert to Vec3d used in geometrics:
    operator Vec3d() const {
        return Vec3d(permute4d<1,2,3,0>(y));  // rotate elements
    }
#endif // VECTOR3D_H 
};


/*****************************************************************************
*
*          Operators for Quaternion4d
*
*****************************************************************************/

// operator + : add
static inline Quaternion4d operator + (Quaternion4d const & a, Quaternion4d const & b) {
    return Quaternion4d(a.to_vector() + b.to_vector());
}

// operator += : add
static inline Quaternion4d & operator += (Quaternion4d & a, Quaternion4d const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Quaternion4d operator - (Quaternion4d const & a, Quaternion4d const & b) {
    return Quaternion4d(a.to_vector() - b.to_vector());
}

// operator - : unary minus
static inline Quaternion4d operator - (Quaternion4d const & a) {
    return Quaternion4d(- a.to_vector());
}

// operator -= : subtract
static inline Quaternion4d & operator -= (Quaternion4d & a, Quaternion4d const & b) {
    a = a - b;
    return a;
}

// operator * : quaternion multiply
static inline Quaternion4d operator * (Quaternion4d const & a, Quaternion4d const & b) {
    Vec4d a1123 = permute4d<1,1,2,3>(a.to_vector());
    Vec4d a2231 = permute4d<2,2,3,1>(a.to_vector());
    Vec4d b1000 = permute4d<1,0,0,0>(b.to_vector());
    Vec4d b2312 = permute4d<2,3,1,2>(b.to_vector());
    Vec4d t1    = a1123 * b1000;
    Vec4d t2    = a2231 * b2312;
    Vec4d t12   = t1 + t2;
    Vec4d t12m  = change_sign<1,0,0,0>(t12);
    Vec4d a3312 = permute4d<3,3,1,2>(a.to_vector());
    Vec4d b3231 = permute4d<3,2,3,1>(b.to_vector());
    Vec4d a0000 = permute4d<0,0,0,0>(a.to_vector());
    Vec4d t3    = a3312 * b3231;
    Vec4d t0    = a0000 * b.to_vector();
    Vec4d t03   = t0  - t3;
    return        t03 + t12m;
}

// operator *= : multiply
static inline Quaternion4d & operator *= (Quaternion4d & a, Quaternion4d const & b) {
    a = a * b;
    return a;
}

// operator ~ : complex conjugate
// ~(a + b*i + c*j + d*k) = (a - b*i - c*j - d*k)
static inline Quaternion4d operator ~ (Quaternion4d const & a) {
    return Quaternion4d(change_sign<0,1,1,1>(a.to_vector()));
}

// function reciprocal: multiplicative inverse
static inline Quaternion4d reciprocal (Quaternion4d const & a) {
    Vec4d sq  = a.to_vector() * a.to_vector();
    double nsq = horizontal_add(sq);
    return Quaternion4d((~a).to_vector() / Vec4d(nsq));
}

// operator / : quaternion divide is defined as
// a / b = a * reciprocal(b)
static inline Quaternion4d operator / (Quaternion4d const & a, Quaternion4d const & b) {
    return a * reciprocal(b);
}

// operator /= : divide
static inline Quaternion4d & operator /= (Quaternion4d & a, Quaternion4d const & b) {
    a = a / b;
    return a;
}

// operator == : returns true if a == b
static inline bool operator == (Quaternion4d const & a, Quaternion4d const & b) {
    Vec4db t1 = a.to_vector() == b.to_vector();
    return horizontal_and(t1);
}

// operator != : returns true if a != b
static inline bool operator != (Quaternion4d const & a, Quaternion4d const & b) {
    Vec4db t1 = a.to_vector() != b.to_vector();
    return horizontal_or(t1);
}


/*****************************************************************************
*
*          Operators mixing Quaternion4d and double
*
*****************************************************************************/

// operator + : add
static inline Quaternion4d operator + (Quaternion4d const & a, double b) {
    return a + Quaternion4d(b);
}

static inline Quaternion4d operator + (double a, Quaternion4d const & b) {
    return b + a;
}

static inline Quaternion4d & operator += (Quaternion4d & a, double & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Quaternion4d operator - (Quaternion4d const & a, double b) {
    return a - Quaternion4d(b);
}

static inline Quaternion4d operator - (double a, Quaternion4d const & b) {
    return Quaternion4d(a) - b;
}

static inline Quaternion4d & operator -= (Quaternion4d & a, double & b) {
    a = a - b;
    return a;
}

// operator * : multiply
static inline Quaternion4d operator * (Quaternion4d const & a, double b) {
    return Quaternion4d(a.to_vector() * b);
}

static inline Quaternion4d operator * (double a, Quaternion4d const & b) {
    return b * a;
}

static inline Quaternion4d & operator *= (Quaternion4d & a, double & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Quaternion4d operator / (Quaternion4d const & a, double b) {
    return Quaternion4d(a.to_vector() / Vec4d(b));
}

static inline Quaternion4d operator / (double a, Quaternion4d const & b) {
    return reciprocal(b) * a;
}

static inline Quaternion4d & operator /= (Quaternion4d & a, double b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Quaternion4d
*
*****************************************************************************/

// function abs: calculate the norm
// abs(a + b*i + c*j + d*k) = sqrt(a*a + b*B + c*c + d*d)
static inline Quaternion4d abs(Quaternion4d const & a) {
    Vec4d sq  = a.to_vector() * a.to_vector();
    double nsq = horizontal_add(sq);
    return sqrt(nsq);
}

// function select
static inline Quaternion4d select (bool s, Quaternion4d const & a, Quaternion4d const & b) {
    return Quaternion4d(s ? a : b);
}

// conversion of precision
// function to_single: convert Quaternion4d to Quaternion4f
static inline Quaternion4f to_single (Quaternion4d const & a) {
    return Quaternion4f(Vec4f(to_single(Complex4d(a.to_vector()))));
}

// function to_double: convert Quaternion4f to Quaternion4d
static inline Quaternion4d to_double (Quaternion4f const & a) {
    return Quaternion4d(Vec4d(to_double(Complex4f(a.to_vector()))));
}

#endif  // QUATERNION_H
