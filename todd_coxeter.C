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

#include "todd_coxeter.h"

#include <algorithm> //for sort
#include <set>
#include <map>
#include <fstream>

#define UNDEFINED -1
#define IDENTITY 4

/*
template<class os> os& operator<< (os& o, const std::vector<int>& v)
{
    for (unsigned i=0; i<v.size(); ++i) {
        o << v[i] << " ";
    }
    return o;
}
*/

namespace ToddCoxeter
{

//[ coxeter group class ]----------
class Group
{
public:
    int ord;
    int **_left;  //left  mult [element][generator]
    Word inv; //inverse table
    Word whence; //min-parse table
    Word parse (int v);
    Group (std::vector<Word> words);
    Group (Group& sup, std::vector<Word> words);
    ~Group (void);

    int left  (int v, int j) { return _left[v][j]; }
    int right (int v, int j) { return inv[_left[inv[v]][j]]; }
    int left  (int v, const Word& word);
    int right (int v, const Word& word);
};
Word Group::parse (int v)
{
    Word result;
    v = inv[v]; //to parse forwards
    for (int j=whence[v]; j!= IDENTITY; j = whence[v]) {
        result.push_back(j);
        v = left(v,j);
    }
    return result;
}
int Group::left (int v, const Word& word)
{
    int g = v;
    for (unsigned t=0; t<word.size(); ++t) {
        g = left(g, word[t]);
    }
    return g;
}
int Group::right (int v, const Word& word)
{
    int g = v;
    for (unsigned t=0; t<word.size(); ++t) {
        g = right(g, word[word.size() -t -1]);
    }
    return g;
}

//[ coxeter matrix parsing ]----------

struct _CmpVectorSize
{
    bool operator()(const Word& lhs, const Word& rhs) const
    { return lhs.size() < rhs.size(); }
};
const _CmpVectorSize _cmpVectorSize = _CmpVectorSize();

typedef std::vector<Word> Relations;
Relations words_from_cartan(const int *cartan)
{
    Relations words;
    int w=0;
    for (int i=0; i<3; ++i) {
        for (int j=i+1; j<4; ++j) {
            Word word;
            for (int n=0; n<cartan[w]; ++n) {
                word.push_back(i);
                word.push_back(j);
            }
            words.push_back(word);
            ++w;
        }
    }
    std::sort(words.begin(), words.end(), _cmpVectorSize);
    return words;
}

//[ processable collapsable vertex ]----------
class Vertex
{
public:
    Vertex *prev,*next;//process list structure
    Vertex *adj[4];//adjacency structure
    Vertex *rep;//equivalence structure
    int state;//additional sturcture
    Vertex *Beg(void);
    Vertex *End(void);
    Vertex *Rep(void);
    void move_before(Vertex *_next);
    void remove_before(void);
    int count(void);
    void equiv_to(Vertex *_v,Vertex *end);
    Vertex(void)
    {
        for(int i = 0;i<4;i++)
            adj[i]= NULL;
        prev = next = this;
        rep = this;
        state = 0;
    //logger.info() << "." |0;
    }
    Vertex(Vertex *_next)
    {
        for(int i = 0;i<4;i++)
            adj[i]= NULL;
        rep = this;
        state = 0;
        next =_next;
        prev = next->prev;//watch out for beg vertices
        next->prev = this;    
        prev->next = this;
    //logger.info() << "." |0;
    }
    ~Vertex(void)
    {
        // recursively calling the destructor may
        // exceed the maximum call stack size
        // if(next!= this) delete next;
        Vertex *curr = this;
        while (curr->next && curr->next != curr) {
            curr = curr->next;
        }

        while (curr != this) {
            curr = curr->prev;
            delete curr->next;
            curr->next = NULL;
        }
    //logger.info() << "-" |0;
    }
};
void Vertex::move_before(Vertex *_next)
{
    if(this ==_next || this ==_next->prev) return;
    next->prev = prev;
    prev->next = next;
    prev =_next->prev;
    prev->next = this;
    next =_next;
    _next->prev = this;
}
void Vertex::remove_before(void)
{
    Vertex *v = prev;
    prev->prev->next = this;
    prev = prev->prev;
    v->next = NULL;
    delete v;
}
Vertex *Vertex::Beg(void)
{
    Vertex *beg = this;
    while(beg!= beg->prev) beg = beg->prev;
    return beg;
}
Vertex *Vertex::End(void)
{
    Vertex *end = this;
    while(end!= end->next) end = end->next;
    return end;
}
Vertex *Vertex::Rep(void)
{
    while(rep!= rep->rep) rep = rep->rep;
    return rep;
}          
int Vertex::count(void)//excluding end
{
    int i = 0;
    for(Vertex *v = Beg()->next;v!= v->next;v = v->next)
        i++;
    return i;
}
void Vertex::equiv_to(Vertex *_v,Vertex *end)
{ //function calling this must hold beg as reference
    if(_v == this) return;
    _v->rep = this;
    _v->move_before(end);
    //fill out equivs
    for(Vertex *v = end->prev;v!= v->next;v = v->next)
    {
        v->Rep();
        for(int r = 0;r<4;r++)
        {
            Vertex *var = v->adj[r];
            if(var!= NULL)
            {
                var = var->Rep();
                Vertex *vrar = v->rep->adj[r];
                if(vrar!= NULL)
                {
                    vrar = vrar->Rep();
                    if(vrar!= var)
                    {
                        if(var!= this)//can't suicide
                        {
                            var->rep = vrar;
                            var->move_before(end);
                        }
                        else
                        {
                            vrar->rep = var;
                            vrar->move_before(end);
                        }
                    }
                }
            }
        }
    }
    //collapse rep trees
    for(Vertex *v = end->prev;v!= v->rep;v = v->prev)
        v->Rep();
    //collapse adj and state structure
    for(Vertex *v = end->prev;v!= v->rep;v = v->prev)
    {
        v->rep->state|= v->state;
        for(int r = 0;r<4;r++)
            if(v->adj[r]!= NULL)
            {
                v->adj[r]->rep->adj[r]= v->rep;
                v->rep->adj[r]= v->adj[r]->rep;
            }
    }
    //delete removed vertices
    while(end->prev->rep!= end->prev)
        end->remove_before();
}

//[ tabular group ]----------
Group::Group(std::vector<Word> words)
    : inv(0), whence(0)
{
    //create vertex structure
    Vertex *v_beg,*v_end;
    try{ v_beg = new Vertex(); }
    catch(std::bad_alloc){ mem_err(); }
    try{ v_beg->next = new Vertex(); }
    catch(std::bad_alloc){ mem_err(); }
        v_beg->next->prev = v_beg;
    try{ v_beg->next->next = new Vertex(); }
    catch(std::bad_alloc){ mem_err(); }
        v_beg->next->next->prev = v_beg->next;
    v_end = v_beg->next->next;

    //build free graph of left-multiplication
    for (Vertex *v = v_beg->next;v!= v->next;v = v->next) {
        for (int w=0; w<6; ++w) {
            if (!(v->state & (1<<w))) {       //i'm so tired...
                Word& word = words[w];
                Vertex *vnew = v;
                for (unsigned i=0; i<word.size(); ++i) {
                    int j = word[i];
                    if (vnew->adj[j] == NULL) {
                        try{ vnew->adj[j]= new Vertex(v_end); }
                        catch(std::bad_alloc){ mem_err(); }
                        vnew->adj[j]->adj[j]= vnew;
                    }
                    vnew->state |= 1<<w;
                    vnew = vnew->adj[j];
                }
                v->equiv_to(vnew, v_end);
            }
        }
    }
    //states == 63.
    logger.info() << "free group built, order = " << v_beg->count() |0;

    /*
    //find largest element and collapse mod 2
    for (Vertex *v = v_end->prev; v != v->next; v = v->next) {
        for (int r=0; r<4; ++r) {
            if (v->adj[r]->state > 0) {
                v->adj[r]->state = 0;
                v->adj[r]->move_before(v_end);
            }
        }
        v->state = 0;
    }
    v_beg->next->equiv_to(v_end->prev, v_end);
    */

    //count & name elements
    ord = 0;
    for (Vertex *v = v_beg->next; v != v->next; v = v->next) {
        v->state = ord++;
    }

    //logger.debug() << "quotient built, order = " << ord |0;

    //build left mult table from graph
    try{ _left = new int*[ord]; }
    catch(std::bad_alloc){ mem_err(); }
    for (int g = 0; g<ord; ++g) {
        try{ _left[g] = new int[4]; }
        catch(std::bad_alloc){ mem_err(); }
    }
    for(Vertex *v = v_beg->next; v != v->next; v = v->next) {
        for(int j = 0; j<4; ++j) {
            _left[v->state][j] = v->adj[j]->state;
        }
    }
    delete v_beg;
    logger.debug() << "left mult table built." |0;
    /*
    for(int c = 0;c<ord;c++)
    {
        logger.info() << "(" << left(c,0) |0;
        for(int j = 1;j<4;j++)
            logger.info() << " " << left(c,j) |0;
        logger.info() << ")" |0;
    }
    */

    //build inverse table
    whence.resize(ord, UNDEFINED);
    int largest = 0;
    Word reached;
    reached.reserve(ord);

    //  start with identity element
    whence[0] = IDENTITY;
    reached.push_back(0);
    //  parse all other words
    for (int i=0; i<ord; ++i) {
        for (int j=0; j<4; ++j) {
            int v = reached[i];
            int g = left(v,j);
            if (whence[g] == UNDEFINED) {
                whence[g] = j;
                largest = g;
                reached.push_back(g);
            }
        }
    }
    //  trace back to identity
    Word(ord, UNDEFINED).swap(inv);
    for (int g=0; g<ord; ++g) {
        if (inv[g] == UNDEFINED) { //for efficiency, traverse only half
            int g1 = g, g1_inv = 0;
            while (g1 != 0) {
                int j = whence[g1];
                g1_inv = left(g1_inv,j);
                g1     = left(g1,    j);
            }
            inv[g] = g1_inv;
            inv[g1_inv] = g;
        }
    }
    logger.debug() << "inverse table built." |0;
    /*
    for (int c=0; c<ord; ++c) {
        logger.info() << inv[c] << " " |0;
    }
    */

    const Logging::fake_ostream& os = logger.debug();
    os << "largest element = ";
    Word l = parse(largest);
    for (unsigned t=0; t<l.size(); ++t) {
        os << l[t];
    }
    os |0;
}
Group::~Group(void)
{
    for(int g=0; g<ord; ++g) {
        delete _left[g];
    }
    delete _left;
}

//[ cayley coset graph with point reps ]----------
class FaceRecognizer
{
    std::set<Ring> known;
public:
    bool operator() (Ring face)
    {
        std::sort(face.begin(), face.end());
        if (known.find(face) != known.end()) return true;
        known.insert(face);
        return false;
    }
    void clear () { known.clear(); }
};
Graph::Graph(const int *cartan,
             const std::vector<Word>& gens, //namesake
             const std::vector<Word>& v_cogens,
             const std::vector<Word>& e_gens,
             const std::vector<Word>& f_gens,
             const Vect& weights)
{
    //define symmetry group relations
    std::vector<Word> words = words_from_cartan(cartan);
    {
        const Logging::fake_ostream& os = logger.debug();
        os << "relations =";
        for (int w=0; w<6; ++w) {
            Word& word = words[w];
            os << "\n  ";
            for (unsigned i=0; i<word.size(); ++i) {
                os << word[i];
            }
        }
        os |0;
    }

    //check vertex stabilizer generators
    {
        const Logging::fake_ostream& os = logger.debug();
        os << "v_cogens =";
        for (unsigned w=0; w<v_cogens.size(); ++w) {
            const Word& jenn = v_cogens[w]; //namesake
            os << "\n  ";
            for (unsigned t=0; t<jenn.size(); ++t) {
                int j = jenn[t];
                os << j;
                Assert (0<=j and j<4,
                        "generator out of range: letter w["
                        << w << "][" << t << "] = " << j );
            }
        }
        os |0;
    }

    //check edge generators
    {
        const Logging::fake_ostream& os = logger.debug();
        os << "e_gens =";
        for (unsigned w=0; w<e_gens.size(); ++w) {
            const Word& edge = e_gens[w];
            os << "\n  ";
            for (unsigned t=0; t<edge.size(); ++t) {
                int j = edge[t];
                os << j;
                Assert (0<=j and j<4,
                        "generator out of range: letter w["
                        << w << "][" << t << "] = " << j );
            }
        }
        os |0;
    }

    //check face generators
    {
        const Logging::fake_ostream& os = logger.debug();
        os << "f_gens =";
        for (unsigned w=0; w<f_gens.size(); ++w) {
            const Word& face = f_gens[w];
            os << "\n  ";
            for (unsigned t=0; t<face.size(); ++t) {
                int j = face[t];
                os << j;
                Assert (0<=j and j<4,
                        "generator out of range: letter w["
                        << w << "][" << t << "] = " << j );
            }
        }
        os |0;
    }

    //build symmetry group
    Group group(words);
    logger.debug() << "group.ord = " << group.ord |0;

    //build subgroup
    std::vector<int> subgroup;  subgroup.push_back(0);
    std::set<int> in_subgroup;  in_subgroup.insert(0);
    for (unsigned g=0; g<subgroup.size(); ++g) {
        int g0 = subgroup[g];
        for (unsigned j=0; j<gens.size(); ++j) {
            int g1 = group.left(g0,gens[j]);
            if (in_subgroup.find(g1) != in_subgroup.end()) continue;
            subgroup.push_back(g1);
            in_subgroup.insert(g1);
        }
    }
    logger.debug() << "subgroup.ord = " << subgroup.size() |0;

    //build cosets and count ord
    std::map<int,int> coset; //maps group elements to cosets
    ord = 0; //used as coset number
    for (unsigned g=0; g<subgroup.size(); ++g) {
        int g0 = subgroup[g];
        if (coset.find(g0) != coset.end()) continue;

        int c0 = ord++;
        coset[g0] = c0;
        std::vector<int> members(1, g0);
        std::vector<int> others(0);
        for (unsigned i=0; i<members.size(); ++i) {
            int g1 = members[i];
            for (unsigned w=0; w<v_cogens.size(); ++w) {
                int g2 = group.left(g1, v_cogens[w]);
                if (coset.find(g2) != coset.end()) continue;
                coset[g2] = c0;
                members.push_back(g2);
            }
        }
    }
    logger.info() << "cosets table built: " << " ord = " << ord |0;

    //build edge lists
    std::vector<std::set<int> > neigh(ord);
    for (unsigned g=0; g<subgroup.size(); ++g) {
        int g0 = subgroup[g];
        int c0 = coset[g0];
        for (unsigned w=0; w<e_gens.size(); ++w) {
            int g1 = group.left(g0, e_gens[w]);
            Assert (in_subgroup.find(g1) != in_subgroup.end(),
                    "edge leaves subgroup");
            int c1 = coset[g1];
            if (c0 != c1) neigh[c0].insert(c1);
        }
    }
    //  make symmetric
    for (int c0=0; c0<ord; ++c0) {
        const std::set<int>& n = neigh[c0];
        for (std::set<int>::iterator c1=n.begin(); c1!=n.end(); ++c1) {
            neigh[*c1].insert(c0);
        }
    }
    //  build edge table
    adj.resize(ord);
    for (int c=0; c<ord; ++c) {
        adj[c].insert(adj[c].begin(), neigh[c].begin(), neigh[c].end());
    }
    neigh.clear();
    deg = adj[0].size();
    logger.info() << "edge table built: deg = " << deg |0;

    //define faces
    for (unsigned g=0; g<f_gens.size(); ++g) {
        const Word& face = f_gens[g];
        logger.debug() << "defining faces on " << face |0;
        Logging::IndentBlock block;

        //define basic face in group
        Ring basic(1,0);
//        g = 0;
        int g0 = 0;
        for (unsigned c=0; true; ++c) {
            g0 = group.left(g0, face[c%face.size()]);
            if (c >= face.size() and g0 == 0) break;
            if (in_subgroup.find(g0) != in_subgroup.end() and g0 != basic.back()) {
                basic.push_back(g0);
            }
        }
        for (unsigned c=0; c<basic.size(); ++c) {
            logger.debug() << "  corner: " << basic[c] |0;
        }
        logger.debug() << "sides/face (free) = " << basic.size() |0;

        //build orbit of basic face
        std::vector<Ring> faces_g;  faces_g.push_back(basic);
        FaceRecognizer recognized;  recognized(basic);
        for (unsigned i=0; i<faces_g.size(); ++i) {
            const Ring f = faces_g[i];
            for (unsigned j=0; j<gens.size(); ++j) {

                //right action of group on faces
                Ring f_j(f.size());
                for (unsigned c=0; c<f.size(); ++c) {
                    f_j[c] = group.right(f[c],gens[j]);
                }

                //add face
                if (not recognized(f_j)) {
                    faces_g.push_back(f_j);
                    //logger.debug() << "new face: " << f_j |0;
                } else {
                    //logger.debug() << "old face: " << f_j|0;
                }
            }
        }

        //hom face down to quotient graph
        recognized.clear();
        for (unsigned f=0; f<faces_g.size(); ++f) {
            const Ring face_g = faces_g[f];
            Ring face;
            face.push_back(coset[face_g[0]]);
            for (unsigned i=1; i<face_g.size(); ++i) {
                int c = coset[face_g[i]];
                if (c != face.back() and c != face[0]) {
                    face.push_back(c);
                }
            }
            if (face.size() < 3) continue;
            if (not recognized(face)) {
                faces.push_back(face);
            }
        }
    }
    ord_f = faces.size();
    logger.info() << "faces defined: order = " << ord_f |0;

    //define vertex coset
    std::vector<Word> vertex_coset;
    for (unsigned g=0; g<subgroup.size(); ++g) {
        int g0 = subgroup[g];
        if (coset[g0]==0) vertex_coset.push_back(group.parse(g0));
    }

    //build geometry
    std::vector<Mat> gen_reps(gens.size());
    points.resize(ord);
    build_geom(cartan, vertex_coset, gens, v_cogens, weights,
               gen_reps, points[0]);
    std::vector<int> pointed(ord,0);
    pointed[0] = true;
    logger.debug() << "geometry built" |0;

    //build point sets
    std::vector<int> reached(1,0);
    std::set<int> is_reached;
    is_reached.insert(0);
    for (unsigned g=0; g<subgroup.size(); ++g) {
        int g0 = reached[g];
        for (unsigned j=0; j<gens.size(); ++j) {
            int g1 = group.right(g0,gens[j]);
            if (is_reached.find(g1) == is_reached.end()) {
                if (not pointed[coset[g1]]) {
                    vect_mult(gen_reps[j], points[coset[g0]],
                                           points[coset[g1]]);
                    pointed[coset[g1]] = true;
                }
                reached.push_back(g1);
                is_reached.insert(g1);
            }
        }
    }
    logger.debug() << "point set built." |0;

    //build face normals
    normals.resize(ord_f);
    for (int f=0; f<ord_f; ++f) {
        Ring& face = faces[f];
        Vect &a = points[face[0]];
        Vect &b = points[face[1]];
        Vect &c = points[face[2]];
        Vect &n = normals[f];
        cross4(a,b,c, n);
        normalize(n);

        /*
        Assert1(fabs(inner(a,n)) < 1e-6,
                "bad normal: <n,a> = " << fabs(inner(a,n)));
        Assert1(fabs(inner(b,n)) < 1e-6,
                "bad normal: <n,b> = " << fabs(inner(b,n)));
        Assert1(fabs(inner(c,n)) < 1e-6,
                "bad normal: <n,b> = " << fabs(inner(c,n)));
        */
    }
    logger.debug() << "face normals built." |0;
}

void Graph::save (const char* filename)
{
    logger.info() << "exporting to " << filename |0;
    std::ofstream file(filename);

    unsigned num_edges = ord * deg / 2;
    file << "GRAPH\n";
    file << ord << " VERTICES\n";
    file << num_edges << " EDGES\n";
    for (int c0=0; c0<ord; ++c0) {
        for (int j=0; j<deg; ++j) {
            int c1 = adj[c0][j];
            if (c0 < c1) {
                file << c0 << ' ' << c1 << " 1\n";
            }
        }
    }
}

}

