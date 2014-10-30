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

#include "linalg.h"

namespace std {
std::ostream& operator<< (std::ostream& os, const std::vector<int>& v)
{
    for (unsigned i=0; i<v.size(); ++i) os << v[i];
    return os;
}
} // namespace std

using namespace LinAlg;

void cross4 (const Vect &a, const Vect &b, const Vect &c, Vect &d)
{
    float t = 1.0f;
    for (int i1=0; i1<4; ++i1) {
        int i2 = (i1+1) % 4;
        int i3 = (i2+1) % 4;
        int i4 = (i3+1) % 4;
        d[i1] = t * ( a[i2] * (b[i3]*c[i4] - b[i4]*c[i3])
                    + a[i3] * (b[i4]*c[i2] - b[i2]*c[i4])
                    + a[i4] * (b[i2]*c[i3] - b[i3]*c[i2]) );
        t = -t;
    }
}

void make_ortho (Mat &M)
{ //gram-schmidt orthonormalization
    float coef, norm;
    for (int i=0; i<4; ++i) {
        for (int j=0; j<i; ++j) {
            coef = inner(M[i],M[j]);
            for (int k=0;k<4; ++k) M[i][k] -= coef*M[j][k];
        }
        norm = 0;
        for (int j=0; j<4; ++j) norm += sqr(M[i][j]);
        norm = sqrt(norm);
        for (int j=0; j<4; ++j) M[i][j] /= norm;
    }
}
void make_asym (Mat &M)
{//projects to asymmetric part of matrix
    for (int i=0; i<4; ++i) {
        M[i][i] = 0;
        for (int j=i+1; j<4; ++j) {
            float& mij = M[i][j];
            float& mji = M[j][i];
            float diff = 0.5f * (mij - mji);
            mij = diff;
            mji = -diff;
        }
    }
}
void mat_iscale (Mat &a, float t)
{
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            a[i][j]  *= t;
}
void mat_iadd (Mat &a, const Mat &b)
{ //a +=b
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            a[i][j]  += b[i][j];
}
void mat_isub (Mat &a, const Mat &b)
{ //a -= b
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            a[i][j]  -= b[i][j];
}
void mat3_iadd (Mat &a, const Mat &b)
{ //a +=b
    for (int i=0; i<3; ++i)
        for (int j=0; j<3; ++j)
            a[i][j]  += b[i][j];
}
void mat_mult (const Mat &a, const Mat &b, Mat &c)
{ //a*b->c
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
        {
            c[i][j] = a[i][0]*b[0][j];
            for (int k=1;k<4; ++k)
                c[i][j] += a[i][k]*b[k][j];
        }
}
void mat_trans (const Mat &a, Mat &b)
{ //a'->b
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            b[j][i] = a[i][j];
}
void mat_conj (const Mat &a, const Mat &b, Mat &c)
{ //a*b*transpose(a)->c
    float temp[4][4];
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
        {
            temp[i][j] = a[0][i]*b[0][j];
            for (int k=1;k<4; ++k)
                temp[i][j] += a[k][i]*b[k][j];
        }          
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
        {
            c[i][j] = temp[i][0]*a[0][j];
            for (int k=1;k<4; ++k)
                c[i][j] += temp[i][k]*a[k][j];
        }
}
void mat_copy (const Mat &a, Mat &b)
{ //a->b
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            b[i][j] = a[i][j];
}
void vect_mult (const Mat &a, const Vect &b, Vect &c)
{ //a*b->c
    for (int i=0; i<4; ++i) {
        c[i] = a[i][0] * b[0];
        for (int j=1; j<4; ++j) {
            c[i] += a[i][j] * b[j];
        }
    }
}
void vect_imul (const Mat &a, Vect &b)
{ //a*b->b
    Vect c;
    for (int i=0; i<4; ++i) {
        c[i] = a[i][0] * b[0];
        for (int j=1; j<4; ++j) {
            c[i] += a[i][j] * b[j];
        }
    }
    b = c;
}
void mat_zero (Mat &a)
{ //a->identity
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
            a[i][j] = 0;
}
void mat_identity (Mat &a)
{ //a->identity
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            a[i][j] = (i==j) ? 1 : 0;
        }
    }
}
void mat_inverse (const Mat &a, Mat &b)
{ //inv(a)->b
    float m[4][8];
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j)
        {
            m[i][j] = a[i][j];
            m[i][j+4] = (i==j) ? 1 : 0;
        }
    for (int i=0; i<4; ++i) { //clears below diagnol
        //ensure m[i][i]! = 0;
        for (int j=i+1; j<4 && sqr(m[i][i])<0.2f; ++j) {
            for (int k=i;k<8; ++k) {
                m[i][k] += m[j][k];
            }
        }
        for (int j=i+1; j<8; ++j) {
            m[i][j] /= m[i][i];
        }
        m[i][i] = 1;
        for (int j=i+1; j<4; ++j) {
            for (int k=i+1;k<8; ++k) {
                m[j][k] -= m[j][i]*m[i][k];
            }
            m[j][i] = 0;
        }
    }
    for (int i = 3; i>0; --i) { //clears above diagnol
        for (int j=i-1; j>=0; --j) {
            for (int k=i+1;k<8; ++k) {
                m[j][k] -= m[i][k]*m[j][i];
            }
            m[j][i] = 0;
        }
    }
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            b[i][j] = m[i][j+4];
        }
    }
}
void mat_rot (int i, int j, float theta, Mat &a)
{ //sets rotator matrix of coord i to coord j by theta
    mat_identity(a);
    float c = cosf(theta);
    float s = sinf(theta);
    a[i][i] = c;
    a[j][j] = c;
    a[j][i] = s;
    a[i][j] = -s;
}
void print_matrix (const Mat &a)
{
    for (int i=0; i<4; ++i) {
        const Logging::fake_ostream& os = logger.info();
        os << "matrix:\n" << a[i][0];
        for (int j=1; j<4; ++j)
            os << "\t" << a[i][j];
    }
}
float rand_gauss ()
{//box-muller
    static bool available = false;
    static float y = 0.0f;

    if (available) {
        available = false;
        return y;
    } else{
        float theta = 2.0f*M_PI*random_unif();
        float r = sqrtf(-2.0f*logf(1.0f-random_unif()));
        float x = r * cosf(theta);
        y = r * sinf(theta);
        available = true;
        return x;
    }
}
void rand_asym_mat (Mat &a, float sigma)
{
    for (int i=0; i<4; ++i) {
        a[i][i] = 0.0f;
        for (int j=0; j<i; ++j) {
            float t = M_SQRT2 * sigma * rand_gauss();
            a[i][j] = t;
            a[j][i] = -t;
        }
    }
}

//[ geometry stuff ]----------

void build_geom (const int *coxeter_utriang,
                 const std::vector<std::vector<int> >& vertex_coset,
                 const std::vector<std::vector<int> >& gens,
                 const std::vector<std::vector<int> >& v_cogens,
                 const Vect& weights,
                 std::vector<Mat>& gen_reps,
                 Vect &origin)
{
    //construct coxeter matrix
    int coxeter[4][4];
    for (int i=0; i<4; ++i) {
        coxeter[i][i] = 1;
        for (int j=i+1; j<4; ++j) {
            coxeter[i][j] = coxeter[j][i] = *(coxeter_utriang++);
        }
    }

    //define cosines bewtween reflection planes
    Mat cosine;
    for (int i = 0; i<4; ++i) {
        for (int j = 0; j<4; ++j) {
            cosine[i][j] = cos(M_PI/coxeter[i][j]);
        }
    }

    //define normal vectors, i.e., reflection axes
    Mat axis;
    for (int i = 0; i<4; ++i) {
        for (int j = 0; j<4; ++j) {
            axis[i][j] = 0;
        }
    }
    axis[0][0] = 1;
    axis[1][0] = cosine[1][0];
    axis[1][1] = g_sqrt(1.0f - sqr(axis[1][0]));
    axis[2][0] = cosine[2][0];
    axis[2][1] = (cosine[2][1] - axis[2][0]*axis[1][0]) / axis[1][1];
    axis[2][2] = g_sqrt(1.0f-sqr(axis[2][0])-sqr(axis[2][1]));
    axis[3][0] = cosine[3][0];
    axis[3][1] = (cosine[3][1] - axis[3][0]*axis[1][0]) / axis[1][1];
    axis[3][2] = (cosine[3][2] - axis[3][0]*axis[2][0]
                 -axis[3][1]*axis[2][1]) / axis[2][2];
    axis[3][3] = g_sqrt(1.0f - sqr(axis[3][0]) - sqr(axis[3][1])
                 -sqr(axis[3][2]));

    //sanity check
    for (int i = 0; i<4; ++i) {
        for (int j = i; j<4; ++j) {
            Assert (fabs(inner(axis[i], axis[j]) - fabs(cosine[j][i])) < 1e-6,
                    "funkay business: axes don't jive with cosines");
        }
    }

    //define orthonormal basis
    Mat ortho;
    mat_inverse(axis, ortho);
    for (int i=0; i<4; ++i) {
        /* don't normalize, else origin will be off center
        //normalize
        float norm = 0;
        for (int j=0; j<4; ++j) norm += sqr(ortho[j][i]);
        float n = sqrt(norm);
        float s = 1 / n;
        logger.debug() << "scaling axis " << i << " by " << s |0;
        for (int j=0; j<4; ++j) {
            ortho[j][i] *= s;
            axis [i][j] *= n;
        }
        */

        //realign others to make angles acute
        for (int j = i+1; j<4; ++j) {
            float ip = 0;
            for (int k=0; k<4; ++k) {
                ip += ortho[k][i] * ortho[k][j];
            }
            if (ip >= -0.00001f) continue; //careful of duoprisms
            logger.debug() << "flipping axis " << j |0;
            for (int k=0; k<4; ++k) {
                ortho[k][j] *= -1;
                axis [j][k] *= -1;
            }
        }
    }
    Mat verts;
    mat_trans(ortho, verts);

    //define reflectors
    Mat reflectors[4];
    for (int letter = 0;letter<4;letter++) {
        //swap mirroror row to first
        for (int j = 0;j<4;j++) {
            ortho[0][j] = axis[letter][j];
            for (int i = 0;i<letter;i++) {
                ortho[i+1][j] = (i == j)?1:0;
            }
            for (int i = letter+1;i<4;i++) {
                ortho[i][j] = (i == j)?1:0;
            }
        }
        //make reflection matrix
        make_ortho(ortho);
        for (int i = 0; i<4; ++i) {
            for (int j = 0; j<4; ++j) {
                reflectors[letter][i][j] = -ortho[0][i] * ortho[0][j];
                for (int k = 1;k<4;k++)
                    reflectors[letter][i][j] += ortho[k][i] * ortho[k][j];
            }
        }
    }

    //define matrix representation
    gen_reps.resize(gens.size());
    for (unsigned j=0; j<gens.size(); ++j) {
        const std::vector<int> &word = gens[j];
        Mat& g = gen_reps[j];
        Mat temp;
        g = reflectors[word[0]];
        for (unsigned w=1; w<word.size(); ++w) {
            mat_mult(g, reflectors[word[w]], temp);
            g = temp;
        }
    }

    //define quotient vertices
    for (int j=0; j<4; ++j) {
        origin[j] = 0;
    }
    int included[4] = {0,0,0,0};
    bool simple_basis = true;
    for (unsigned n=0; n<v_cogens.size(); ++n) {
        const std::vector<int>& word = v_cogens[n];
        if (word.size() > 1) {
            simple_basis = false;
            break;
        }
        for (unsigned w=0; w<word.size(); ++w) {
            included[word[w]] = 1;
        }
    }
    if (simple_basis) {
        //just include gen_reps
        for (int i=0; i<4; ++i) {
            if (included[i]) continue;
            for (int j=0; j<4; ++j) {
                origin[j] += weights[i] * verts[i][j];
            }
        }
    } else {
        //average over coset
        for (unsigned w=0; w<vertex_coset.size(); ++w) {
            for (int i=0; i<4; ++i) {
                Vect term = verts[i];
                const std::vector<int>& word = vertex_coset[w];
                for (unsigned t=0; t<word.size(); ++t) { //XXX: backwards?
                    vect_imul(reflectors[word[t]], term);
                }
                vect_iadd(origin, term);
            }
        }
    }
    normalize(origin);
}

complex hopf_phase (Vect &e) //a 4-vector
{//phase from hopf fibration
    float x = e[0]*e[2] + e[1]*e[3];
    float y = e[1]*e[2] - e[0]*e[3];
    return complex(x, y);
}

