/*
This file is part of Jenn.
Copyright 2001-2007 Fritz Obermeyer.

Jenn is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Jenn is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Jenn; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef JENN_LINALG_H
#define JENN_LINALG_H

#include <cmath>
#include <complex>
#include <vector>
#include <cstdlib>

typedef std::complex<float> complex;

#include "definitions.h"

#ifndef M_PI
    #define M_PI 3.1415926535897
#endif

namespace std {
std::ostream& operator<< (std::ostream& os, const std::vector<int>& v);
} // namespace std

namespace LinAlg
{
const Logging::Logger logger("linalg", Logging::INFO);
}

//[ linear algebra shite ]----------------------------------

/* conventions:
  [x, y, z, w] - first two are screen space,
            - thrid is out-of-screen vector
            - fourth is the center (hidden) dimension
*/

struct Vect
{
    float data[4];
    float& operator[] (int i)       { return data[i]; }
    float  operator[] (int i) const { return data[i]; }

    //void operator= (float x) { data[3] = data[2] = data[1] = data[0] = x; }
    void operator+= (const Vect& v)
        { for (int i=0; i<4; ++i) data[i] += v[i]; }
    void operator*= (float c)
        { for (int i=0; i<4; ++i) data[i] *= c; }
};
inline Vect const_vect (float x)
{
    Vect result;
    for (int i=0; i<4; ++i) result[i] = x;
    return result;
}
struct Mat
{
    Vect data[4];
    Vect& operator[] (int i)       { return data[i]; }
    Vect  operator[] (int i) const { return data[i]; }
};

inline float g_sqrt (float num) { return num>0 ? sqrt(num) : 0; }

inline float sqr (float num) { return num*num; }

inline float inner (const Vect &a, const Vect &b) //inner product
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}
inline float inner3 (const Vect &a, const Vect &b) //inner product
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
inline void cross3 (const Vect &a, const Vect &b, Vect &c)
{
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
}
void cross4 (const Vect &a, const Vect &b, const Vect &c, Vect &d);

inline float norm (const Vect &a)
{
    return sqrt(sqr(a[0]) + sqr(a[1]) + sqr(a[2]) + sqr(a[3]));
}

inline float r3_norm(const Vect &a)
{
    return sqrt(sqr(a[0]) + sqr(a[1]) + sqr(a[2]));
}

inline float r3_dist_sqr(const Vect &a, const Vect &b)
{
    return sqr(a[0]-b[0]) + sqr(a[1]-b[1]) + sqr(a[2]-b[2]);
}

inline float r4_dist (const Vect& a, const Vect& b)
{
    return sqrt( sqr(a[0]-b[0])
               + sqr(a[1]-b[1])
               + sqr(a[2]-b[2])
               + sqr(a[3]-b[3]) );
}

inline void normalize (Vect &a)
{
    const float norm = sqrt(sqr(a[0]) + sqr(a[1]) + sqr(a[2]) + sqr(a[3]));
    a[0] /= norm;
    a[1] /= norm;
    a[2] /= norm;
    a[3] /= norm;
}

inline bool normalize3(Vect &a)
{//returns true if normalizable
    const float norm = sqrtf(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    if (norm <= 0) return false;
    a[0] /= norm;
    a[1] /= norm;
    a[2] /= norm;
    return true;
}
inline void vect_iadd (Vect &a, const Vect &b)
{//a += b
    for (int i=0; i<4; ++i) a[i] += b[i];
}
inline void vect_scale (Vect&a, float s, const Vect& b)
{//a = s * b
    for (int i=0; i<4; ++i) { a[i] = b[i] * s; }
}
inline void vect_isadd (Vect&a, float s, const Vect& b)
{//a *= s * b
    for (int i=0; i<4; ++i) { a[i] += b[i] * s; }
}

void make_ortho (Mat &M);
void make_asym (Mat &M);
void mat_iscale (Mat &a, float t);
void mat_iadd (Mat &a, const Mat &b);
void mat_isub (Mat &a, const Mat &b);
void mat3_iadd (Mat &a, const Mat &b);
void mat_mult (const Mat &a, const Mat &b, Mat &c);
void mat_trans (const Mat &a, Mat &b);
void mat_conj (const Mat &a, const Mat &b, Mat &c);
void mat_copy (const Mat &a, Mat &b);
void vect_mult (const Mat &a, const Vect &b, Vect &c);
void vect_imul (const Mat &a, Vect &b);
void mat_zero (Mat &a);
void mat_identity (Mat &a);
void mat_inverse (const Mat &a, Mat &b);
void mat_rot (int i, int j, float theta, Mat &a);
void print_matrix (const Mat &a);


float rand_gauss ();
void rand_asym_mat (Mat &a, float sigma=1.0f);
inline float random_unif ()
{
#ifndef CYGWIN_HACKS
    return drand48();
#else
    return static_cast<float>(rand()) / RAND_MAX;
#endif
}

//[ geometry stuff ]----------

void build_geom (const int *coxeter_utriang,
                 const std::vector<std::vector<int> >& vertex_coset,
                 const std::vector<std::vector<int> >& gens,
                 const std::vector<std::vector<int> >& v_cogens,
                 const Vect& weights,
                 std::vector<Mat>& gen_reps,
                 Vect &origin);
complex hopf_phase (Vect &e);

#endif

