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

#ifndef JENN_DEFINITIONS_H
#define JENN_DEFINITIONS_H

#include <iostream>
#include <sys/time.h>
#include <cstdlib>

template<class T> inline T min (T lhs, T rhs) { return (lhs<rhs) ? lhs:rhs; }
template<class T> inline T max (T lhs, T rhs) { return (lhs>rhs) ? lhs:rhs; }

//================================ debugging ================================

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL 0
  #define NDEBUG
#endif

#define LATER() {logger.error()\
    << "control reached unfinished code:\n\t"\
    << __FILE__ << " : " << __LINE__ << "\n\t"\
    << __PRETTY_FUNCTION__ << "\n" |0;\
    abort();}

#define Assert(cond,mess) {if (!(cond)) {logger.error()\
    << mess << "\n\t"\
    << __FILE__ << " : " << __LINE__ << "\n\t"\
    << __PRETTY_FUNCTION__ << "\n" |0;\
    abort();}}

#define Assert1(cond,mess) Assert(((DEBUG_LEVEL<1)or(cond)),mess)
#define Assert2(cond,mess) Assert(((DEBUG_LEVEL<2)or(cond)),mess)
#define Assert3(cond,mess) Assert(((DEBUG_LEVEL<3)or(cond)),mess)

//================================ logging ================================

namespace Logging
{

#ifdef LOG_FILE
extern std::ofstream logFile;
#endif

class fake_ostream
{
private:
    const bool m_live;
public:
    fake_ostream (bool live) : m_live(live) {}
    template <class Message>
    const fake_ostream& operator << (const Message& message) const
    {
#ifdef LOG_FILE
        if (m_live) { logFile << message; }
#else
        if (m_live) { std::cout << message; }
#endif
        return *this;
    }
    const fake_ostream& operator | (int) const
    {
#ifdef LOG_FILE
        if (m_live) { logFile << std::endl; }
#else
        if (m_live) { std::cout << std::endl; }
#endif
        return *this;
    }
};

const fake_ostream live_out(true), dead_out(false);

//title/section label
void title (std::string name);

//indentation stuff
const int length = 64;
const int stride = 2;
const char* const spaces = "                                                                " + length;
extern int indentLevel;
inline void indent ();
inline void outdent ();
inline const char* indentation () { return spaces - indentLevel * stride; }
class IndentBlock
{
public:
    IndentBlock () { indent (); }
    ~IndentBlock () { outdent (); }
};

//log channels
enum LogLevel {ERROR, WARNING, INFO, DEBUG, DEBUG1};
const std::string levelNames[5] =
{
    "ERROR",  //red
    //"\033[36mWARNING\033[39m",//cyan
    "warning",//red
    "info",   //green
    "debug",  //yellow
    "debug1"  //yellow
};
class Logger
{
private:
    const std::string m_name;
    const LogLevel m_level;
public:
    Logger (std::string name, LogLevel level = INFO)
        : m_name(name), m_level(level) {}
    const fake_ostream& active_log (LogLevel level) const;

    //heading
    void static heading (char* label);

    //log
    const fake_ostream& log (LogLevel level) const
    { return (level <= m_level) ? active_log(level) : dead_out; }
    const fake_ostream& error   () const { return log(ERROR); }
    const fake_ostream& warning () const { return log(WARNING); }
    const fake_ostream& info    () const { return log(INFO); }
    const fake_ostream& debug   () const { return log(DEBUG); }
    const fake_ostream& debug1  () const { return log(DEBUG1); }

    //indent
    void indent_ (LogLevel level) const { if (level <= m_level) indent(); }
    void indent_info   () const { return indent_(INFO); }
    void indent_debug  () const { return indent_(DEBUG); }
    void indent_debug1 () const { return indent_(DEBUG1); }

    //outdent
    void outdent_ (LogLevel level) const { if (level <= m_level) outdent(); }
    void outdent_info   () const { return outdent_(INFO); }
    void outdent_debug  () const { return outdent_(DEBUG); }
    void outdent_debug1 () const { return outdent_(DEBUG1); }
};
const Logger logger ("general", INFO);

inline void indent ()
{
    ++indentLevel;
    Assert1(indentLevel*stride < length, "indent level overflow");
}
inline void outdent ()
{
    --indentLevel;
    Assert1(indentLevel >= 0, "indent level underflow");
}

}

//[ memory stuff ]------------------------------------------
inline void mem_err(void)
{
    Logging::logger.error() << "\nerror: out of memory";
}

//[ time measurement ]----------------------------------
long time_seed ();
float elapsed_time ();
float time_difference ();

#endif

