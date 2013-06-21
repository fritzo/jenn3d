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

#include <sys/time.h>

//================================ logging ================================

namespace Logging
{
#ifdef LOG_FILE
//std::ofstream logFile(LOG_FILE, std::ios_base::app);
std::ofstream logFile(LOG_FILE);
#endif

//title/section label
void title (std::string name)
{
    live_out << "================ " << name
             << " ================" |0;
}

//indentation stuff
int indentLevel(0);

//log channels
const fake_ostream& Logger::active_log (LogLevel level) const
{
    return live_out << levelNames[level] << '\t'
                    << m_name << '\t'
                    << indentation();
}

}

//[ time measurement ]----------------------------------
long time_seed () { return clock(); }
/*
float elapsed_time ()
{
    static timeval start, now;
    static int started(gettimeofday(&start, NULL));
    gettimeofday(&now, NULL);
    float result = now.tv_sec - start.tv_sec;
    result += 1e-6 * (now.tv_usec - start.tv_usec);
    return result;
}
float time_difference ()
{
    static timeval g_this_time, g_last_time;
    static int g_time_is_available(gettimeofday(&g_last_time, NULL));
    gettimeofday(&g_this_time, NULL);
    float result = g_this_time.tv_sec - g_last_time.tv_sec;
    result += 1e-6 * (g_this_time.tv_usec - g_last_time.tv_usec);
    gettimeofday(&g_last_time, NULL);
    return result;
}
*/
float elapsed_time ()
{
    return (1.0 / CLOCKS_PER_SEC) * clock();
}
float time_difference ()
{
    static long g_this_time, g_last_time;
    g_this_time = clock();
    float result = (1.0 / CLOCKS_PER_SEC) * (g_this_time - g_last_time);
    g_last_time = g_this_time;
    return result;
}

