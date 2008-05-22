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


#include <map>

#include "db_core.hh"
#include "combat.hh"


extern c_db_core db;

static map<int, float>::iterator user_map_it;
static map<int, float> user_xp_map;

// Was gemacht werden muss:
//	- Flotten updaten (Zuladung)


static void update_ships(s_ship* cur_ship, int n_ships, int* ship_counter) {
	for(int i = 0; i < n_ships; ++i) {
		// Im syslog traten komische Berichte auf von UPDATEs auf hitpoints = 0 WHERE ship_id = 0
		if(cur_ship->ship_id == 0) {
			++cur_ship;
			
			DEBUG_LOG("Encountered ship_id == 0 while update_ships()");
			
			continue;
		}
		
		if(cur_ship->changed == true) {
		
			if(cur_ship->hitpoints < 1.0) {
				if(cur_ship->xp_gained > 0.0) {
					if(map_key_exists(user_xp_map, cur_ship->user_id, user_map_it)) {
						user_map_it->second += cur_ship->xp_gained;
					}
					else {
						user_xp_map[cur_ship->user_id] = cur_ship->xp_gained;
					}
				}

#ifndef SIMULATOR
				if(!db.query("DELETE FROM ships WHERE ship_id = %i", cur_ship->ship_id)) {
					DEBUG_LOG("Could not delete ship %i\n", cur_ship->ship_id);
				}
				
				DEBUG_LOG("DELETE FROM ships WHERE ship_id = %i", cur_ship->ship_id);
#endif
				
				--cur_ship->fleet->n_ships;
				
				if(cur_ship->tpl.ship_torso == SHIP_TORSO_TRANSPORTER) --cur_ship->fleet->n_transporter;
			} // end-if-hitpoints
			else {
			
#ifndef SIMULATOR								
				if(!db.query("UPDATE ships SET hitpoints = %i, experience = experience + %i WHERE ship_id = %i", (int)cur_ship->hitpoints, (int)cur_ship->xp_gained, cur_ship->ship_id)) {
					DEBUG_LOG("Could not update ship %i\n", cur_ship->ship_id);
				}
				
				DEBUG_LOG("UPDATE ships SET hitpoints = %i, experience = experience + %i WHERE ship_id = %i", (int)cur_ship->hitpoints, (int)cur_ship->xp_gained, cur_ship->ship_id);				
#endif
				
				if(cur_ship->xp_gained > 0.0) {
					if(map_key_exists(user_xp_map, cur_ship->user_id, user_map_it)) {
						user_map_it->second += cur_ship->xp_gained;
					}
					else {
						user_xp_map[cur_ship->user_id] = cur_ship->xp_gained;
					}
				}
				
				++(*ship_counter);
			} // end-else-hitpoints
			
		} // end-if-changed
		
		++cur_ship;
		
	} // end-for
}

static void update_fleets(s_fleet* cur_fleet, int n_fleets) {
	int n_resources, n_units;
	
	for(int i = 0; i < n_fleets; ++i) {
		// Flotte wurde zerstört
		if(cur_fleet->n_ships == 0) {
#ifndef SIMULATOR
			if(!db.query("DELETE FROM ship_fleets WHERE fleet_id = %i", cur_fleet->fleet_id)) {
				DEBUG_LOG("Could not delete fleet %i\n", cur_fleet->fleet_id);
			}
#endif
		}
		else {
			n_resources = (cur_fleet->resource_1 + cur_fleet->resource_2 + cur_fleet->resource_3);
			n_units = (cur_fleet->resource_4 + cur_fleet->unit_1 + cur_fleet->unit_2 + cur_fleet->unit_3 + cur_fleet->unit_4 + cur_fleet->unit_5 + cur_fleet->unit_6);
		
			if( (n_resources > 0) || (n_units > 0) ) {
				// Waren truncate!
			}
			else {
				// Wir updaten immer, da ein Kampf auch genutzt wird für eine eventuelle Korrektur von n_ships
				// und es nicht so viele Flotten bei einem Kampf gibt
			
#ifndef SIMULATOR
				if(!db.query("UPDATE ship_fleets SET n_ships = %i WHERE fleet_id = %i", cur_fleet->n_ships, cur_fleet->fleet_id)) {
					DEBUG_LOG("Could not update fleet %i\n", cur_fleet->fleet_id);
				}
#endif
			}
		}
	}
}


bool finish_combat(s_move_data* move, int winner, char** argv) {
#if VERBOSE >= 1
#else
	// Ausgabeformat für PHP-Interface in Zeilen:
	// 1: Status (erstes Zeichen 1 oder 0, danach optional Fehlermeldung)
	// 2: Gewinner (0 Angreifer, 1 Verteidiger)
	// 3: Vernichtete große orbitale Geschütze
	// 4: Vernichtete kleine orbitale Geschütze	
	// 5: Result line (geez, wer hat diesen Schrott verbrochen)
	printf("1OK\n");
		
	if(winner == -1) printf("0\n");
	else printf("1\n");	
	
	printf("%i\n", move->n_large_orbital_defense);
	printf("%i\n", move->n_small_orbital_defense);
	
	c_db_result* res;
	
	if(!db.query(&res, "SELECT st.name, st.ship_torso, COUNT(s.ship_id) AS n_ships \
						FROM (ship_templates st) \
						INNER JOIN ships s ON s.template_id = st.id \
						WHERE s.fleet_id IN (%s) \
						GROUP BY st.id", argv[1])) {
						
		DEBUG_LOG("Could not query attackers template overview\n");
	}
	else {
		printf("<u>Angreifende Schiffe:</u><br>");
		
		while(res->fetch_row()) {
			printf("%s (Rumpf: %i) <b>%ix</b><br>", res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]));
		}
	}
	
	safe_delete(res);
	
	if(!db.query(&res, "SELECT st.name, st.ship_torso, COUNT(s.ship_id) AS n_ships \
						FROM (ship_templates st) \
						INNER JOIN ships s ON s.template_id = st.id \
						WHERE s.fleet_id IN (%s) \
						GROUP BY st.id", argv[2])) {
						
		DEBUG_LOG("Could not query attackers template overview\n");
	}
	else {
		printf("<br><u>Verteidigende Schiffe:</u><br>");
		
		if(move->n_large_orbital_defense > 0) printf("Orbitalgesch&uuml;tz (station&auml;r): <b>%i</b><br>", move->n_large_orbital_defense);
		if(move->n_small_orbital_defense > 0) printf("L. Orbitalgesch�&uuml;z (station&auml;r): <b>%i</b><br>", move->n_small_orbital_defense);
		
		while(res->fetch_row()) {
			printf("%s (Rumpf: %i) <b>%ix</b><br>", res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]));
		}
	}
	
	s_ship* cur_winner_ship;
	int n_winner_ships;
	
	if(winner == -1) {
		printf("<br>Die angreifenden Schiffe haben den Kampf gewonnen.<br><br>");
		
		cur_winner_ship = move->atk_ships;
		n_winner_ships = move->n_atk_ships;
	}
	else {
		printf("<br>Die verteidigenden Schiffe haben den Kampf gewonnen.<br><br>");
		
		cur_winner_ship = move->dfd_ships;
		n_winner_ships = move->n_dfd_ships;
	}
	
	int n_winner_destroyed = 0, n_winner_light_damage = 0, n_winner_heavy_damage = 0, n_winner_very_heavy_damage = 0;
	float hp_status = 0.0;
    
    for(int i = 0; i < n_winner_ships; ++i) {
		if(cur_winner_ship[i].hitpoints <= 0) {
			++n_winner_destroyed;
		}
		else {
			 hp_status = ((cur_winner_ship[i].previous_hitpoints - cur_winner_ship[i].hitpoints) / cur_winner_ship[i].tpl.value_5);
			 
			 if(hp_status == 0.0) { }
			 else if(hp_status <= 0.25) ++n_winner_light_damage;
			 else if(hp_status <= 0.5) ++n_winner_heavy_damage;
			 else ++n_winner_very_heavy_damage;
		}			
    }
    
    printf("Von den siegreichen Schiffen wurden...<br>... <b>%i</b> zerst&ouml;rt<br>... <b>%i</b> besch&auml;digt<br>", n_winner_destroyed, (n_winner_light_damage + n_winner_heavy_damage + n_winner_very_heavy_damage));
    printf("&nbsp;&nbsp;&nbsp;&nbsp;davon <b>%i</b> stark<br>&nbsp;&nbsp;&nbsp;&nbsp;und <b>%i</b> sehr stark<br>", n_winner_heavy_damage, n_winner_very_heavy_damage);
	  	
	if(move->destroyed_large_orbital_defense) printf("Es wurden <b>%i</b> Orbitalgesch&uuml;tze zerst&ouml;rt.<br>", move->destroyed_large_orbital_defense);
	if(move->destroyed_small_orbital_defense) printf("Es wurden <b>%i</b> kleine Orbitalgesch&uuml;tze zerst&ouml;rt.<br>", move->destroyed_small_orbital_defense);	
	
	printf("<br>\n");
	
#endif


	if( (move->destroyed_large_orbital_defense > 0) || (move->destroyed_small_orbital_defense > 0) ) {
#ifndef SIMULATOR
		if(!db.query("UPDATE planets "
					 "SET building_10 = building_10 - %i, "
					  	 "building_13 = building_13 - %i "
					 "WHERE planet_id = %i", move->destroyed_large_orbital_defense, move->destroyed_small_orbital_defense, move->dest)) {
			
			DEBUG_LOG("Could not update planetary data\n");
		}
#endif
	}
	
	int new_n_atk_ships = 0, new_n_dfd_ships = 0;

	update_ships(move->atk_ships, move->n_atk_ships, &new_n_atk_ships);
	update_ships(move->dfd_ships, move->n_dfd_ships, &new_n_dfd_ships);
	
	update_fleets(move->atk_fleets, move->n_atk_fleets);
	update_fleets(move->dfd_fleets, move->n_dfd_fleets);
	
	for(user_map_it = user_xp_map.begin(); user_map_it != user_xp_map.end(); ++user_map_it) {
#ifndef SIMULATOR
		if(!db.query("UPDATE user SET user_honor = user_honor + %i WHERE user_id = %i", ((int)user_map_it->second / 10), user_map_it->first)) {
			DEBUG_LOG("Could not update user honor data\n");
		}
#endif
	}
		
	return true;
}
