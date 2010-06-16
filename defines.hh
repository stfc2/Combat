/*    
	This file is part of STFC.
	Copyright 2006-2007 by Michael Krauss (info@stfc2.de) and Tobias Gafner
		
	STFC is based on STGC,
	Copyright 2003-2007 by Florian Brede (florian_brede@hotmail.com) and Philipp Schmidt
	
    STFC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    STFC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __DEFINES_H__
#define __DEFINES_H__
#include <syslog.h>
#include <time.h>


// ------------- Configuration Macros -------------

// This constant indicates the output level to the scheduler.
// NOTE: ONLY WITH VERBOSE = 0 THE BINARY CAN BE USED IN THE PHP SCHEDULER !
//
//	0 = only expenditures for status and compact combat result, for php schedulers
//	1 = normal mode for standalone operation, detailed information and human-readable
//	2 = normal display with time measurements
//	3 = detailed display for standalone operation and fight details
//	4 = additionally output  for fleets / ships / templates listed before calculate
#define VERBOSE 0


// This constant sets the scheduler in simulation mode, in
// which he NO CHANGES STORED.
//#define SIMULATOR


// If defined, the Kazon torpedoe destroys itself after some attacks
//#define KAZON_TORPEDO_SUICIDE

// ------------- Configuration Macros -------------


#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SIMULATOR	
	#define DEBUG_LOG(a, ...) printf(a, ##__VA_ARGS__)
#else
	#define DEBUG_LOG(a, ...) syslog (LOG_NOTICE, a, ##__VA_ARGS__)
#endif


#define safe_delete(d) if(d) { delete d; d = NULL; }
#define safe_delete_array(d) if(d) { delete[] d; d = NULL; }
#define safe_delete_packet(d) if(d) { delete[] d; d = NULL; }
#define safe_delete_packet_array(d) if(d) { delete[] d; d = NULL; }
#define safe_delete_list(l) while(!l.empty()) { delete l.front(); l.pop_front(); }
#define safe_delete_list_reverse(l) while(!l.empty()) { delete l.back(); l.pop_back(); }
#define safe_delete_map_values(m) while(!m.empty()) { delete m.begin()->second; m.erase(m.begin()); }
#define safe_delete_queue(q) while(!q.empty()) { delete q.front(); q.pop(); } 	

#define map_key_exists(m, k, i) (!((i = m.find(k)) == m.end()))
#define delete_map_key(m, k, i) if((i = m.find(k)) != m.end()) { m.erase(i); }

// especially for floats thought, but applicable to all types
// (unlike those from libc ...)
#define fabs(x) ((x > 0) ? x : -x)
#define fmin(a, b) ((a > b) ? b : a)
#define fmax(a, b) ((a > b) ? a : b)

using namespace std;

double get_current_time();

int make_cstring(char** str_pointer, const char* str_content, ...);

int rand_uint_range(int rand_min, int rand_max);


#endif
