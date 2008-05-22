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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.hh"

#ifdef _WIN32
	#include <windows.h>
	#include <sys/timeb.h>
	
	int gettimeofday (timeval* tp, ...) {
		timeb tb;
		
		ftime(&tb);
		
		tp->tv_sec = tb.time;
		tp->tv_usec = tb.millitm * 1000;
		
		return 0;
	}
#else
	#include <sys/time.h>
#endif


double get_current_time() {
	timeval cur_time;
	
	gettimeofday(&cur_time, 0);
	
	return ((double)cur_time.tv_sec + ((double)cur_time.tv_usec / 1000000));
}

int make_cstring(char** str_pointer, const char* str_content, ...) {
	int buf_len = 128;
    int n_chars = -1;
	
	va_list arg_pointer;
	va_start(arg_pointer, str_content);
	
	while(n_chars == -1 || n_chars >= buf_len) {
		safe_delete_array(*str_pointer);
		
		if(n_chars == -1)
			buf_len *= 2;
		else
			buf_len = n_chars + 1;
		
		*str_pointer = new char[buf_len];
		
		n_chars = vsnprintf(*str_pointer, buf_len, str_content, arg_pointer);
	}
	
	va_end(arg_pointer);
	
	return n_chars;
}

int rand_uint_range(int rand_min, int rand_max) {
	return (rand() % (rand_min + rand_max));
}
