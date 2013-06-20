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

#ifndef NONSTD_ALIGNED_VECT_H
#define NONSTD_ALIGNED_VECT_H

#include "aligned_alloc.h"

namespace nonstd
{

template<class T>
class aligned_vect
{
    T* m_data;
public:
    aligned_vect () : m_data(NULL) {}
    aligned_vect (int size)
        : m_data (static_cast<Vect*>(alloc_blocks(sizeof(T), size))) {}
    aligned_vect (int size, int block)
        : m_data (static_cast<Vect*>(alloc_blocks(sizeof(T)*block, size))) {}
    ~aligned_vect () { if (m_data) free_blocks(m_data); }

    T& operator[] (int i)       { return m_data[i]; }
    T  operator[] (int i) const { return m_data[i]; }
};

}
#endif

