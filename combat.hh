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

#define SPLANETARY_DEFENSE_ATTACK 160
#define SPLANETARY_DEFENSE_ATTACK2 0
#define SPLANETARY_DEFENSE_DEFENSE 4500
#define PLANETARY_DEFENSE_ATTACK 160
#define PLANETARY_DEFENSE_ATTACK2 450
#define PLANETARY_DEFENSE_DEFENSE 10000

#define SHIP_TORSO_TRANSPORTER 1

#define MAX_TRANSPORT_RESOURCES 4000
#define MAX_TRANSPORT_UNITS 400

#define SHIP_RANK_0_LIMIT 25
#define SHIP_RANK_1_LIMIT 40
#define SHIP_RANK_2_LIMIT 50
#define SHIP_RANK_3_LIMIT 60
#define SHIP_RANK_4_LIMIT 70
#define SHIP_RANK_5_LIMIT 80
#define SHIP_RANK_6_LIMIT 90
#define SHIP_RANK_7_LIMIT 100
#define SHIP_RANK_8_LIMIT 110
#define SHIP_RANK_9_LIMIT 120

#define SHIP_RANK_TIER_1  140
#define SHIP_RANK_TIER_2  155
#define SHIP_RANK_TIER_3  170
#define SHIP_RANK_TIER_4  185
#define SHIP_RANK_TIER_5  200

#define SHIP_RANK_0_BONUS 0.12
// #define SHIP_RANK_1_BONUS 0.02
// #define SHIP_RANK_2_BONUS 0.05
#define SHIP_RANK_3_BONUS 0.20
// #define SHIP_RANK_4_BONUS 0.12
// #define SHIP_RANK_5_BONUS 0.16
#define SHIP_RANK_6_BONUS 0.28
// #define SHIP_RANK_7_BONUS 0.24
#define SHIP_RANK_8_BONUS 0.32
// #define SHIP_RANK_9_BONUS 0.32

#define SHIP_RANK_TIER_1_BONUS 1
#define SHIP_RANK_TIER_2_BONUS 1
#define SHIP_RANK_TIER_3_BONUS 2
#define SHIP_RANK_TIER_4_BONUS 3
#define SHIP_RANK_TIER_5_BONUS 2

#define OPTIMAL_0 0.05
#define OPTIMAL_1 0.25
#define OPTIMAL_2 0.55
#define OPTIMAL_3 0.25

#define MAX_SHIP_CLASS 3


struct template_stat {
    int deathblows;   // # final blow delivered
    int out;          // # knocked out
    int damaged;      // # damaged
    int h_damaged;    // # heavily damaged
    int v_h_damaged;  // # very heavily damaged
    int escaped;      // # escaped from combat
};

struct s_fleet {
	int fleet_id;
        int owner;     // fleet owner, used for movement managing
	int n_ships;
	int n_transporter;
        int position;  // fleet position, used for movement managing
        int homebase;  // fleet home, used for movement managing

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
	// Some fields are float, as in later
	// multiplications rational numbers also occur
	// It costs us with 50,000 ships about 586 KiB,
	// However, the CPU saves a lot of calculations in
	// those 50,000 ships
	float value_1; // l.weapons
	float value_2; // h.weapons
	float value_3; // pl.weapons
	short value_4; // shield
	short value_5; // hull
	float value_6; // reaction
	float value_7; // readiness
	float value_8; // agility
	float value_10; // warp
	short value_11; // sensors
	short value_12; // cloak
        short unit_5;   // techs onboard
        short unit_6;   // scientists onboard
	char ship_torso;
	char race; // is REALLY needed?
	char ship_class;
};

struct s_ship {
	int ship_id;
	s_fleet* fleet;
	int user_id; // we really need?
	float experience;
	float xp_gained;
	bool knockout;        // ship is not active
	bool changed;         // ship data are changed
	bool fleed;           // ship has fleed the fight
	bool surrendered;     // ship has surrendered
	bool captured;        // ship has been captured
        int  deathblows;      // # of final blows delivered by the ship in the battle
	float hitpoints;
	float previous_hitpoints;
	short unit_1;
	short unit_2;
	short unit_3;
	short unit_4;
	float firststrike;
	float shields;
	short torp;
	short rof;
        short rof2;
        short dmg_ctrl;       // % of dmg mitigated at the end of the fight
        char atk_lvl;         // selector for firing pattern
        int ship_template_id; // template id
	s_ship_template tpl;
};


#include <list>

class cshipclass {
	private:

	public:
		s_ship* ship_reference;
		cshipclass* target;

		bool party; // ATTACKER or DEFENDER
		int num_attackers; // number of ships, the degree of this ship as a target have

		bool check_target();
		bool check_systems();
		bool get_target(list<cshipclass*> *ship_list);
		bool primary_shoot();
		bool secondary_shoot();
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

// Optimize later
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
