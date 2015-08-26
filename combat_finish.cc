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

#include <math.h>
#include <map>
#include <unordered_map>


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

// What needs to be made:
//	- Updating fleets (Payload)


static void update_ships(s_ship* cur_ship, int n_ships, int* ship_counter) {
    
    int experience;
    int rec_hp;
    
	for(int i = 0; i < n_ships; ++i) {
		// Reports into syslog on occurred comic UPDATEs on hitpoints = 0 WHERE ship_id = 0
		if(cur_ship->ship_id == 0) {
			++cur_ship;

			DEBUG_LOG("Encountered ship_id == 0 while update_ships()");

			continue;
		}

		if(cur_ship->changed == true) {
                    
                        experience = fmin (200, cur_ship->experience);

			if(cur_ship->knockout) {
				if(cur_ship->xp_gained > 0.0) {
					if(map_key_exists(user_xp_map, cur_ship->user_id, user_map_it)) {
						user_map_it->second += cur_ship->xp_gained;
					}
					else {
						user_xp_map[cur_ship->user_id] = cur_ship->xp_gained;
					}
				}
				if(cur_ship->hitpoints < 1.0) {
					if(!db.query((char*)"DELETE FROM ships WHERE ship_id = %i", cur_ship->ship_id)) {
						DEBUG_LOG("Could not delete ship %i\n", cur_ship->ship_id);
					}
#if VERBOSE >= 2
					DEBUG_LOG("DELETE FROM ships WHERE ship_id = %i", cur_ship->ship_id);
#endif

					--cur_ship->fleet->n_ships;
					if(cur_ship->tpl.ship_torso == SHIP_TORSO_TRANSPORTER) --cur_ship->fleet->n_transporter;
				}
				else {
                                        // Damage Control
                                        if(cur_ship->previous_hitpoints > cur_ship->hitpoints) {
                                                rec_hp = (cur_ship->previous_hitpoints - cur_ship->hitpoints) * ((5 + cur_ship->dmg_ctrl)/100);
                                                cur_ship->hitpoints = fmin(cur_ship->previous_hitpoints, floor(cur_ship->hitpoints + rec_hp));
                                        }
                                        
					if(!db.query((char*)"UPDATE ships SET hitpoints = %i, experience = %i, torp = %i WHERE ship_id = %i", (int)cur_ship->hitpoints, experience, (int)cur_ship->torp, cur_ship->ship_id)) {
						DEBUG_LOG("Could not update ship %i\n", cur_ship->ship_id);
					}
				}

			} // end-knockout
			else {  

                                // Damage Control
                                if(cur_ship->previous_hitpoints > cur_ship->hitpoints) {
                                        rec_hp = (cur_ship->previous_hitpoints - cur_ship->hitpoints) * ((5 + cur_ship->dmg_ctrl)/100);
                                        cur_ship->hitpoints = fmin(cur_ship->previous_hitpoints, floor(cur_ship->hitpoints + rec_hp));
                                }

				if(!db.query((char*)"UPDATE ships SET hitpoints = %i, experience = %i, torp = %i WHERE ship_id = %i", (int)cur_ship->hitpoints, experience, (int)cur_ship->torp, cur_ship->ship_id)) {
					DEBUG_LOG("Could not update ship %i\n", cur_ship->ship_id);
				}

#if VERBOSE >= 2
				DEBUG_LOG("UPDATE ships SET hitpoints = %i, experience = %i, torp = %i WHERE ship_id = %i", (int)cur_ship->hitpoints, (int)cur_ship->xp_gained, (int)cur_ship->torp, cur_ship->ship_id);
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
			} //

		} // end-if-changed

		++cur_ship;
		
	} // end-for
}

static void update_fleets(s_fleet* cur_fleet, int n_fleets, bool winner, int tick) {
	int n_resources, n_units, new_move_id, system_id, distance_id, newposition_id, tick_start, tick_end;
        c_db_result* res;
	
	for(int i = 0; i < n_fleets; ++i) {
		// Fleet was destroyed
		if(cur_fleet->n_ships == 0) {
#ifndef SIMULATOR
			if(!db.query((char*)"DELETE FROM ship_fleets WHERE fleet_id = %i", cur_fleet->fleet_id)) {
				DEBUG_LOG("Could not delete fleet %i\n", cur_fleet->fleet_id);
			}
#endif
		}
		else {
			n_resources = (cur_fleet->resource_1 + cur_fleet->resource_2 + cur_fleet->resource_3);
			n_units = (cur_fleet->resource_4 + cur_fleet->unit_1 + cur_fleet->unit_2 + cur_fleet->unit_3 + cur_fleet->unit_4 + cur_fleet->unit_5 + cur_fleet->unit_6);

			if( (n_resources > 0) || (n_units > 0) ) {
				// Truncate goods!
			}
			else {
				// We always update, since a fight is also used for a possible correction of n_ships
				// and not so many fleets are in a battle

#ifndef SIMULATOR
				if(!db.query((char*)"UPDATE ship_fleets SET n_ships = %i WHERE fleet_id = %i", cur_fleet->n_ships, cur_fleet->fleet_id)) {
					DEBUG_LOG("Could not update fleet %i\n", cur_fleet->fleet_id);
				}
#endif
			}
                        
                        // CD - Fleet Movement Generator
                        // CD - Let's give it a try
                        
                        if(winner) continue;
                                
                        tick_start = tick;
                        tick_end   = tick+4;
                        
                        if((cur_fleet->position != 0) && (cur_fleet->position == cur_fleet->homebase)) {
                                // Fleet was stationary at her own homebase; will LOOP on the planet
                                if(!db.query((char*)"INSERT INTO scheduler_shipmovement"
                                    "(user_id, move_status, move_exec_started, start, dest, move_begin, move_finish, n_ships, action_code, action_data)"
                                    "VALUES (%i, 0, 0, %i, %i, %i, %i, %i, 28, 0)"
                                    ,cur_fleet->owner, cur_fleet->position, cur_fleet->position, tick_start, tick_end, cur_fleet->n_ships )) {
                                        DEBUG_LOG("Could not insert new movement data for fleet %i\n", cur_fleet->fleet_id);
                                }
                                
                                new_move_id = (int)db.last_insert_id();
                                
                                if(!db.query((char*)"UPDATE ship_fleets SET planet_id = 0, move_id = %i WHERE fleet_id = %i", new_move_id, cur_fleet->fleet_id )) {
                                        DEBUG_LOG("Could not update fleet %i\n", cur_fleet->fleet_id);
                                }
                                continue;
                        }
                        
                        if(cur_fleet->position != 0) {
                                // Fleet was stationary, will try to move on a near planet
                                newposition_id = cur_fleet->position;
                                if(!db.query(&res, (char*)"SELECT system_id, planet_distance_id FROM planets WHERE planet_id = %i", cur_fleet->position)) {
                                        DEBUG_LOG("Could not read system data for fleet %i\n", cur_fleet->fleet_id);
                                }
                                res->fetch_row();
                                system_id   = atoi(res->row[0]);
                                distance_id = atoi(res->row[1]);
                                if(!db.query(&res, (char*)"SELECT planet_id FROM planets WHERE system_id = %i AND planet_id <> %i AND "
                                                          "planet_distance_id > %i ORDER BY planet_distance_id ASC LIMIT 0,1", system_id, cur_fleet->position, distance_id)) {
                                        DEBUG_LOG("Could not get destination data for fleet %i\n", cur_fleet->fleet_id);
                                }
                                
                                if(res->num_rows() > 0) {
                                    res->fetch_row();
                                    newposition_id = atoi(res->row[0]);
                                }
                                if(!db.query((char*)"INSERT INTO scheduler_shipmovement"
                                    "(user_id, move_status, move_exec_started, start, dest, move_begin, move_finish, n_ships, action_code, action_data)"
                                    "VALUES (%i, 0, 0, %i, %i, %i, %i, %i, 28, 0)"
                                    ,cur_fleet->owner, cur_fleet->position, newposition_id, tick_start, tick_end, cur_fleet->n_ships )) {
                                        DEBUG_LOG("Could not insert new movement data for fleet %i\n", cur_fleet->fleet_id);
                                }
                                
                                new_move_id = (int)db.last_insert_id();
                                
                                if(!db.query((char*)"UPDATE ship_fleets SET planet_id = 0, move_id = %i WHERE fleet_id = %i", new_move_id, cur_fleet->fleet_id )) {
                                        DEBUG_LOG("Could not update fleet %i\n", cur_fleet->fleet_id);
                                }                                
                                continue;    
                        }
		}
	}
}


bool finish_combat(s_move_data* move, int winner, char** argv) {
    
        std::unordered_map<int, template_stat> tmp_map_winner;
        std::unordered_map<int, template_stat> tmp_map_loser;

        int tmp_id;
        template_stat tmp_item;
        int actual_tick;
        
        
#if VERBOSE >= 1
#else
	// Output format for PHP interface in lines:
	// 1: Status (first character 1 or 0, thereafter optionally error message)
	// 2: Winner (0 attacker, 1 Defender)
	// 3: Large orbital defense destroyed
	// 4: Small orbital defense destroyed
	// 5: Result line (geez, who has scrap this iron committed a crime)
	printf("1OK\n");

	if(winner == -1) printf("0\n");
	else printf("1\n");

	printf("%i\n", move->n_large_orbital_defense);
	printf("%i\n", move->n_small_orbital_defense);

	c_db_result* res;

        if(!db.query(&res, (char*)"SELECT tick_id FROM config WHERE config_set_id = 0")) 
        {
                DEBUG_LOG("Could not query actual tick data from db");
        }
        else
        {
                res->fetch_row();
                actual_tick = atoi(res->row[0]);
        }
        
	/**
	 ** 03/04/08 - AC: Introduce localization of the message strings
	 **/
	int lang = LANG_ENG;
	const char *sAttackingShips,*sHull,*sHullHeader,*sDefendingShips,*sOrbital,*sLightOrbital,
                   *sAttackerWon,*sDefenderWon,*sAttackDestroy1,*sAttackDestroy2,*sDefendFled,*sLegend;
		 // *sAttackerWon,*sDefenderWon,*sAttackDestroy1,*sAttackDestroy2,*sDefendFled,*sOrbDestroyed,*sLOrbDestroyed;
	if(!db.query(&res, (char*)"SELECT language FROM user WHERE user_id = %i", move->user_id))
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
			sAttackingShips = "<center><span class=\"sub_caption\">Attacking ships:</span></center><br>";
                        sHullHeader     = "<th style=\"text-align: left;\">Name</th><th style=\"text-align: left;\">Type</th><th style=\"text-align: left;\">#</th>"
                                          "<th style=\"text-align: left;\">db</th>"
                                          "<th style=\"text-align: left;\">KO</th><th style=\"text-align: left;\">Dmg</th><th style=\"text-align: left;\">H.Dmg</th>"
                                          "<th style=\"text-align: left;\">V.H.Dmg</th><th style=\"text-align: left;\">Esc.</th>";
                        sHull           = "<tr><td>%s</td><td>(Hull: %i)</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td></tr>";                        
			sDefendingShips = "<br><center><span class=\"sub_caption\">Defending ships:</span></center><br>";
			// sOrbital        = "Orbital cannon (stationary): <b>%i</b><br>";
                        sOrbital        = "<tr><td>Orbital cannon</td><td>(stationary)</td><td>%i</td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";                        
			// sLightOrbital   = "L. Orbital cannon (stationary): <b>%i</b><br>";
                        sLightOrbital   = "<tr><td>L. Orbital cannon</td><td>(stationary)</td><td>%i</td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";                        
			sAttackerWon    = "<br>The attacking ships have won the fight.<br><br>";
			sDefenderWon    = "<br>The defending ships have won the fight.<br><br>";
			sAttackDestroy1 = "From the victorious ships became...<br>... <b>%i</b> destroyed<br>... <b>%i</b> damaged<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;of which <b>%i</b> seriously<br>&nbsp;&nbsp;&nbsp;&nbsp;and <b>%i</b> very seriously<br>";
			sDefendFled     = "There are <b>%i</b> defeated ships escaped from the batlle.<br>";
                        sLegend         = "# = starting quantity<br>db = deathblows dealt<br>ko = destroyed<br>"
                                          "d. = damaged; h. = seriously damaged; v.h. = very seriously damaged<br>"
                                          "esc. = escaped the fight";                        
			// sOrbDestroyed   = "There were <b>%i</b> Orbital Cannon destroyed.<br>";
			// sLOrbDestroyed  = "There were <b>%i</b> Light Orbital Cannon destroyed.<br>";
		break;
		// German
		case LANG_GER:
			sAttackingShips = "<center><span class=\"sub_caption\">Angreifende Schiffe:</span></center><br>";
                        sHullHeader     = "<th style=\"text-align: left;\">Name</th><th style=\"text-align: left;\">Type</th><th style=\"text-align: left;\"> # </th>"
                                          "<th style=\"text-align: left;\">db</th>"
                                          "<th style=\"text-align: left;\">ko</th><th style=\"text-align: left;\">d.</th><th style=\"text-align: left;\">h.</th>"
                                          "<th style=\"text-align: left;\">v.h.</th><th style=\"text-align: left;\">esc.</th>";   
                        sHull           = "<tr><td>%s</td><td>(Rumpf: %i)</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td></tr>"; 
			sDefendingShips = "<br><center><span class=\"sub_caption\">Verteidigende Schiffe:</span></center><br>";
			// sOrbital        = "Orbitalgesch&uuml;tz (station&auml;r): <b>%i</b><br>";
                        sOrbital        = "<tr><td>Orbitalgesch&uuml;tz</td><td>(station&auml;r)</td><td>%i</td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";                        
			// sLightOrbital   = "L. Orbitalgesch&uuml;tz (station&auml;r): <b>%i</b><br>";
                        sLightOrbital   = "<tr><td>L. Orbitalgesch&uuml;tz</td><td>(station&auml;r)</td><td>%i</td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";                        
			sAttackerWon    = "<br>Die angreifenden Schiffe haben den Kampf gewonnen.<br><br>";
			sDefenderWon    = "<br>Die verteidigenden Schiffe haben den Kampf gewonnen.<br><br>";
			sAttackDestroy1 = "Von den siegreichen Schiffen wurden...<br>... <b>%i</b> zerst&ouml;rt<br>... <b>%i</b> besch&auml;digt<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;davon <b>%i</b> stark<br>&nbsp;&nbsp;&nbsp;&nbsp;and <b>%i</b> sehr stark<br>";
			sDefendFled     = "There are <b>%i</b> defeated ships escaped from the batlle.<br>";
                        sLegend         = "# = starting quantity<br>db = deathblows dealt<br>ko = destroyed<br>"
                                          "d. = damaged; h. = seriously damaged; v.h. = very seriously damaged<br>"
                                          "esc. = escaped the fight";                        
			// sOrbDestroyed   = "Es wurden <b>%i</b> Orbitalgesch&uuml;tze zerst&ouml;rt.<br>";
			// sLOrbDestroyed  = "Es wurden <b>%i</b> kleine Orbitalgesch&uuml;tze zerst&ouml;rt.<br>";
		break;
		// Italian
		case LANG_ITA:
			sAttackingShips = "<center><span class=\"sub_caption\">Navi in attacco:</span></center><br>";
                        sHullHeader     = "<th style=\"text-align: left;\">Nome</th><th style=\"text-align: left;\">Tipo</th><th style=\"text-align: left;\"> # </th>"
                                          "<th style=\"text-align: left;\">db</th>"
                                          "<th style=\"text-align: left;\">ko</th><th style=\"text-align: left;\">d.</th><th style=\"text-align: left;\">h.</th>"
                                          "<th style=\"text-align: left;\">v.h.</th><th style=\"text-align: left;\">esc.</th>";
			sHull           = "<tr><td>%s</td><td>(Scafo: %i)</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td><td>%i</td></tr>";
			sDefendingShips = "<br><center><span class=\"sub_caption\">Navi in difesa:</span></center><br>";
			// sOrbital        = "Cannoni Orbitali (stazionari): <b>%i</b><br>";
                        sOrbital        = "<tr><td>Cannone Orbitale</td><td>(Stazionario)</td><td>%i</td><td> -- </td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";
			// sLightOrbital   = "Cannoni Orbitali L. (stazionari): <b>%i</b><br>";
                        sLightOrbital   = "<tr><td>Cannone Orbitale L.</td><td>(Stazionario)</td><td>%i</td><td> -- </td><td>%i</td><td> -- </td><td> -- </td><td> -- </td><td> -- </td></tr>";
			sAttackerWon    = "<br>Le navi in attacco hanno vinto la battaglia.<br><br>";
			sDefenderWon    = "<br>Le navi in difesa hanno vinto la battaglia.<br><br>";
			sAttackDestroy1 = "Delle navi vittoriose ci sono state...<br>... <b>%i</b> distrutte<br>... <b>%i</b> danneggiate<br>";
			sAttackDestroy2 = "&nbsp;&nbsp;&nbsp;&nbsp;di cui <b>%i</b> seriamente<br>&nbsp;&nbsp;&nbsp;&nbsp;e <b>%i</b> molto seriamente<br>";
			sDefendFled     = "Delle navi sconfitte <b>%i</b> sono fuggite dal combattimento.<br>";
                        sLegend         = "# = numero iniziale per tipo<br>db = numero di colpi finali inferto<br>ko = distrutte<br>"
                                          "d. = danneggiate; h. = danneggiate gravemente; v.h. = danneggiate molto gravemente<br>"
                                          "esc. = fuggite dal combattimento";
			// sOrbDestroyed   = "Sono stati distrutti <b>%i</b> Cannoni Orbitali.<br>";
			// sLOrbDestroyed  = "Sono stati distrutti <b>%i</b> Cannoni Orbitali Leggeri.<br>";
		break;
	}
	/* */

	s_ship* cur_winner_ship;
	s_ship* cur_loser_ship;
	int n_winner_ships;
	int n_loser_ships;

	if(winner == -1) {
		printf(sAttackerWon);

		cur_winner_ship = move->atk_ships;
		n_winner_ships = move->n_atk_ships;
		cur_loser_ship = move->dfd_ships;
		n_loser_ships = move->n_dfd_ships;
	}
	else {
		printf(sDefenderWon);

		cur_winner_ship = move->dfd_ships;
		n_winner_ships = move->n_dfd_ships;
		cur_loser_ship = move->atk_ships;
		n_loser_ships = move->n_atk_ships;
	}

	int n_winner_destroyed = 0, n_winner_light_damage = 0, n_winner_heavy_damage = 0, n_winner_very_heavy_damage = 0;
	float hp_status = 0.0;

	for(int i = 0; i < n_winner_ships; ++i) {
                tmp_id = cur_winner_ship[i].ship_template_id;
                tmp_item = tmp_map_winner[tmp_id];
                tmp_item.deathblows = tmp_item.deathblows + cur_winner_ship[i].deathblows;
		if(cur_winner_ship[i].hitpoints <= 0) {
			++n_winner_destroyed;
                        ++tmp_item.out;
		}
		else {
			hp_status = ((cur_winner_ship[i].previous_hitpoints - cur_winner_ship[i].hitpoints) / cur_winner_ship[i].tpl.value_5);

			if(hp_status == 0.0) { }
			else if(hp_status <= 0.25) {
                            ++n_winner_light_damage;
                            ++tmp_item.damaged;
                        }
			else if(hp_status <= 0.5) {
                            ++n_winner_heavy_damage;
                            ++tmp_item.h_damaged;
                        }
			else {
                            ++n_winner_very_heavy_damage;
                            ++tmp_item.v_h_damaged;
                        }
		}
                tmp_map_winner[tmp_id] = tmp_item;
	}

	// int n_fled_ship = 0;

	for(int i = 0; i < n_loser_ships; i++){
            tmp_id = cur_loser_ship[i].ship_template_id;
            tmp_item = tmp_map_loser[tmp_id];
            tmp_item.deathblows = tmp_item.deathblows + cur_loser_ship[i].deathblows;
            if(cur_loser_ship[i].fleed) ++tmp_item.escaped; else ++tmp_item.out;
            tmp_map_loser[tmp_id] = tmp_item;
	}

        if(!db.query(&res, (char*)"SELECT st.name, st.ship_torso, COUNT(s.ship_id) AS n_ships, st.id \
						FROM (ship_templates st) \
						INNER JOIN ships s ON s.template_id = st.id \
						WHERE s.fleet_id IN (%s) \
						GROUP BY st.id", argv[1])) {

		DEBUG_LOG("Could not query attackers template overview\n");
	}
	else {
		printf("%s",sAttackingShips);
                printf("%s%s", "<table width=450 cellpadding=0 cellspacing=0 border=0>", sHullHeader);

		while(res->fetch_row()) {
                    tmp_id = atoi(res->row[3]);
                    if(winner == -1) tmp_item = tmp_map_winner[tmp_id]; else tmp_item = tmp_map_loser[tmp_id]; 
                    printf(sHull, res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]), tmp_item.deathblows, tmp_item.out, tmp_item.damaged, tmp_item.h_damaged, tmp_item.v_h_damaged, tmp_item.escaped);
		}
                
                printf("%s", "</table><br>");
	}

	safe_delete(res);

	if(!db.query(&res, (char*)"SELECT st.name, st.ship_torso, COUNT(s.ship_id) AS n_ships, st.id \
						FROM (ship_templates st) \
						INNER JOIN ships s ON s.template_id = st.id \
						WHERE s.fleet_id IN (%s) \
						GROUP BY st.id", argv[2])) {

		DEBUG_LOG("Could not query attackers template overview\n");
	}
	else {
		printf("%s",sDefendingShips);

		// if(move->n_large_orbital_defense > 0) printf(sOrbital, move->n_large_orbital_defense);
		// if(move->n_small_orbital_defense > 0) printf(sLightOrbital, move->n_small_orbital_defense);
		
                printf("%s%s", "<table width=450 cellpadding=0 cellspacing=0 border=0>", sHullHeader);
                
		while(res->fetch_row()) {
                    tmp_id = atoi(res->row[3]);
                    if(winner == -1) tmp_item = tmp_map_loser[tmp_id]; else tmp_item = tmp_map_winner[tmp_id];
                    printf(sHull, res->row[0], (atoi(res->row[1]) + 1), atoi(res->row[2]), tmp_item.deathblows, tmp_item.out, tmp_item.damaged, tmp_item.h_damaged, tmp_item.v_h_damaged, tmp_item.escaped);
		}
                
                if(move->n_large_orbital_defense > 0) {
                    printf(sOrbital, move->n_large_orbital_defense, move->destroyed_large_orbital_defense);
                }
                
                if(move->n_small_orbital_defense > 0) {
                    printf(sLightOrbital, move->n_small_orbital_defense, move->destroyed_small_orbital_defense);
                }                
                
                printf("%s", "</table><br><br>");
	}
        
        printf("%s",sLegend);
	// printf(sAttackDestroy1, n_winner_destroyed, (n_winner_light_damage + n_winner_heavy_damage + n_winner_very_heavy_damage));
	// printf(sAttackDestroy2, n_winner_heavy_damage, n_winner_very_heavy_damage);

	// if(n_fled_ship>0) printf(sDefendFled, n_fled_ship);

	// if(move->destroyed_large_orbital_defense) printf(sOrbDestroyed, move->destroyed_large_orbital_defense);
	// if(move->destroyed_small_orbital_defense) printf(sLOrbDestroyed, move->destroyed_small_orbital_defense);

	printf("<br>\n");

#endif


	if( (move->destroyed_large_orbital_defense > 0) || (move->destroyed_small_orbital_defense > 0) ) {
#ifndef SIMULATOR
		if(!db.query((char*)"UPDATE planets "
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

        if(winner == -1) {
                update_fleets(move->atk_fleets, move->n_atk_fleets, true, actual_tick);
                update_fleets(move->dfd_fleets, move->n_dfd_fleets, false, actual_tick);
        }
        else {
                update_fleets(move->atk_fleets, move->n_atk_fleets, false, actual_tick);
                update_fleets(move->dfd_fleets, move->n_dfd_fleets, true, actual_tick);
        }


	for(user_map_it = user_xp_map.begin(); user_map_it != user_xp_map.end(); ++user_map_it) {
#ifndef SIMULATOR
		if(!db.query((char*)"UPDATE user SET user_honor = user_honor + %i WHERE user_id = %i", ((int)user_map_it->second / 10), user_map_it->first)) {
			DEBUG_LOG("Could not update user honor data\n");
		}
#endif
	}

	return true;
}
