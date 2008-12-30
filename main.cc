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

#include "db_core.hh"
#include "combat.hh"
#include <syslog.h>

c_db_core db;


// moves_sched [atk_fleet_ids_str] [dfd_fleet_ids_str] [dest] [large_orbital] [small_orbital] [database] [user] [password]

int main(int argc, char** argv) {

	openlog ("combat-bin", LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	if(argc != 9) {
		printf("0Wrong Parameter Count\n");
		DEBUG_LOG("0Wrong Parameter Count\n");	
		closelog();
		return 0;
	}

	// Host , Database, User, Password

	db.init_by_str((char*)"localhost", argv[6], argv[7], argv[8]);

	s_move_data* move = new s_move_data;
	memset(move, 0, sizeof(s_move_data));

	int winner = 0;

#if VERBOSE >= 2
	double start_time, end_time;

	start_time = get_current_time();
#endif

	if(!prepare_combat(move, argv)) goto exit;

#if VERBOSE >= 2
	end_time = get_current_time();

	DEBUG_LOG("* prepare complete, time: %.3lfs\n", end_time - start_time);
#else
	DEBUG_LOG("* prepare complete\n");
#endif


#if VERBOSE >= 2
	start_time = get_current_time();
#endif

	winner = process_combat(move);

	if(winner == 0) goto exit;

#if VERBOSE >= 2
	end_time = get_current_time();

	DEBUG_LOG("* process complete, time: %.3lfs\n", end_time - start_time);
#else
	DEBUG_LOG("* process complete\n");
#endif


#if VERBOSE >= 2
	start_time = get_current_time();
#endif

	finish_combat(move, winner, argv);

#if VERBOSE >= 2
	end_time = get_current_time();

	DEBUG_LOG("* finish complete, time: %.3lfs\n", end_time - start_time);
#else
	DEBUG_LOG("* finish complete\n");
#endif


exit:
	safe_delete_array(move->atk_fleets);
	safe_delete_array(move->atk_ships);
	safe_delete_array(move->dfd_fleets);
	safe_delete_array(move->dfd_ships);

	safe_delete(move);
	closelog();
	return 0;
}
