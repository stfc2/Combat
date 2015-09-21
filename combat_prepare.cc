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

#include "combat.hh"
#include "db_core.hh"


extern c_db_core db;

static map<int, s_ship_template*>::iterator tpl_map_it;
static map<int, s_ship_template*> ship_templates_map;

static map<int, s_fleet*>::iterator fleet_map_it;
static map<int, s_fleet*> fleets_map;

static char set_atk_lvl(char race) {
    
    char result;
    
    // Indicati i valori per tutte le razze giocanti, anche se potevo indicare solo
    // le razze che richiedono il settaggio del campo a 1.
    switch(race) {
        case 0:         // Fed
            result = 1;
            break;
        case 1:         // Romulan
            result = 0;
            break;
        case 2:         // Klingon
            result = 1;
            break;
        case 3:         // Cardassian
            result = 1;
            break;
        case 4:         // Dominion
            result = 0;
            break;
        case 6:         // Borg
            result = 1;
            break;
        case 9:         // Hirogeni
            result = 1;
            break;
        case 11:        // Kazon
            result = 0;
            break;
        default:
            result = 0;
    }
    
    return result;
}

static void prepare_fleets(c_db_result* res, s_fleet* cur_fleet) {
	while(res->fetch_row()) {
		cur_fleet->fleet_id = atoi(res->row[0]);

		cur_fleet->n_ships = cur_fleet->n_transporter = 0;

		cur_fleet->resource_1 = atoi(res->row[2]);
		cur_fleet->resource_2 = atoi(res->row[3]);
		cur_fleet->resource_3 = atoi(res->row[4]);
		cur_fleet->resource_4 = atoi(res->row[5]);
		cur_fleet->unit_1 = atoi(res->row[6]);
		cur_fleet->unit_2 = atoi(res->row[7]);
		cur_fleet->unit_3 = atoi(res->row[8]);
		cur_fleet->unit_4 = atoi(res->row[9]);
		cur_fleet->unit_5 = atoi(res->row[10]);
		cur_fleet->unit_6 = atoi(res->row[11]);
                cur_fleet->position = atoi(res->row[12]);
                cur_fleet->homebase = atoi(res->row[13]);
                cur_fleet->owner = atoi(res->row[14]);
                
		fleets_map[cur_fleet->fleet_id] = cur_fleet;

		++cur_fleet;
	}
}

static bool prepare_ships(c_db_result* res, s_ship* cur_ship) {
	while(res->fetch_row()) {
		cur_ship->ship_id = atoi(res->row[0]);

		if(map_key_exists(fleets_map, atoi(res->row[1]), fleet_map_it)) {
			cur_ship->fleet = fleet_map_it->second;
		}
		else {
			printf("0Could not find fleet for ship %i in map\n", cur_ship->ship_id);

			return false;
		}

		cur_ship->user_id = atoi(res->row[2]);

		cur_ship->experience = atoi(res->row[4]);
		cur_ship->xp_gained = 0;

		cur_ship->hitpoints = atof(res->row[5]);
		cur_ship->previous_hitpoints = atof(res->row[5]);

		cur_ship->unit_1 = atoi(res->row[6]);
		cur_ship->unit_2 = atoi(res->row[7]);
		cur_ship->unit_3 = atoi(res->row[8]);
		cur_ship->unit_4 = atoi(res->row[9]);
		cur_ship->torp = atoi(res->row[10]);
		cur_ship->rof = atoi(res->row[11]);
                cur_ship->rof2 = atoi (res->row[12]);
                cur_ship->atk_lvl = 0;
                cur_ship->ship_template_id = atoi(res->row[3]);

                cur_ship->deathblows = 0;
		cur_ship->captured = false;
		cur_ship->fleed = false;
		cur_ship->knockout = false;
		cur_ship->surrendered = false;
		cur_ship->changed = false;

		if(map_key_exists(ship_templates_map, atoi(res->row[3]), tpl_map_it)) {
			memcpy(&cur_ship->tpl, tpl_map_it->second, sizeof(s_ship_template));
		}
		else {
			// bad, extra query
		}

		++cur_ship->fleet->n_ships;

		if(cur_ship->tpl.ship_torso == SHIP_TORSO_TRANSPORTER) {
			++cur_ship->fleet->n_transporter;
		}

		++cur_ship;
	}
	return true;
}

bool prepare_combat(s_move_data* move, char** argv) {
	char* atk_fleet_ids_str = argv[1];
	char* dfd_fleet_ids_str = argv[2];

	c_db_result* res;

	s_ship_template* new_tpl;
	int cur_template_id;
#if VERBOSE >= 4
	s_fleet* cur_fleet;
#endif
	s_ship* cur_ship;


	// templates
	if(!db.query(&res, (char*)"SELECT s.template_id, "
			   "       st.value_1, st.value_2, st.value_3, st.value_4, st.value_5, st.value_6, st.value_7, st.value_8, st.value_10, st.value_11, st.value_12, st.ship_torso, st.race, st.ship_class, "
                           "       st.unit_5, st.unit_6 "
			   "FROM (ships s) "
			   "INNER JOIN ship_templates st ON st.id = s.template_id "
			   "WHERE s.fleet_id IN (%s, %s) "
			   "GROUP BY s.template_id", atk_fleet_ids_str, dfd_fleet_ids_str)) {

		printf("0Could not query ship templates data\n");

		return false;
	}

	if(res->num_rows() == 0) {
		printf("0Could not find ship templates data\n");

		return false;
	}

#if VERBOSE >= 4
	DEBUG_LOG("\n- loaded ship templates:\n");
#endif

	while(res->fetch_row()) {
		cur_template_id = atoi(res->row[0]);

		new_tpl = new s_ship_template;

		new_tpl->value_1 = atof(res->row[1]);
		new_tpl->value_2 = atof(res->row[2]);
		new_tpl->value_3 = atof(res->row[3]);
		new_tpl->value_4 = atoi(res->row[4]);
		new_tpl->value_5 = atoi(res->row[5]);
		new_tpl->value_6 = atof(res->row[6]);
		new_tpl->value_7 = atof(res->row[7]);
		new_tpl->value_8 = atof(res->row[8]);
		new_tpl->value_10 = atof(res->row[9]);
		new_tpl->value_11 = atoi(res->row[10]);
		new_tpl->value_12 = atoi(res->row[11]);
		new_tpl->ship_torso = atoi(res->row[12]);
		new_tpl->race = atoi(res->row[13]);
		new_tpl->ship_class = atoi(res->row[14]);
                new_tpl->unit_5 = atoi(res->row[15]);
                new_tpl->unit_6 = atoi(res->row[16]);                

#if VERBOSE >= 4
		DEBUG_LOG("%i (type:  %i/%i values: %.0f/%.0f/%.0f/%i/%i/%.0f/%.0f/%.0f/%.1f/%i/%i)\n", cur_template_id, new_tpl->ship_torso, new_tpl->ship_class,
			new_tpl->value_1, new_tpl->value_2, new_tpl->value_3, new_tpl->value_4, new_tpl->value_5, new_tpl->value_6, new_tpl->value_7,
			new_tpl->value_8, new_tpl->value_10, new_tpl->value_11, new_tpl->value_12);
#endif

		ship_templates_map[cur_template_id] = new_tpl;

		++move->n_ship_templates;
	}

	safe_delete(res);


	// attacker fleets
	if(!db.query(&res, (char*)"SELECT fleet_id, n_ships, resource_1, resource_2, resource_3, resource_4, unit_1, unit_2, unit_3, unit_4, unit_5, unit_6, planet_id, homebase, user_id "
			   "FROM ship_fleets WHERE fleet_id IN (%s)", atk_fleet_ids_str)) {
		printf("0Could not query attacker's fleet data\n");

		return false;
	}

	move->n_atk_fleets = res->num_rows();

	if(move->n_atk_fleets == 0) {
		printf("0No attacker fleets\n");

		return false;
	}

	move->atk_fleets = new s_fleet[move->n_atk_fleets];

	prepare_fleets(res, move->atk_fleets);

	safe_delete(res);


	// attacker ships
	if(!db.query(&res, (char*)"SELECT ship_id, fleet_id, user_id, template_id, experience, hitpoints, unit_1, unit_2, unit_3, unit_4, torp, rof, rof2 FROM ships WHERE fleet_id IN (%s)", atk_fleet_ids_str)) {
		printf("0Could not query attacker's ship data\n");

		return false;
	}

	move->n_atk_ships = res->num_rows();

	if(move->n_atk_ships == 0) {
		printf("0No attacker ships\n");

		return false;
	}

	move->atk_ships = new s_ship[move->n_atk_ships];

	prepare_ships(res, move->atk_ships);

	// 03/04/08 - AC: Store attacker userID for message localization
	move->user_id = move->atk_ships->user_id;

	safe_delete(res);


	// defender fleets
	fleets_map.clear();

	if(!db.query(&res, (char*)"SELECT fleet_id, n_ships, resource_1, resource_2, resource_3, resource_4, unit_1, unit_2, unit_3, unit_4, unit_5, unit_6, planet_id, homebase, user_id "
			   "FROM ship_fleets WHERE fleet_id IN (%s)", dfd_fleet_ids_str)) {
		printf("0Could not query defender's fleet data\n");

		return false;
	}

	move->n_dfd_fleets = res->num_rows();

	if(move->n_dfd_fleets != 0) {
		move->dfd_fleets = new s_fleet[move->n_dfd_fleets];
		prepare_fleets(res, move->dfd_fleets);
	}

	safe_delete(res);


	// defender ships
	if(!db.query(&res, (char*)"SELECT ship_id, fleet_id, user_id, template_id, experience, hitpoints, unit_1, unit_2, unit_3, unit_4, torp, rof, rof2 FROM ships WHERE fleet_id IN (%s)", dfd_fleet_ids_str)) {
		printf("0Could not query defender's ship data\n");

		return false;
	}

	move->n_dfd_ships = res->num_rows();

	if(move->n_dfd_ships != 0) {
		move->dfd_ships = new s_ship[move->n_dfd_ships];
		// 22/12/08 - AC: Check error reporting!
		if(!prepare_ships(res, move->dfd_ships))
		{
			safe_delete_array(move->dfd_ships);
			move->n_dfd_ships = 0;
		}
	}

	safe_delete(res);
	safe_delete_map_values(ship_templates_map);

	move->dest = atoi(argv[3]);
	move->n_large_orbital_defense = atoi(argv[4]);
	move->n_small_orbital_defense = atoi(argv[5]);

	int ships_by_class[MAX_SHIP_CLASS + 1];

	float bonus_attacker = 0.0, bonus_defender = 0.0;
	float diff;

	memset(ships_by_class, 0, (MAX_SHIP_CLASS + 1) * sizeof(int));

	// Calculate attackers bonus:
	if(move->n_atk_ships < 20000) {
		cur_ship = move->atk_ships;

		for(int i = 0; i < move->n_atk_ships; ++i) {
			++ships_by_class[(cur_ship++)->tpl.ship_class];
		}

		diff = fabs((float)(ships_by_class[0] - move->n_atk_ships * OPTIMAL_0)) +
			   fabs((float)(ships_by_class[1] - move->n_atk_ships * OPTIMAL_1)) +
			   fabs((float)(ships_by_class[2] - move->n_atk_ships * OPTIMAL_2)) +
			   fabs((float)(ships_by_class[3] - move->n_atk_ships * OPTIMAL_3));

		bonus_attacker = 0.25 - (0.5 / move->n_atk_ships * diff);

		if(bonus_attacker < 0) bonus_attacker = 0;
	}

	memset(ships_by_class, 0, (MAX_SHIP_CLASS + 1) * sizeof(int));


	// Calculate defender bonus:
	if(move->n_dfd_ships>0 && move->n_dfd_ships < 20000) {
		cur_ship = move->dfd_ships;

		for(int i = 0; i < move->n_dfd_ships; ++i) {
			++ships_by_class[(cur_ship++)->tpl.ship_class];
		}

		diff = fabs((float)(ships_by_class[0] - move->n_dfd_ships * OPTIMAL_0)) +
			   fabs((float)(ships_by_class[1] - move->n_dfd_ships * OPTIMAL_1)) +
			   fabs((float)(ships_by_class[2] - move->n_dfd_ships * OPTIMAL_2)) +
			   fabs((float)(ships_by_class[3] - move->n_dfd_ships * OPTIMAL_3));

		bonus_defender = 0.25 - (0.5 / move->n_dfd_ships * diff);

		if(bonus_defender < 0) bonus_defender = 0;
	}

	++bonus_attacker;
	++bonus_defender;

	// Calculate Experience + First strike:

	// Sensors*0.5 + Reaction*2 + Readiness*3 + Agility + Camouflage d. eig. ship

	// + rand(0,5) bei att, rand(0,7) bei def

	float rank_bonus, firststrike;

	cur_ship = move->atk_ships;

	for(int i = 0; i < move->n_atk_ships; ++i) {
            
                cur_ship->dmg_ctrl = 1*floor(cur_ship->tpl.unit_5 / 3); // 1 pnt.percent every three techs onboard, rounded down
                
                cur_ship->atk_lvl = set_atk_lvl(cur_ship->tpl.race);
                
                firststrike = 0;

                // race bonus
                switch(cur_ship->tpl.race) {
                    case 0:
                        // Fed bonus
                        // Damage mitigation bonus
                        // Shield bonus (value_4)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }                        
                        break;
                    case 1:
                        // Romulan bonus
                        // First-strike bonus
                        // Cloaking_bonus (value_12)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 40;
                            cur_ship->tpl.value_12 = cur_ship->tpl.value_12 + 10;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 40;
                            break;
                        }                        
                        break;
                    case 2:
                        // Klingon bonus
                        // First-strike bonus
                        // Secondary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 40;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 40;
                            break;
                        }                        
                        break;
                    case 3:
                        // Cardassian bonus
                        // Damage mitigation bonus
                        // Primary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }                        
                        break;
                    case 4:
                        // Dominion bonus
                        // Primary Weapon bonus
                        // Hull bonus (value_5)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.15;
                            cur_ship->hitpoints += (cur_ship->tpl.value_5 * 0.15);                            
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.15;
                            break;
                        }
                        break;
                    case 6:
                        // Borg bonus
                        // Shield bonus (value_4)
                        // Hull bonus (value_5)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.34;
                            cur_ship->hitpoints += (cur_ship->tpl.value_5 * 0.34);
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.34;
                            break;
                        }                        
                        break;
                    case 9:
                        // Hirogen bonus
                        // First-Strike bonus
                        // Rof/Rof2 bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 50;
                            cur_ship->rof++;
                            cur_ship->rof2++;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 50;
                            break;
                        }
                        break;
                    case 11:
                        // Kazon bonus
                        // First-Strike bonus
                        // Primary/Secondary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 25;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.1;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 25;
                            break;
                        }
                        break;
                    case 13:
                        // Settlers bonus
                        // Damage mitigations bonus
                        // Hull bonus (value_5)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            cur_ship->hitpoints += (cur_ship->tpl.value_5 * 0.10);
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }                        
                        break;
                    default:
                        break;
                }
                
                // class bonus
                switch(cur_ship->tpl.ship_class) {
                    case 3:
                        // Damage mitigation bonus
                        // Primary Weapon bonus dmg
                        // Secondary Weapon bonus dmg
                        if(cur_ship->experience >= SHIP_RANK_7_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.1;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            break;
                        }                        
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            break;
                        }
                        break;
                    case 2:
                        // Damage mitigation bonus
                        // First-Strike bonus
                        // Shields bonus
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            firststrike = firststrike + 30;
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_4_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            firststrike = firststrike + 30;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            break;
                        }
                        break;
                    case 1:
                        // Damage mitigation bonus
                        // Agility bonus (value_8)
                        // Sensor bonus (value_11)
                        // Reaction bonus (value_6)
                        if(cur_ship->experience >= SHIP_RANK_7_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            cur_ship->tpl.value_11 = cur_ship->tpl.value_11 + 15;
                            cur_ship->tpl.value_6 = cur_ship->tpl.value_6 + 15;
                            break;
                        }                        
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            cur_ship->tpl.value_11 = cur_ship->tpl.value_11 + 15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_4_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            break;
                        }                        
                        break;
                    default:
                        break;
                }
                
		if(cur_ship->experience < SHIP_RANK_0_LIMIT) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_8_LIMIT) rank_bonus = SHIP_RANK_8_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_6_LIMIT) rank_bonus = SHIP_RANK_6_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_3_LIMIT) rank_bonus = SHIP_RANK_3_BONUS;
		else rank_bonus = SHIP_RANK_0_BONUS;

		// Recalculate ship's statistics (base stat + modifier) + ship's experience bonus

		cur_ship->tpl.value_1 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_2 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_3 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_6 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_7 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_8 *= (rank_bonus + bonus_attacker);
                
                // rof1 bonus
                if(cur_ship->experience < SHIP_RANK_TIER_1) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_TIER_4) cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_4_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_TIER_3) cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_3_BONUS;
		else cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_1_BONUS;                
                
                // rof2 bonus
                if(cur_ship->experience < SHIP_RANK_TIER_2) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_TIER_5) cur_ship->rof2 = cur_ship->rof2 + SHIP_RANK_TIER_5_BONUS;
		else cur_ship->rof2 = cur_ship->rof2 + SHIP_RANK_TIER_2_BONUS;    
                
		firststrike = firststrike + (cur_ship->tpl.value_11 * 0.5 + cur_ship->tpl.value_6 * 2 + cur_ship->tpl.value_7 * 3 + cur_ship->tpl.value_8 + cur_ship->tpl.value_12);

		if(firststrike < 1) firststrike = 1;

		firststrike *= (1 + (cur_ship->experience / 5000));

		cur_ship->firststrike = firststrike;
		cur_ship->shields = cur_ship->tpl.value_4;

		if(cur_ship->experience < 10) cur_ship->experience = 10;

		++cur_ship;
	}

	cur_ship = move->dfd_ships;

	for(int i = 0; i < move->n_dfd_ships; ++i) {

                cur_ship->dmg_ctrl = 1*floor(cur_ship->tpl.unit_5 / 3); // 1 pnt.percent every three techs onboard, rounded down
                
                cur_ship->atk_lvl = set_atk_lvl(cur_ship->tpl.race);
                
                firststrike = 0;

                // race bonus
                switch(cur_ship->tpl.race) {
                    case 0:
                        // Fed bonus
                        // Damage mitigation bonus
                        // Shield bonus (value_4)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }                        
                        break;
                    case 1:
                        // Romulan bonus
                        // First-strike bonus
                        // Cloaking_bonus (value_12)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 40;
                            cur_ship->tpl.value_12 = cur_ship->tpl.value_12 + 10;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 40;
                            break;
                        }                        
                        break;
                    case 2:
                        // Klingon bonus
                        // First-strike bonus
                        // Secondary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 40;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 40;
                            break;
                        }                        
                        break;
                    case 3:
                        // Cardassian bonus
                        // Damage mitigation bonus
                        // Primary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }                        
                        break;
                    case 4:
                        // Dominion bonus
                        // Primary Weapon bonus
                        // Hull bonus (value_5)
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.15;
                            cur_ship->tpl.value_5 = cur_ship->tpl.value_5 * 1.15;                            
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 15;
                            break;
                        }
                        break;
                    case 9:
                        // Hirogen bonus
                        // First-Strike bonus
                        // Rof/Rof2 bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 50;
                            cur_ship->rof++;
                            cur_ship->rof2++;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 50;
                            break;
                        }
                        break;
                    case 11:
                        // Kazon bonus
                        // First-Strike bonus
                        // Primary/Secondary Weapon bonus
                        if(cur_ship->experience >= SHIP_RANK_9_LIMIT) {
                            firststrike = firststrike + 25;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.1;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_2_LIMIT) {
                            firststrike = firststrike + 25;
                            break;
                        }
                        break;                        
                    default:
                        break;
                }                
                
                // class bonus
                switch(cur_ship->tpl.ship_class) {
                    case 3:
                        // Damage mitigation bonus
                        // Primary Weapon bonus dmg
                        // Secondary Weapon bonus dmg
                        if(cur_ship->experience >= SHIP_RANK_7_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            cur_ship->tpl.value_2 = cur_ship->tpl.value_2 * 1.1;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            cur_ship->tpl.value_1 = cur_ship->tpl.value_1 * 1.1;
                            break;
                        }                        
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 20;
                            break;
                        }
                        break;
                    case 2:
                        // Damage mitigation bonus
                        // First-Strike bonus
                        // Shields bonus (value_4)
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            firststrike = firststrike + 30;
                            cur_ship->tpl.value_4 = cur_ship->tpl.value_4 * 1.15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_4_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            firststrike = firststrike + 30;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 10;
                            break;
                        }
                        break;
                    case 1:
                        // Damage mitigation bonus
                        // Agility bonus (value_8)
                        // Sensor bonus (value_11)
                        // Reaction bonus (value_6)
                        if(cur_ship->experience >= SHIP_RANK_7_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            cur_ship->tpl.value_11 = cur_ship->tpl.value_11 + 15;
                            cur_ship->tpl.value_6 = cur_ship->tpl.value_6 + 15;
                            break;
                        }                        
                        if(cur_ship->experience >= SHIP_RANK_5_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            cur_ship->tpl.value_11 = cur_ship->tpl.value_11 + 15;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_4_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            cur_ship->tpl.value_8 = cur_ship->tpl.value_8 + 10;
                            break;
                        }
                        if(cur_ship->experience >= SHIP_RANK_1_LIMIT) {
                            cur_ship->dmg_ctrl = cur_ship->dmg_ctrl + 5;
                            break;
                        }                        
                        break;
                    default:
                        break;
                }            
            
		if(cur_ship->experience < SHIP_RANK_0_LIMIT) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_8_LIMIT) rank_bonus = SHIP_RANK_8_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_6_LIMIT) rank_bonus = SHIP_RANK_6_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_3_LIMIT) rank_bonus = SHIP_RANK_3_BONUS;
		else rank_bonus = SHIP_RANK_0_BONUS;

		cur_ship->tpl.value_1 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_2 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_3 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_6 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_7 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_8 *= (rank_bonus + bonus_defender);
                
                // rof1 bonus
                if(cur_ship->experience < SHIP_RANK_TIER_1) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_TIER_4) cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_4_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_TIER_3) cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_3_BONUS;
		else cur_ship->rof = cur_ship->rof + SHIP_RANK_TIER_1_BONUS;                
                
                // rof2 bonus
                if(cur_ship->experience < SHIP_RANK_TIER_2) { } // do nothing
		else if(cur_ship->experience >= SHIP_RANK_TIER_5) cur_ship->rof2 = cur_ship->rof2 + SHIP_RANK_TIER_5_BONUS;
		else cur_ship->rof2 = cur_ship->rof2 + SHIP_RANK_TIER_2_BONUS;                
                
		firststrike = firststrike + (cur_ship->tpl.value_11 * 0.5 + cur_ship->tpl.value_6 * 2 + cur_ship->tpl.value_7 * 3 + cur_ship->tpl.value_8 + cur_ship->tpl.value_12);

		if(firststrike < 1) firststrike = 1;

		firststrike *= (1 + (cur_ship->experience / 5000));

		cur_ship->firststrike = firststrike;
		cur_ship->shields = cur_ship->tpl.value_4;

		if(cur_ship->experience < 10) cur_ship->experience = 10;

		++cur_ship;
	}

#if VERBOSE >= 1
	DEBUG_LOG("\nn_atk: %i/%i\nn_dfd: %i/%i\norbital: %i/%i\n\n", move->n_atk_fleets, move->n_atk_ships, move->n_dfd_fleets, move->n_dfd_ships, move->n_large_orbital_defense, move->n_small_orbital_defense);
#endif

#if VERBOSE >= 4
	DEBUG_LOG("\n- atk fleets:\n");

	cur_fleet = move->atk_fleets;

	for(int i = 0; i < move->n_atk_fleets; ++i) {
		DEBUG_LOG("%i (n_ships: %i)\n", cur_fleet->fleet_id, cur_fleet->n_ships);

		++cur_fleet;
	}

	DEBUG_LOG("\n- atk ships:\n");

	cur_ship = move->atk_ships;

	for(int i = 0; i < move->n_atk_ships; ++i) {
		DEBUG_LOG("%i (fleet: %i xp: %.0f hp: %.0f units: %i/%i/%i/%i type: %i/%i rof: %i torp: %i values: %.0f/%.0f/%.0f/%i/%i/%.0f/%.0f/%.0f/%.1f/%i/%i)\n",
				cur_ship->ship_id, cur_ship->fleet->fleet_id, cur_ship->experience, cur_ship->hitpoints,
				cur_ship->unit_1, cur_ship->unit_2, cur_ship->unit_3, cur_ship->unit_4,
				cur_ship->tpl.ship_torso, cur_ship->tpl.ship_class,
				cur_ship->rof, cur_ship->torp,
				cur_ship->tpl.value_1, cur_ship->tpl.value_2, cur_ship->tpl.value_3, cur_ship->tpl.value_4, cur_ship->tpl.value_5,
				cur_ship->tpl.value_6, cur_ship->tpl.value_7, cur_ship->tpl.value_8, cur_ship->tpl.value_10, cur_ship->tpl.value_11, cur_ship->tpl.value_12);

		++cur_ship;
	}

	DEBUG_LOG("\n\n- dfd fleets:\n");

	cur_fleet = move->dfd_fleets;

	for(int i = 0; i < move->n_dfd_fleets; ++i) {
		DEBUG_LOG("%i (n_ships: %i)\n",cur_fleet->fleet_id, cur_fleet->n_ships);

		++cur_fleet;
	}

	DEBUG_LOG("\n- dfd ships:\n");

	cur_ship = move->dfd_ships;

	for(int i = 0; i < move->n_dfd_ships; ++i) {
		DEBUG_LOG("%i (fleet: %i xp: %.0f hp: %.0f units: %i/%i/%i/%i type: %i/%i rof: %i torp: %i values: %.0f/%.0f/%.0f/%i/%i/%.0f/%.0f/%.0f/%.1f/%i/%i)\n",
				cur_ship->ship_id, cur_ship->fleet->fleet_id, cur_ship->experience, cur_ship->hitpoints,
				cur_ship->unit_1, cur_ship->unit_2, cur_ship->unit_3, cur_ship->unit_4,
				cur_ship->tpl.ship_torso, cur_ship->tpl.ship_class,
				cur_ship->rof, cur_ship->torp,
				cur_ship->tpl.value_1, cur_ship->tpl.value_2, cur_ship->tpl.value_3, cur_ship->tpl.value_4, cur_ship->tpl.value_5,
				cur_ship->tpl.value_6, cur_ship->tpl.value_7, cur_ship->tpl.value_8, cur_ship->tpl.value_10, cur_ship->tpl.value_11, cur_ship->tpl.value_12);

		++cur_ship;
	}

	DEBUG_LOG("\n");
#endif

	return true;
}
