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

#define LANG_ENG	0
#define LANG_GER	1
#define LANG_ITA	2
#define LANG_FRA	3
#define LANG_ESP	4


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
				// Wir updaten immer, da ein Kampf auch genutzt wird fur eine eventuelle Korrektur von n_ships
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
	// Ausgabeformat fur PHP-Interface in Zeilen:
	// 1: Status (erstes Zeichen 1 oder 0, danach optional Fehlermeldung)
	// 2: Gewinner (0 Angreifer, 1 Verteidiger)
	// 3: Vernichtete grobe orbitale Geschutze
	// 4: Vernichtete kleine orbitale Geschutze
	// 5: Result line (geez, wer hat diesen Schrott verbrochen)
	printf("1OK\n");

	if(winner == -1) printf("0\n");
	else printf("1\n");	

	printf("%i\n", move->n_large_orbital_defense);
	printf("%i\n", move->n_small_orbital_defense);

	c_db_result* res;

	/**
	 ** 03/04/08 - AC: Introduce localization of the message strings
	 **/
	int lang = LANG_ENG;
	const char *sAttackingShips,*sHull,*sDefendingShips,*sOrbital,*sLightOrbital,
		 *sAttackerWon,*sDefenderWon,*sAttackDestroy1,*sAttackDestroy2,*sOrbDestroyed,*sLOrbDestroyed;
	if(!db.query(&res, "SELECT language FROM user WHERE user_id = %i", move->user_id))
	{
		DEBUG_LOG("Could not query user %i language\n",move->user_id);
	}
	else {
		if(!res->fetch_row())
			DEBUG_LOG("Could not fetch user %i language!\n",move->user_id);
		else
		{
			if(strcmp(res->row[0],"GER") == 0)
				lang = LANG_GER;
			else if(strcmp(res->row[0],"ITA") == 0)
				lang = LANG_ITA;
		}
	}

	switch(lang)
	{
		// English
		case LANG_ENG:
			sAttackingShips = "<u>Attacking ships:</u><br>";
			sHull           = "%s (Hull: %i) <b>%ix</b><br>";
			sDefendingShips = "<br><u>Defending ships:</u><br>";
			sOrbital        = "Orbital cannon (stationary): <b>%i</b><br>";
			sLightOrbital   = "L. Orbital cannon (stationary): <b>%i</b><br>";
			sAttackerWon    = "<br>The attacking ships have won the fight.<br><br>";
			sDefenderWon    = "<br>The defending ships have won the fight.<br><br>";
			sAttackDestroy1 = "From the victorious ships became...<br>... <b>%i</b> destroyed<br>... <b>%i</b> damaged<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;of which <b>%i</b> seriously<br>&nbsp;&nbsp;&nbsp;&nbsp;and <b>%i</b> very seriously<br>";
			sOrbDestroyed   = "There were <b>%i</b> Orbital Cannon destroyed.<br>";
			sLOrbDestroyed  = "There were <b>%i</b> Light Orbital Cannon destroyed.<br>";
		break;
		// German
		case LANG_GER:
			sAttackingShips = "<u>Angreifende Schiffe:</u><br>";
			sHull           = "%s (Rumpf: %i) <b>%ix</b><br>";
			sDefendingShips = "<br><u>Verteidigende Schiffe:</u><br>";
			sOrbital        = "Orbitalgesch&uuml;tz (station&auml;r): <b>%i</b><br>";
			sLightOrbital   = "L. Orbitalgesch&uuml;tz (station&auml;r): <b>%i</b><br>";
			sAttackerWon    = "<br>Die angreifenden Schiffe haben den Kampf gewonnen.<br><br>";
			sDefenderWon    = "<br>Die verteidigenden Schiffe haben den Kampf gewonnen.<br><br>";
			sAttackDestroy1 = "Von den siegreichen Schiffen wurden...<br>... <b>%i</b> zerst&ouml;rt<br>... <b>%i</b> besch&auml;digt<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;davon <b>%i</b> stark<br>&nbsp;&nbsp;&nbsp;&nbsp;and <b>%i</b> sehr stark<br>";
			sOrbDestroyed   = "Es wurden <b>%i</b> Orbitalgesch&uuml;tze zerst&ouml;rt.<br>";
			sLOrbDestroyed  = "Es wurden <b>%i</b> kleine Orbitalgesch&uuml;tze zerst&ouml;rt.<br>";
		break;
		// Italian
		case LANG_ITA:
			sAttackingShips = "<u>Navi in attacco:</u><br>";
			sHull           = "%s (Scafo: %i) <b>%ix</b><br>";
			sDefendingShips = "<br><u>Navi in difesa:</u><br>";
			sOrbital        = "Cannoni Orbitali (stazionari): <b>%i</b><br>";
			sLightOrbital   = "Cannoni Orbitali L. (stazionari): <b>%i</b><br>";
			sAttackerWon    = "<br>Le navi in attacco hanno vinto la battaglia.<br><br>";
			sDefenderWon    = "<br>Le navi in difesa hanno vinto la battaglia.<br><br>";
			sAttackDestroy1 = "Delle navi vittoriose ci sono state...<br>... <b>%i</b> distrutte<br>... <b>%i</b> danneggiate<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;di cui <b>%i</b> seriamente<br>&nbsp;&nbsp;&nbsp;&nbsp;e <b>%i</b> molto seriamente<br>";
			sOrbDestroyed   = "Sono stati distrutti <b>%i</b> Cannoni Orbitali.<br>";
			sLOrbDestroyed  = "Sono stati distrutti <b>%i</b> Cannoni Orbitali Leggeri.<br>";
		break;
	}
	/* */

	if(!db.query(&res, "SELECT st.name, st.ship_torso, COUNT(s.ship_id) AS n_ships \
						FROM (ship_templates st) \
						INNER JOIN ships s ON s.template_id = st.id \
						WHERE s.fleet_id IN (%s) \
						GROUP BY st.id", argv[1])) {

		DEBUG_LOG("Could not query attackers template overview\n");
	}
	else {
		printf(sAttackingShips);

		while(res->fetch_row()) {
			printf(sHull, res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]));
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
		printf(sDefendingShips);

		if(move->n_large_orbital_defense > 0) printf(sOrbital, move->n_large_orbital_defense);
		if(move->n_small_orbital_defense > 0) printf(sLightOrbital, move->n_small_orbital_defense);
		
		while(res->fetch_row()) {
			printf(sHull, res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]));
		}
	}

	s_ship* cur_winner_ship;
	int n_winner_ships;
	
	if(winner == -1) {
		printf(sAttackerWon);

		cur_winner_ship = move->atk_ships;
		n_winner_ships = move->n_atk_ships;
	}
	else {
		printf(sDefenderWon);

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

	printf(sAttackDestroy1, n_winner_destroyed, (n_winner_light_damage + n_winner_heavy_damage + n_winner_very_heavy_damage));
	printf(sAttackDestroy2, n_winner_heavy_damage, n_winner_very_heavy_damage);

	if(move->destroyed_large_orbital_defense) printf(sOrbDestroyed, move->destroyed_large_orbital_defense);
	if(move->destroyed_small_orbital_defense) printf(sLOrbDestroyed, move->destroyed_small_orbital_defense);	

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
