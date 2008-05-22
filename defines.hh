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


// ------------- Konfigurations-Makros -------------

// Diese Konstante gibt das Output-Level des Schedulers an.
// HINWEIS: NUR BEI VERBOSE = 0 KANN DAS BINARY IM PHP-SCHEDULER BENUTZT WERDEN !
//
//	0 = nur Status-Ausgaben und kompaktes Kampfergebnis, für php-scheduler
//	1 = normale Anzeige für standalone-Betrieb, detailiertere Angaben und human-readable
//	2 = normale Anzeige mit Zeitmessungen
//	3 = ausführliche Anzeige für standalone-Betrieb und Kampfdetails
//  4 = zusätzlich werden noch alle Flotten/Schiffe/Templates vor dem Berechnen aufgeführt
#define VERBOSE 0


// Diese Konstante setzt den Scheduler in den Simulationsmodus, in
// dem er KEINE ÄNDERUNGEN SPEICHERT.
//#define SIMULATOR


// Falls definiert, zerstören sich die Kazon Torpedos nach einigen Angriffen selbst
#define KAZON_TORPEDO_SUICIDE

// ------------- Konfigurations-Makros -------------


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

// vorallem für floats gedacht, aber für alle Typen anwendbar
// (im Gegensatz zu denen aus libc...)
#define fabs(x) ((x > 0) ? x : -x)
#define fmin(a, b) ((a > b) ? b : a)
#define fmax(a, b) ((a > b) ? a : b)

using namespace std;

double get_current_time();

int make_cstring(char** str_pointer, const char* str_content, ...);

int rand_uint_range(int rand_min, int rand_max);


#endif
