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

#ifndef JENN_POLYTOPES_H
#define JENN_POLYTOPES_H

#include "definitions.h"
#include "linalg.h"
#include <vector>

namespace Polytope
{

const Logging::Logger logger("poly", Logging::INFO);

//conversions
typedef std::vector<int> Word;
typedef std::vector<Word> WordList;
Word int2word (int g);
Word str2word (const char* g);
Vect int2Vect (int w);

void view (const int* coxeter,  //upper triang of cox. matrix
   const WordList& gens,        //subgroup generators
   const WordList& v_cogens,    //vertex-stabilizing words
   const WordList& e_gens,      //edge words: origin to ___
   const WordList& f_gens,      //generator pairs, or empty word for all faces
   const Vect& weights);        //vertex weights, for positioning center
void select (int code, int edges=1111, int faces=111111, int weights=1111);


//named polytopes
const int the_5_cell    = 322323234;
const int the_8_cell    = 422323234;
const int the_16_cell   = 322324234;
const int the_24_cell   = 322423234;
const int the_120_cell  = 522323234;
const int the_600_cell  = 322325234;

const int graph_torus   = 922229000;

const int graph_333     = 322323000;
const int graph_Y       = 333222000;
const int graph_334     = 422323000;
const int graph_343     = 322423000;
const int graph_335     = 522323000;

}

#endif

