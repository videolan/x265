/****************************  vector3d.h   ***********************************
| Author:        Agner Fog
| Date created:  2012-08-01
| Last modified: 2012-08-01
| Version:       1.02 Beta
| Project:       vector classes
| Description:
| Classes for 3-dimensional vectors
| Vec3f:      A vector of 3 single precision floats
| Vec3d:      A vector of 3 double precision floats
|
| (c) Copyright 2012 GNU General Public License http://www.gnu.org/licenses
\*****************************************************************************/

#ifndef VECTOR3D_H
#define VECTOR3D_H  102

#include "vectorclass.h"
#include <math.h>          // define math library functions


/*****************************************************************************
*
*               Class Vec3f: vector of 3 single precision floats
*
*****************************************************************************/

class Vec3f : public Vec4f {
public:
    // default constructor
    Vec3f() {
    }
    // construct from three coordinates
    Vec3f(float x, float y, float z) {
        xmm = Vec4f(x, y, z, 0.f);
    }
    // Constructor to convert from Vec4f
    Vec3f(Vec4f const & x) {
        xmm = x;
        // cutoff(3);
    }
    // Constructor to convert from type __m128 used in intrinsics:
    Vec3f(__m128 const & x) {
        xmm = x;
    }
    // Assignment operator to convert from type __m128 used in intrinsics:
    Vec3f & operator = (__m128 const & x) {
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
    Vec3f & load(float const * p) {
        xmm = Vec4f().load_partial(3, p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Vec3f const & load_a(float const * p) {
        xmm = Vec4f().load_a(p).cutoff(3);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(float * p) const {
        Vec4f(xmm).store_partial(3, p);
    }
    // Member function to store into array, aligned by 16
    void store_a(float * p) const {
        store(p);
    }
    // get x part
    float get_x() const {
        return _mm_cvtss_f32(xmm);
    }
    // get y part
    float get_y() const {
        Vec4f t = permute4f<1,-256,-256,-256>(Vec4f(xmm));
        return Vec3f(t).get_x();
    }
    // get z part
    float get_z() const {
        Vec4f t = permute4f<2,-256,-256,-256>(Vec4f(xmm));
        return Vec3f(t).get_x();
    }
    // Member function to extract one coordinate
    float extract(uint32_t index) const {
        float x[4];
        Vec4f(xmm).store(x);
        if (index < 3) return x[index];
        return 0.f;
    }
    // Operator [] to extract one coordinate
    // Operator [] can only read an element, not write.
    float operator [] (uint32_t index) const {
        return extract(index);
    }
};

/*****************************************************************************
*
*          Operators for Vec3f
*
*****************************************************************************/

// operator + : add
static inline Vec3f operator + (Vec3f const & a, Vec3f const & b) {
    return Vec3f(Vec4f(a) + Vec4f(b));
}

// operator += : add
static inline Vec3f & operator += (Vec3f & a, Vec3f const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Vec3f operator - (Vec3f const & a, Vec3f const & b) {
    return Vec3f(Vec4f(a) - Vec4f(b));
}

// operator - : unary minus
static inline Vec3f operator - (Vec3f const & a) {
    return Vec3f(- Vec4f(a));
}

// operator -= : subtract
static inline Vec3f & operator -= (Vec3f & a, Vec3f const & b) {
    a = a - b;
    return a;
}

// operator * : multiply element-by-element
// (see also cross_product and dot_product)
static inline Vec3f operator * (Vec3f const & a, Vec3f const & b) {
    return Vec3f(Vec4f(a) * Vec4f(b));
}

// operator *= : multiply element-by-element
static inline Vec3f & operator *= (Vec3f & a, Vec3f const & b) {
    a = a * b;
    return a;
}

// operator / : divide element-by-element
static inline Vec3f operator / (Vec3f const & a, Vec3f const & b) {
    return Vec3f(Vec4f(a) / Vec4f(b));
}

// operator /= : divide element-by-element
static inline Vec3f & operator /= (Vec3f & a, Vec3f const & b) {
    a = a / b;
    return a;
}

// operator == : returns true if a == b
static inline bool operator == (Vec3f const & a, Vec3f const & b) {
    Vec4fb t1 = Vec4f(a) == Vec4f(b);
    Vec4fb t2 = _mm_shuffle_ps(t1, t1, 0x24);  // ignore unused top element
    return horizontal_and(t2);
}

// operator != : returns true if a != b
static inline bool operator != (Vec3f const & a, Vec3f const & b) {
    Vec4fb t1 = Vec4f(a) != Vec4f(b);
    Vec4fb t2 = _mm_shuffle_ps(t1, t1, 0x24);  // ignore unused top element
    return horizontal_or(t2);
}

/*****************************************************************************
*
*          Operators mixing Vec3f and float
*
*****************************************************************************/

// operator * : multiply
static inline Vec3f operator * (Vec3f const & a, float b) {
    return _mm_mul_ps(a, _mm_set1_ps(b));
}
static inline Vec3f operator * (float a, Vec3f const & b) {
    return b * a;
}
static inline Vec3f & operator *= (Vec3f & a, float & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Vec3f operator / (Vec3f const & a, float b) {
    return _mm_div_ps(a, _mm_set1_ps(b));
}

static inline Vec3f & operator /= (Vec3f & a, float b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Vec3f
*
*****************************************************************************/

// function cross_product
static inline Vec3f cross_product (Vec3f const & a, Vec3f const & b) {
    Vec4f a1 = permute4f<1,2,0,-256>(Vec4f(a));
    Vec4f b1 = permute4f<1,2,0,-256>(Vec4f(b));
    Vec4f a2 = permute4f<2,0,1,-256>(Vec4f(a));
    Vec4f b2 = permute4f<2,0,1,-256>(Vec4f(b));
    Vec4f c  = a1 * b2 - a2 * b1;
    return c.cutoff(3);
}

// function dot_product
static inline float dot_product (Vec3f const & a, Vec3f const & b) {
    Vec4f c  = (Vec4f(a) * Vec4f(b)).cutoff(3);
    return horizontal_add(c);
}

// function vector_length
static inline float vector_length (Vec3f const & a) {
    return sqrtf(dot_product(a,a));
}

// function normalize_vector
static inline Vec3f normalize_vector (Vec3f const & a) {
    return a / vector_length(a);
}

// function select
static inline Vec3f select (bool s, Vec3f const & a, Vec3f const & b) {
    return s ? a : b;
}

// function rotate
// The vector a is rotated by multiplying by the matrix defined by the three columns col0, col1, col2
static inline Vec3f rotate (Vec3f const & col0, Vec3f const & col1, Vec3f const & col2, Vec3f const & a) {
    Vec3f xbroad = permute4f<0,0,0,-256>(Vec4f(a));  // broadcast x
    Vec3f ybroad = permute4f<1,1,1,-256>(Vec4f(a));  // broadcast y
    Vec3f zbroad = permute4f<2,2,2,-256>(Vec4f(a));  // broadcast z
    Vec3f r = col0 * xbroad + col1 * ybroad + col2 * zbroad;
    return r.cutoff(3);
}


/*****************************************************************************
*
*               Class Vec3d: vector of 3 double precision floats
*
*****************************************************************************/

class Vec3d : public Vec4d {
public:
    // default constructor
    Vec3d() {
    }
    // construct from three coordinates
    Vec3d(double x, double y, double z) 
        : Vec4d(x, y, z, 0.) {}
    // Constructor to convert from Vec4d
    Vec3d(Vec4d const & x) 
        : Vec4d(x) {
        // cutoff(3);
    }
    // Constructor to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Vec3d(__m256d const & x) : Vec4d(x) {}
#else
    Vec3d(Vec256de const & x) : Vec4d(x) {}
#endif
    // Assignment operator to convert from type __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    Vec3d & operator = (__m256d const & x) {
#else
    Vec3d & operator = (Vec256de const & x) {
#endif
        *this = Vec3d(x);
        return *this;
    }
    // Type cast operator to convert to __m256d used in intrinsics or Vec256de used in emulation
#if INSTRSET >= 7  // AVX
    operator __m256d() const {
#else
    operator Vec256de() const {
#endif
        return Vec4d(*this);
    }
    // Member function to load from array (unaligned)
    Vec3d & load(double const * p) {
        Vec4d::load_partial(3, p);
        return *this;
    }
    // Member function to load from array, aligned by 16
    Vec3d const & load_a(double const * p) {
        Vec4d::load_a(p);
        cutoff(3);
        return *this;
    }
    // Member function to store into array (unaligned)
    void store(double * p) const {
        Vec4d::store_partial(3, p);
    }
    // Member function to store into array, aligned by 16
    void store_a(double * p) const {
        store(p);
    }
    // get x part
    double get_x() const {
        return _mm_cvtsd_f64(get_low());
    }
    // get y part
    double get_y() const {
        Vec4d t = permute4d<1,-256,-256,-256>(Vec4d(*this));
        return Vec3d(t).get_x();
    }
    // get z part
    double get_z() const {
        Vec4d t = permute4d<2,-256,-256,-256>(Vec4d(*this));
        return Vec3d(t).get_x();
    }
    // Member function to extract one coordinate
    double extract(uint32_t index) const {
        double x[4];
        Vec4d::store(x);
        if (index < 3) return x[index];
        return 0.0;
    }
    // Operator [] to extract one coordinate
    // Operator [] can only read an element, not write.
    double operator [] (uint32_t index) const {
        return extract(index);
    }
};

/*****************************************************************************
*
*          Operators for Vec3d
*
*****************************************************************************/

// operator + : add
static inline Vec3d operator + (Vec3d const & a, Vec3d const & b) {
    return Vec3d(Vec4d(a) + Vec4d(b));
}

// operator += : add
static inline Vec3d & operator += (Vec3d & a, Vec3d const & b) {
    a = a + b;
    return a;
}

// operator - : subtract
static inline Vec3d operator - (Vec3d const & a, Vec3d const & b) {
    return Vec3d(Vec4d(a) - Vec4d(b));
}

// operator - : unary minus
static inline Vec3d operator - (Vec3d const & a) {
    return Vec3d(- Vec4d(a));
}

// operator -= : subtract
static inline Vec3d & operator -= (Vec3d & a, Vec3d const & b) {
    a = a - b;
    return a;
}

// operator * : multiply element-by-element
// (see also cross_product and dot_product)
static inline Vec3d operator * (Vec3d const & a, Vec3d const & b) {
    return Vec3d(Vec4d(a) * Vec4d(b));
}

// operator *= : multiply element-by-element
static inline Vec3d & operator *= (Vec3d & a, Vec3d const & b) {
    a = a * b;
    return a;
}

// operator / : divide element-by-element
static inline Vec3d operator / (Vec3d const & a, Vec3d const & b) {
    return Vec3d(Vec4d(a) / Vec4d(b));
}

// operator /= : divide element-by-element
static inline Vec3d & operator /= (Vec3d & a, Vec3d const & b) {
    a = a / b;
    return a;
}

// operator == : returns true if a == b
static inline bool operator == (Vec3d const & a, Vec3d const & b) {
    Vec4db t1 = Vec4d(a) == Vec4d(b);
#if INSTRSET >= 7  // AVX
    Vec4db t2 = Vec4db(permute4d<0,1,2,2>(Vec4d(t1)));  // ignore unused top element
    return horizontal_and(t2);
#else
    Vec2db u0 = t1.get_low();
    Vec2db u1 = t1.get_high();
    u1 = permute2d<0,0>(Vec2d(u1));             // ignore unused top element
    return horizontal_and(u0 & u1);
#endif
}

// operator != : returns true if a != b
static inline bool operator != (Vec3d const & a, Vec3d const & b) {
    Vec4db t1 = Vec4d(a) != Vec4d(b);
#if INSTRSET >= 7  // AVX
    Vec4db t2 = Vec4db(permute4d<0,1,2,2>(Vec4d(t1)));  // ignore unused top element
    return horizontal_and(t2);
#else
    Vec2db u0 = t1.get_low();
    Vec2db u1 = t1.get_high();
    u1 = permute2d<0,0>(Vec2d(u1));             // ignore unused top element
    return horizontal_or(u0 | u1);
#endif
}

/*****************************************************************************
*
*          Operators mixing Vec3d and double
*
*****************************************************************************/

// operator * : multiply
static inline Vec3d operator * (Vec3d const & a, double b) {
    return Vec4d(a) * Vec4d(b);
}
static inline Vec3d operator * (double a, Vec3d const & b) {
    return b * a;
}
static inline Vec3d & operator *= (Vec3d & a, double & b) {
    a = a * b;
    return a;
}

// operator / : divide
static inline Vec3d operator / (Vec3d const & a, double b) {
    return Vec4d(a) / Vec4d(b);
}

static inline Vec3d & operator /= (Vec3d & a, double b) {
    a = a / b;
    return a;
}


/*****************************************************************************
*
*          Functions for Vec3d
*
*****************************************************************************/

// function cross_product
static inline Vec3d cross_product (Vec3d const & a, Vec3d const & b) {
    Vec4d a1 = permute4d<1,2,0,-256>(Vec4d(a));
    Vec4d b1 = permute4d<1,2,0,-256>(Vec4d(b));
    Vec4d a2 = permute4d<2,0,1,-256>(Vec4d(a));
    Vec4d b2 = permute4d<2,0,1,-256>(Vec4d(b));
    Vec4d c  = a1 * b2 - a2 * b1;
    return c.cutoff(3);
}

// function dot_product
static inline double dot_product (Vec3d const & a, Vec3d const & b) {
    Vec4d c  = (Vec4d(a) * Vec4d(b)).cutoff(3);
    return horizontal_add(c);
}

// function vector_length
static inline double vector_length (Vec3d const & a) {
    return sqrt(dot_product(a,a));
}

// function normalize_vector
static inline Vec3d normalize_vector (Vec3d const & a) {
    return a / vector_length(a);
}

// function select
static inline Vec3d select (bool s, Vec3d const & a, Vec3d const & b) {
    return s ? a : b;
}

// function rotate
// The vector a is rotated by multiplying by the matrix defined by the three columns col0, col1, col2
static inline Vec3d rotate (Vec3d const & col0, Vec3d const & col1, Vec3d const & col2, Vec3d const & a) {
    Vec3d xbroad = permute4d<0,0,0,-256>(Vec4d(a));  // broadcast x
    Vec3d ybroad = permute4d<1,1,1,-256>(Vec4d(a));  // broadcast y
    Vec3d zbroad = permute4d<2,2,2,-256>(Vec4d(a));  // broadcast z
    Vec3d r = col0 * xbroad + col1 * ybroad + col2 * zbroad;
    return r.cutoff(3);
}


/*****************************************************************************
*
*          Conversion functions
*
*****************************************************************************/

// function to_single: convert Vec3d to Vec3f
static inline Vec3f to_single (Vec3d const & a) {
#if INSTRSET >= 7  // AVX
    //return _mm256_castps256_ps128(_mm256_cvtpd_ps(a));
    return _mm256_cvtpd_ps(a);
#else
    return Vec3f(Vec4f(compress(a.get_low(), a.get_high())));
#endif
}

// function to_double: convert Vec3f to Vec3d
static inline Vec3d to_double (Vec3f const & a) {
#if INSTRSET >= 7  // AVX
    return _mm256_cvtps_pd(a);
#else
    return Vec3d(Vec4d(extend_low(a), extend_high(a)));
#endif
}

#endif  // VECTOR3D_H
