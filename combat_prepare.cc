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

#include "combat.hh"
#include "db_core.hh"


extern c_db_core db;

static map<int, s_ship_template*>::iterator tpl_map_it;
static map<int, s_ship_template*> ship_templates_map;

static map<int, s_fleet*>::iterator fleet_map_it;
static map<int, s_fleet*> fleets_map;


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

		cur_ship->changed = false;

		if(map_key_exists(ship_templates_map, atoi(res->row[3]), tpl_map_it)) {
			memcpy(&cur_ship->tpl, tpl_map_it->second, sizeof(s_ship_template));
		}
		else {
			// schlecht, extra query
		}

		++cur_ship->fleet->n_ships;

		if(cur_ship->tpl.ship_torso == SHIP_TORSO_TRANSPORTER) {
			++cur_ship->fleet->n_transporter;
		}

		++cur_ship;
	}
}

bool prepare_combat(s_move_data* move, char** argv) {
	char* atk_fleet_ids_str = argv[1];
	char* dfd_fleet_ids_str = argv[2];
	char* orbital_defense = argv[3];

	c_db_result* res;

	s_ship_template* new_tpl;
	int cur_template_id;

	s_fleet* cur_fleet;
	s_ship* cur_ship;


	// templates
	if(!db.query(&res, "SELECT s.template_id, "
					   "	   st.value_1, st.value_2, st.value_3, st.value_4, st.value_5, st.value_6, st.value_7, st.value_8, st.value_10, st.value_11, st.value_12, st.ship_torso, st.race, st.ship_class "
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

#if VERBOSE >= 4
		DEBUG_LOG("%i (type:  %.0f/ %.0f values: %.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.1f/%.0f/%.0f)\n", cur_template_id, new_tpl->ship_torso, new_tpl->ship_class,
					new_tpl->value_1, new_tpl->value_2, new_tpl->value_3, new_tpl->value_4, new_tpl->value_5, new_tpl->value_6, new_tpl->value_7,
					new_tpl->value_8, new_tpl->value_10, new_tpl->value_11, new_tpl->value_12);
#endif

		ship_templates_map[cur_template_id] = new_tpl;

		++move->n_ship_templates;
	}

	safe_delete(res);


	// attacker fleets
	if(!db.query(&res, "SELECT fleet_id, n_ships, resource_1, resource_2, resource_3, resource_4, unit_1, unit_2, unit_3, unit_4, unit_5, unit_6 "
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
	if(!db.query(&res, "SELECT ship_id, fleet_id, user_id, template_id, experience, hitpoints, unit_1, unit_2, unit_3, unit_4 FROM ships WHERE fleet_id IN (%s)", atk_fleet_ids_str)) {
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

	if(!db.query(&res, "SELECT fleet_id, n_ships, resource_1, resource_2, resource_3, resource_4, unit_1, unit_2, unit_3, unit_4, unit_5, unit_6 "
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
	if(!db.query(&res, "SELECT ship_id, fleet_id, user_id, template_id, experience, hitpoints, unit_1, unit_2, unit_3, unit_4 FROM ships WHERE fleet_id IN (%s)", dfd_fleet_ids_str)) {
		printf("0Could not query defender's ship data\n");

		return false;
	}

	move->n_dfd_ships = res->num_rows();

	if(move->n_dfd_ships != 0) {
		move->dfd_ships = new s_ship[move->n_dfd_ships];
		prepare_ships(res, move->dfd_ships);
	}

	safe_delete(res);
	safe_delete_map_values(ship_templates_map);

	move->dest = atoi(argv[3]);	
	move->n_large_orbital_defense = atoi(argv[4]);
	move->n_small_orbital_defense = atoi(argv[5]);

	int ships_by_class[MAX_SHIP_CLASS + 1];

	float bonus_attacker = 0.0, bonus_defender = 0.0;
	float diff;

	// Angreifer Bonus berechnen:
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


	// Verteidiger Bonus berechnen:
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

	// Erfahrung + Erstschlag berechnen:

	// Sensoren*0.5 + Reaktion*2 + Bereitschaft*3 + Wendigkeit + Tarnung d. eig. Schiffs

	// + rand(0,5) bei att, rand(0,7) bei def	

	float rank_bonus, new_value, firststrike;

	cur_ship = move->atk_ships;

	for(int i = 0; i < move->n_atk_ships; ++i) {
		rank_bonus = SHIP_RANK_0_BONUS;

		if(cur_ship->experience < SHIP_RANK_1_LIMIT) { } // nichts tun
		else if(cur_ship->experience >= SHIP_RANK_9_LIMIT) rank_bonus = SHIP_RANK_9_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_8_LIMIT) rank_bonus = SHIP_RANK_8_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_7_LIMIT) rank_bonus = SHIP_RANK_7_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_6_LIMIT) rank_bonus = SHIP_RANK_6_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_5_LIMIT) rank_bonus = SHIP_RANK_5_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_4_LIMIT) rank_bonus = SHIP_RANK_4_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_3_LIMIT) rank_bonus = SHIP_RANK_3_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_2_LIMIT) rank_bonus = SHIP_RANK_2_BONUS;
		else rank_bonus = SHIP_RANK_1_BONUS;

		cur_ship->tpl.value_1 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_2 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_3 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_6 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_7 *= (rank_bonus + bonus_attacker);
		cur_ship->tpl.value_8 *= (rank_bonus + bonus_attacker);

		firststrike = cur_ship->tpl.value_11 * 0.5 + cur_ship->tpl.value_6 * 2 + cur_ship->tpl.value_7 * 3 + cur_ship->tpl.value_8 + cur_ship->tpl.value_12;

		if(firststrike < 1) firststrike = 1;

		firststrike *= (1 + (cur_ship->experience / 5000));

		cur_ship->firststrike = firststrike;
		cur_ship->shields = cur_ship->tpl.value_4;

		if(cur_ship->experience < 10) cur_ship->experience = 10;

		++cur_ship;
	}

	cur_ship = move->dfd_ships;

	for(int i = 0; i < move->n_dfd_ships; ++i) {
		rank_bonus = SHIP_RANK_0_BONUS;

		if(cur_ship->experience < SHIP_RANK_1_LIMIT) { } // nichts tun
		else if(cur_ship->experience >= SHIP_RANK_9_LIMIT) rank_bonus = SHIP_RANK_9_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_8_LIMIT) rank_bonus = SHIP_RANK_8_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_7_LIMIT) rank_bonus = SHIP_RANK_7_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_6_LIMIT) rank_bonus = SHIP_RANK_6_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_5_LIMIT) rank_bonus = SHIP_RANK_5_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_4_LIMIT) rank_bonus = SHIP_RANK_4_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_3_LIMIT) rank_bonus = SHIP_RANK_3_BONUS;
		else if(cur_ship->experience >= SHIP_RANK_2_LIMIT) rank_bonus = SHIP_RANK_2_BONUS;
		else rank_bonus = SHIP_RANK_1_BONUS;

		cur_ship->tpl.value_1 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_2 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_3 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_6 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_7 *= (rank_bonus + bonus_defender);
		cur_ship->tpl.value_8 *= (rank_bonus + bonus_defender);

		firststrike = cur_ship->tpl.value_11 * 0.5 + cur_ship->tpl.value_6 * 2 + cur_ship->tpl.value_7 * 3 + cur_ship->tpl.value_8 + cur_ship->tpl.value_12;

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
		DEBUG_LOG("%i (fleet: %i xp: %i hp: %.0f units: %i/%i/%i/%i type: %.0f/%.0f values: %.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.1f/%.0f/%.0f)\n", 
				cur_ship->ship_id, cur_ship->fleet->fleet_id, cur_ship->experience, cur_ship->hitpoints,
				cur_ship->unit_1, cur_ship->unit_2, cur_ship->unit_3, cur_ship->unit_4,
				cur_ship->tpl.ship_torso, cur_ship->tpl.ship_class,
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
		DEBUG_LOG("%i (fleet: %i xp: %i hp: %.0f units: %i/%i/%i/%i type: %.0f/%.0f values: %.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.0f/%.1f/%.0f/%.0f)\n", 
				cur_ship->ship_id, cur_ship->fleet->fleet_id, cur_ship->experience, cur_ship->hitpoints,
				cur_ship->unit_1, cur_ship->unit_2, cur_ship->unit_3, cur_ship->unit_4,
				cur_ship->tpl.ship_torso, cur_ship->tpl.ship_class,
				cur_ship->tpl.value_1, cur_ship->tpl.value_2, cur_ship->tpl.value_3, cur_ship->tpl.value_4, cur_ship->tpl.value_5,
				cur_ship->tpl.value_6, cur_ship->tpl.value_7, cur_ship->tpl.value_8, cur_ship->tpl.value_10, cur_ship->tpl.value_11, cur_ship->tpl.value_12);
		
		++cur_ship;
	}

	DEBUG_LOG("\n");
#endif

	return true;
}
