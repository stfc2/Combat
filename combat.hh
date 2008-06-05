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


#ifndef __COMBAT_H__
#define __COMBAT_H__


#include "defines.hh"

#define ATTACKER 0
#define DEFENDER 1

#define SPLANETARY_DEFENSE_ATTACK 350
#define SPLANETARY_DEFENSE_ATTACK2 0
#define SPLANETARY_DEFENSE_DEFENSE 1000
#define PLANETARY_DEFENSE_ATTACK 100
#define PLANETARY_DEFENSE_ATTACK2 400
#define PLANETARY_DEFENSE_DEFENSE 2500

#define SHIP_TORSO_TRANSPORTER 1

#define MAX_TRANSPORT_RESOURCES 4000
#define MAX_TRANSPORT_UNITS 400

#define SHIP_RANK_0_LIMIT 0
#define SHIP_RANK_1_LIMIT 10
#define SHIP_RANK_2_LIMIT 50
#define SHIP_RANK_3_LIMIT 60
#define SHIP_RANK_4_LIMIT 70
#define SHIP_RANK_5_LIMIT 80
#define SHIP_RANK_6_LIMIT 90
#define SHIP_RANK_7_LIMIT 99
#define SHIP_RANK_8_LIMIT 100
#define SHIP_RANK_9_LIMIT 101

#define SHIP_RANK_0_BONUS 0
#define SHIP_RANK_1_BONUS 0.02
#define SHIP_RANK_2_BONUS 0.05
#define SHIP_RANK_3_BONUS 0.08
#define SHIP_RANK_4_BONUS 0.12
#define SHIP_RANK_5_BONUS 0.16
#define SHIP_RANK_6_BONUS 0.20
#define SHIP_RANK_7_BONUS 0.24
#define SHIP_RANK_8_BONUS 0.28
#define SHIP_RANK_9_BONUS 0.32

#define OPTIMAL_0 0.05
#define OPTIMAL_1 0.25
#define OPTIMAL_2 0.55
#define OPTIMAL_3 0.25

#define MAX_SHIP_CLASS 3


struct s_fleet {
	int fleet_id;
	int n_ships;
	int n_transporter;

	int resource_1;
	int resource_2;
	int resource_3;
	int resource_4;
	int unit_1;
	int unit_2;
	int unit_3;
	int unit_4;
	int unit_5;
	int unit_6;
};

struct s_ship_template {
	// einige Felder sind float, da bei späteren
	// Multiplikationen auch rationale Zahlen vorkommen
	// Das kostet uns zwar mit 50.000 Schiffen etwa 586 KiB,
	// erspart der CPU jedoch eine Menge Berechnungen bei
	// diesen 50.000 Schiffen
	float value_1; // l.waffen
	float value_2; // s.waffen
	float value_3; // pl.waffen
	short value_4; // schilde
	short value_5; // hülle
	float value_6; // reaktion
	float value_7; // bereitschaft
	float value_8; // wendigkeit
	float value_10; // warp
	short value_11; // sensoren
	short value_12; // tarnung
	char ship_torso;
	char race; // wird das WIRKLICH gebraucht?
	char ship_class;
};

struct s_ship {
	int ship_id;
	s_fleet* fleet;
	int user_id; // brauchen wir das wirklich?
	float experience;
	float xp_gained;
	bool changed;
	float hitpoints;
	float previous_hitpoints;
	short unit_1;
	short unit_2;
	short unit_3;
	short unit_4;
	float firststrike;
	float shields;
	s_ship_template tpl;
};

#include <list>

class cshipclass {
	private:
	
	public:
		s_ship* ship_reference;
		cshipclass* target;
		
		bool party; // ATTACKER oder DEFENDER
		int num_attackers; // anzahl der schiffe, die grad dieses schiff als target haben

		bool check_target();
		bool get_target(list<cshipclass*> *ship_list);
		bool shoot();
};

struct s_move_data {
	int n_atk_fleets;
	int n_dfd_fleets;

	s_fleet* atk_fleets;
	s_fleet* dfd_fleets;

	int n_atk_ships;
	int n_dfd_ships;

	s_ship* atk_ships;
	s_ship* dfd_ships;

	int n_ship_templates;

// späteres Optimieren
//	list<cshipclass*> atk_ships_list;
//	list<cshipclass*> dfd_ships_list;
//	list<cshipclass*> global_ships_list

	int dest;
	int user_id; // Attacker userID

	short n_large_orbital_defense;
	short destroyed_large_orbital_defense;

	short n_small_orbital_defense;
	short destroyed_small_orbital_defense;

	bool all_orbital;
};


bool prepare_combat(s_move_data* move, char** argv);

int process_combat(s_move_data* data);

bool finish_combat(s_move_data* move, int winner, char** argv);


#endif
