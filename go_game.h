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

#ifndef JENN_GO_GAME_H
#define JENN_GO_GAME_H

#include <cmath>
#include <vector>
#include <map>
#include <utility>

#include "definitions.h"
#include "todd_coxeter.h"

namespace GoGame
{

const Logging::Logger logger("go", Logging::INFO);

//[ simple go playing stuff ]----------

inline int other  (int s) { return (s == 2)?1:2; }

//states
typedef char Color;
const Color EMPTY = 0, BLACK = 1, WHITE = 2;
class Diff
{
    typedef std::map<int,std::pair<Color,Color> > DiffType;
    DiffType m_diff;
public:
    void add (int pos, Color old_c, Color new_c)
    { m_diff[pos] = std::make_pair(old_c,new_c); }

    //iteration
    typedef DiffType::const_iterator iterator;
    iterator begin () const { return m_diff.begin(); }
    iterator end   () const { return m_diff.end(); }
};
class State
{
    std::vector<Color> m_state;
public: 
    State (int ord=0, Color start=0) : m_state(ord, start) {}
    void fwd (const Diff& diff);
    void bwd (const Diff& diff);
    Color  operator[] (int pos) const { return m_state[pos]; }
    Color& operator[] (int pos)       { return m_state[pos]; }
};

//a go board, with surrounding actions disabled
class GO
{
public:
    std::vector<int> group;    
    ToddCoxeter::Graph *graph;
    State current_state;
    unsigned time;
    std::vector<Diff> history;
    std::vector<bool> highlighted;
    
    GO (ToddCoxeter::Graph *g);
    ~GO () { delete graph; }

    //status
    Color& state (int v) { return current_state[v]; }

    //playing
    void play (int v, Color s); //position, button number
    void highlight (int v);   //position
    void highlight_none ();

    //history traversal
    void back ();
    void forward ();
private:
    void _push_diff ();
    Diff& diff () { return history[time-1]; }
};

}

#endif

