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

#include "polytopes.h"
#include "drawing.h"
#include "projection.h"
#include <cstring>

namespace Polytope
{

Word int2word (int g)
{
    Word result;
    while (g) {
        result.push_back((g%10) - 1);
        g /= 10;
    }
    return result;
}
Word str2word (const char* g)
{
    Word result;
    char num[2] = {0,0};
    int I=strlen(g);
    for (int i=0; i<I; ++i) {
        num[0] = g[i];
        result.push_back(atoi(num));
    }
    return result;
}
Vect int2Vect (int w)
{
    Vect result;
    for (int i=0; i<4; ++i) {
        result[3-i] = w%10;
        w /= 10;
    }
    return result;
}

void view (int c12, int c13, int c14, int c23, int c24, int c34,
           int g1, int g2, int g3,
           int edges, int faces, int weights);
void select (int code, int edges, int faces, int weights)
{//selects based on digit pattern CCCCCCGGG (WARNING: pack g's with zeros)
    if (not code) return;

    int g3 = code % 10; code /= 10;
    int g2 = code % 10; code /= 10;
    int g1 = code % 10; code /= 10;

    int c34 = code % 10; code /= 10;
    int c24 = code % 10; code /= 10;
    int c23 = code % 10; code /= 10;
    int c14 = code % 10; code /= 10;
    int c13 = code % 10; code /= 10;
    int c12 = code % 10; code /= 10;

    view(c12,c13,c14,c23,c24,c34,
         g1,g2,g3,
         edges, faces, weights);
}
void view (int c12, int c13, int c14, int c23, int c24, int c34,
           int g1, int g2, int g3,
           int edges, int faces, int weights)
{
    /*
    logger.debug() << "viewing <"
        << c12 << ", "
        << c13 << ", "
        << c14 << ", "
        << c23 << ", "
        << c24 << ", "
        << c34 << "> modulo {"
        << g1 << ", "
        << g2 << ", "
        << g3 << "}" |0;
    logger.debug() << "edges = " << edges << ", faces = " << faces |0;
    */

    int arg_coxeter[6];
    arg_coxeter[0] = c12;
    arg_coxeter[1] = c13;
    arg_coxeter[2] = c14;
    arg_coxeter[3] = c23;
    arg_coxeter[4] = c24;
    arg_coxeter[5] = c34;

    int e[5] = {0,0,0,0,0}; //so zero index does nothing
    Assert (0<=g1 and g1 <5, "generator #1 out of range");  e[g1] = true;
    Assert (0<=g2 and g2 <5, "generator #2 out of range");  e[g2] = true;
    Assert (0<=g3 and g3 <5, "generator #3 out of range");  e[g3] = true;

    //define symmetry subgroup
    std::vector<Word> gens;
    for (int i=0; i<4; ++i) {
        //LATER: this does nothing yet
        gens.push_back(Word(1,i));
    }

    //define vertex stabiliizer subgroup generators
    std::vector<Word> v_cogens;
    for (int i=0; i<4; ++i) {
        if (e[i+1]) v_cogens.push_back(int2word(i+1));
    }

    //define edges generators
    std::vector<Word> e_gens;
    for (int i=0; i<4; ++i) {
        if ((not e[i+1]) and (edges % 10)) {
            e_gens.push_back(int2word(i+1));
        }
        edges /= 10;
    }

    //define face generators
    std::vector<Word> f_gens;
    for (int i=0; i<4; ++i) {
        for (int j=i+1; j<4; ++j) {
            if (faces % 10) {
                Word word;
                word.push_back(i);
                word.push_back(j);
                f_gens.push_back(word);
            }
            faces /= 10;
        }
    }

    //define weights
    Vect weight_vect = int2Vect(weights);

    view(arg_coxeter, gens, v_cogens, e_gens, f_gens, weight_vect);
}

void view (const int* coxeter,
           const WordList& gens,
           const WordList& v_cogens,
           const WordList& e_gens,
           const WordList& f_gens,
           const Vect& weights)
{
    static bool polytope_exists = false;

    const Logging::fake_ostream& os = logger.debug() << "setting polytope:";
    os << "\n  gens = ";
    for (unsigned i=0; i<gens.size(); ++i) {
        os << gens[i] << ", ";
    }
    os << "\n  v_cogens = ";
    for (unsigned i=0; i<v_cogens.size(); ++i) {
        os << v_cogens[i] << ", ";
    }
    os << "\n  e_gens = ";
    for (unsigned i=0; i<e_gens.size(); ++i) {
        os << e_gens[i] << ", ";
    }
    os << "\n  f_gens = ";
    for (unsigned i=0; i<f_gens.size(); ++i) {
        os << f_gens[i] << ", ";
    }
    os |0;

    int params = 0;
    if (polytope_exists) {
        params = drawing->get_params();
        delete drawing;
    }

    drawing = new Drawings::Drawing(
              new ToddCoxeter::Graph(coxeter, gens,
                                     v_cogens, e_gens, f_gens, weights));

    if (polytope_exists) {
        drawing->set_params(params);
        projector->set_drawing();
    }

    polytope_exists = true;
}

}

