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

#include "go_game.h"
#include <algorithm>

namespace GoGame
{

//states
void State::fwd (const Diff& diff)
{
    for (Diff::iterator iter = diff.begin(); iter!=diff.end(); ++iter) {
        m_state[iter->first] = iter->second.second;
    }
}
void State::bwd (const Diff& diff)
{
    for (Diff::iterator iter = diff.begin(); iter!=diff.end(); ++iter) {
        m_state[iter->first] = iter->second.first;
    }
}

//group class
struct Group
{
    std::vector<int> members, liberties, neighbors;
    int state;
    Group (int v, GO* go);
    void remove (GO* go);
    void highlight (GO* go, int value=true);
    bool in_atari () { return liberties.size() == 1; }
};
inline bool safe_insert (int i, std::vector<int>& v)
{
    if (find(v.begin(), v.end(), i) == v.end()) {
        v.push_back(i);
        return true;
    }
    return false;
}
Group::Group (int v, GO* go)
    : members(1,v),
      liberties(0),
      neighbors(0),
      state(go->state(v))
{
    for (unsigned i=0; i<members.size(); ++i) {
        int v1 = members[i];
        for (int j=0; j<go->graph->deg; ++j) {
            int v2 = go->graph->adj[v1][j];
            int s2 = go->state(v2);
            if (s2 == state) {
                safe_insert(v2, members);
                continue;
            }
            if (s2 == 0) {
                safe_insert(v2, liberties);
                continue;
            }
            if (s2 == other(state)) {
                safe_insert(v2, neighbors);
            }
        }
    }
    //if (state && liberties.empty()) remove(go);
}
inline int weight (int s) { return (s == 2)?1:-1; }
void Group::remove (GO* go)
{
    for (unsigned i=0; i<members.size(); ++i) {
        go->state(members[i]) = 0;
    }
}
void Group::highlight (GO* go, int value)
{
    for (unsigned i=0; i<members.size(); ++i) {
        go->highlighted[members[i]] = value;
    }
}

//go class
GO::GO (ToddCoxeter::Graph* g)
    : graph(g),
      current_state(graph->ord, EMPTY),
      time(0),
      history(0),
      highlighted(graph->ord, false)
{}
void GO::back ()
{
    if (time == 0) return;

    logger.debug() << "skipping back one frame" |0;
    current_state.bwd(diff());
    --time;

    highlight_none();
}
void GO::forward ()
{
    if (time == history.size()) return;

    logger.debug() << "moving forward one frame" |0;
    ++time;
    current_state.fwd(diff());

    highlight_none();
}
void GO::_push_diff ()
{
    history.resize(time++);
    history.push_back(Diff());
}
void GO::play (int v, Color s)
{
    Assert (s==EMPTY or s==BLACK or s==WHITE, "unknown go state: " << s);
    int old = state(v);
    if (s == old) { highlight(v); return; }
    if (old != EMPTY) s = EMPTY;

    _push_diff();
    if (highlighted[v]) {
        //play whole group
        for (int w=0; w<graph->ord; ++w) {
            if (highlighted[w]) diff().add(w, state(w), s);
        }
    } else {
        //play one stone only
        diff().add(v, state(v), s);
    }
    current_state.fwd(diff());
    highlight_none();
}
void GO::highlight (int v)
{
    Group(v,this).highlight(this, not highlighted[v]);
}
void GO::highlight_none ()
{
    for (int i=0; i<graph->ord; ++i) {
        highlighted[i] = false;
    }
}

}

