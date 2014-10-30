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

#include "definitions.h"
#include "aligned_alloc.h"
#include <cstdlib> //for posix_memalign, free
#include <cstring> //for memset

//log levels
#define LOG_DEBUG1(mess)
#define LOG_INDENT_DEBUG1
//#define LOG_DEBUG1(mess) {logger.debug() << message |0;}
//#define LOG_INDENT_DEBUG1 Logging::IndentBlock block;

namespace nonstd
{

using Logging::logger;

unsigned roundDown (unsigned i) {
    unsigned j = 1;
    for (i>>=1; i; i>>=1) j <<= 1;
    return j;
}

void* alloc_blocks (size_t blockSize, size_t numBlocks)
{//allocates an aligned array, wraps posix_memalign
    logger.debug() << "Allocating " << numBlocks
                   << " blocks of size " << blockSize << 'B' |0;
    Logging::IndentBlock block;

    size_t alignment = roundDown(blockSize);
    size_t numBytes = blockSize * numBlocks;
    void* base;
#if defined(CYGWIN_HACKS) || defined(MAC_HACKS)
    if (NULL == (base = malloc(numBytes)))
#else
    if (posix_memalign(&base, alignment, numBytes))
#endif
    {
        logger.warning() << "posix_memalign failed" |0;
        return NULL;
    } else {
        //bzero (base, numBytes); //initializes to all zeros
        return base;
    }   
}
void free_blocks (void* base)
{//just wraps free()
    logger.debug() << "Freeing blocks" |0;
    Logging::IndentBlock block;

    free(base);
}
void clear_block (void* base, size_t blockSize)
{//sets data to zero, wraps memset
    LOG_DEBUG1( "Clearing block of size " << blockSize << 'B' )
    Logging::IndentBlock block;

    std::memset(base, 0, blockSize);
}
void copy_blocks (void* destin_base, const void* source_base,
                  size_t blockSize, size_t numBlocks)
{//justs wraps for memcpy
    LOG_DEBUG1( "Copying blocks" )
    Logging::IndentBlock block;

    memcpy (destin_base, source_base, blockSize * numBlocks);
}

}

