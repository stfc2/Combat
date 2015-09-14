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


#include "combat.hh"

list<cshipclass*>::iterator atk_ship_it;
list<cshipclass*>::iterator dfd_ship_it;


struct cshipclass_ptr_cmp{
	bool operator()(const cshipclass* lhs, const cshipclass* rhs)
	{
		if (rand()%202>=100)
			return (lhs->ship_reference->firststrike > rhs->ship_reference->firststrike);
		else
			return (lhs->ship_reference->firststrike >= rhs->ship_reference->firststrike);
	}
};

// Examine the existence of targets
bool cshipclass::check_target()
{
	if (!this->target) return 0;
	if (this->target->ship_reference->knockout)
	{
		this->target->num_attackers--;
		this->target=NULL;
		return 0;
	}
	return 1;
}

// Examine ship health
bool cshipclass::check_systems()
{
	float damage_ratio, escape_chance, escape_roll;
	int damage_threshold;
#if VERBOSE >= 5
	DEBUG_LOG("Entering check_systems in get_target\n");
#endif

        escape_chance = 0;
    
	if(this->ship_reference->ship_id<0) return 0; // Planetary guns do not flee!!!
    
        if(this->ship_reference->tpl.ship_torso > 11) return 0; // Orbital guns either!!!!

	if(this->ship_reference->tpl.race == 2 || 
           this->ship_reference->tpl.race == 4 ||
           this->ship_reference->tpl.race == 6 || 
           this->ship_reference->tpl.race == 8 || 
           this->ship_reference->tpl.race == 9) return 0;

	switch(this->ship_reference->tpl.race)
	{
	// Federals
	case 0:
		escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 80.0) escape_chance = 80.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);
                damage_threshold = 25;
		break;
	// Romulans
	// Il check si basa sulla caratteristica occultamento
	case 1:
		escape_chance = (this->ship_reference->tpl.value_12*1.1) - (this->num_attackers*5) + (this->ship_reference->tpl.value_6*0.4);
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 85.0) escape_chance = 85.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);
                damage_threshold = 25;
		break;
        case 2:
                break;
	// Cardassians
	case 3:
		escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 80.0) escape_chance = 80.0;
		escape_roll = (this->party == ATTACKER ? rand()%90 : rand()%100);
                damage_threshold = 25;
		break;
        case 4:
            break;
        // Ferengi
        case 5:
		escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 95.0) escape_chance = 95.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);    
                damage_threshold = 25;
                break;
        case 6:
        case 7:
        case 8:
        case 9:
                break;
        // Krenim
        case 10:
                escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 90.0) escape_chance = 90.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);
                damage_threshold = 25;
                break;
        // Kazon
        case 11:
		escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 95.0) escape_chance = 95.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);
                damage_threshold = 25;
                break;
        case 12:
                break;
        // Settlers
        case 13:
		escape_chance = this->ship_reference->tpl.value_6*0.3 + this->ship_reference->tpl.value_7*0.4
				+ this->ship_reference->tpl.value_8*0,4 + this->ship_reference->tpl.value_11*0.5;
		if(escape_chance <  1.0) escape_chance = 1.0;
		if(escape_chance > 95.0) escape_chance = 95.0;
		escape_roll = (this->party == ATTACKER ? rand()%100 : rand()%100);
                damage_threshold = 25;    
                break;
	default:
		return 0;
		break;
	}

	damage_threshold = damage_threshold - (this->ship_reference->experience*0.1);
    
        if(damage_threshold < 1) damage_threshold = 1;
	
	damage_ratio = (this->ship_reference->hitpoints*100)/this->ship_reference->tpl.value_5;
	
	if((damage_ratio < damage_threshold) && (escape_chance > escape_roll)){
		this->ship_reference->fleed = true;
		this->ship_reference->knockout = true;
		this->ship_reference->xp_gained++;
		return 1;
	}

	return 0;
}

// Examines this ship a new target from the corresponding list
bool cshipclass::get_target(list<cshipclass*> *ship_list)
{
	if (!this->ship_reference) return 0;

	this->target = NULL;
	double rate = 1.7976931348623158e+308;

	// we are looking for the best target from the normal instance max 50
	int count=0;

#if VERBOSE >= 5
	DEBUG_LOG("Entering loops in get_target\n");
#endif

	if (this->party==ATTACKER)
	{
		while(1)
		{
			if ((!(*dfd_ship_it)->ship_reference->knockout) && (dfd_ship_it!=ship_list->end()))
			{
				double test_rate = fabs(this->ship_reference->firststrike - (*dfd_ship_it)->ship_reference->firststrike)
					+ (*dfd_ship_it)->num_attackers;
				if (test_rate < rate)
				{
					rate=test_rate;
					this->target=(*dfd_ship_it);
				}
				count++;
			}
			
			++dfd_ship_it;
			if (dfd_ship_it==ship_list->end()) dfd_ship_it=ship_list->begin();
			if (count>50) break;
		};
	}
	else
	{
		while(1)
		{
			if ((!(*atk_ship_it)->ship_reference->knockout) && (atk_ship_it!=ship_list->end()))
			{
				float test_rate = fabs(this->ship_reference->firststrike - (*atk_ship_it)->ship_reference->firststrike)
					+ (*atk_ship_it)->num_attackers;
				if (test_rate < rate)
				{
					rate=test_rate;
					this->target=(*atk_ship_it);
				}
				count++;
			}

			++atk_ship_it;
			if (atk_ship_it==ship_list->end()) atk_ship_it=ship_list->begin();
			if (count>50) break;
		};
	}
#if VERBOSE >= 5
	DEBUG_LOG("Exiting loops in get_target\n");
#endif

	if (!this->target) return 0;
	this->target->num_attackers++;
	return 1;
}

// Firing phasers!!
bool cshipclass::primary_shoot()
{
	float hitchance = 0;
	// The calculation for hitchance is the same for the both shoot routines, should we create only one function
	// in order to semplify?
	if (this->target->ship_reference->tpl.value_12 > 0)
		{
			hitchance = (this->ship_reference->tpl.value_6 + this->ship_reference->tpl.value_7
				  + this->ship_reference->tpl.value_8)*0,5 + this->ship_reference->tpl.value_11*1,5;
			hitchance *= 0.1f;
		}
		else
		{
			hitchance = (this->ship_reference->tpl.value_6 + this->ship_reference->tpl.value_7 + this->ship_reference->tpl.value_8)*0,5
				  + (this->ship_reference->tpl.value_11 - this->target->ship_reference->tpl.value_12)*1,5;
			hitchance *= 0.1f;
		}
	hitchance *= 0.1f;

	if (hitchance>15)	hitchance = 15;
	if (hitchance<1)	hitchance = 1;

	int hit = rand()%17;

	if (hit>hitchance) return 0;

	// defchance = (reaction + agility*1.5) * (cloak_target / cloak_attacker)
	float defchance = this->target->ship_reference->tpl.value_6 * 1.1 + this->target->ship_reference->tpl.value_8 * 2.4f;

	defchance *= (this->target->ship_reference->tpl.value_12==0 ? 1 : this->target->ship_reference->tpl.value_12) / (this->ship_reference->tpl.value_12==0 ? 1 : this->ship_reference->tpl.value_12);
	defchance *= 0.1f;

	if (defchance>15) defchance = 15;
	if (defchance<1) defchance = 1;
	if (rand()%40 <= defchance) return 0;

	// calculate damage:
	// damage = lw + lw * experience/7500
	float damage = 0;

	// DC ---- Damages from phasers
	float phasers_dmg = this->ship_reference->tpl.value_1 + (this->ship_reference->tpl.value_1 * ((float)this->ship_reference->experience/1000.0f));
	phasers_dmg *= 0.25;

	// Damage reduction according to the statistics of the hitted ship
	// tilefactor = 0.5 + reaktion/100 + bereitschaft / 200 + rand(0,100) / 1000
	float tile = 0.5f + this->target->ship_reference->tpl.value_6 * 0.01f + this->target->ship_reference->tpl.value_7 * 0.005f + ((float)(rand()%100)) / 1000.0f;

	phasers_dmg /= tile;
	if (phasers_dmg > 10000.0f) phasers_dmg = 10000.0f;

	int restdamage = 0;

	#if VERBOSE >= 5
		DEBUG_LOG("\nPhaser turn of the: %s\n",(this->party==ATTACKER)?"ATTACKER":"DEFENDER");
	#endif

	this->target->ship_reference->changed = true;
        
	if (this->target->ship_reference->shields>0)
	{
		if ((phasers_dmg*0.90) > this->target->ship_reference->shields)
		{
			restdamage = (int)phasers_dmg - (int)this->target->ship_reference->shields;
			this->target->ship_reference->shields=0;
			this->target->ship_reference->hitpoints -= (int)restdamage/3;
		}
		else
		{
			this->target->ship_reference->shields -= (int)(phasers_dmg*0.90);
			if (this->target->ship_reference->shields<0) this->target->ship_reference->shields=0;
                        this->target->ship_reference->hitpoints -= (int)(phasers_dmg*0.10);
                        if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;                        
		}

	}
	else
	{
		this->target->ship_reference->hitpoints -= (int)phasers_dmg;
		if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;
	}

	if (this->target->ship_reference->hitpoints<=0)	return 1;

	return 0;
}


// Torpedoes Away!!!
bool cshipclass::secondary_shoot()
{
    
        if(this->ship_reference->tpl.ship_torso<3) this->ship_reference->torp = 1; // Fake torpedoes for fighter, cargoes and coloships

	if (this->ship_reference->torp > 0)
	{
		this->ship_reference->torp--;
		this->ship_reference->changed = true;
	}
	else
	{
                this->ship_reference->rof2 = 0;
		return 0;
	}
                
	// hitchance = reaction + readiness + agility + sensors
	float hitchance = 0;

	if (this->target->ship_reference->tpl.value_12 > 0)
	{
                hitchance = (this->ship_reference->tpl.value_6 + this->ship_reference->tpl.value_7
		          + this->ship_reference->tpl.value_8)*0,5 + this->ship_reference->tpl.value_11*1,5;
		hitchance *= 0.1f;
	}
	else
	{
		hitchance = (this->ship_reference->tpl.value_6 + this->ship_reference->tpl.value_7 + this->ship_reference->tpl.value_8)*0,5
                          + (this->ship_reference->tpl.value_11 - this->target->ship_reference->tpl.value_12)*1,5;
		hitchance *= 0.1f;
	}
	hitchance *= 0.1f;
	if (hitchance>15)	hitchance = 15;
	if (hitchance<1)	hitchance = 1;

	int hit = rand()%17;
	if (hit>hitchance) return 0;

	// defchance = (reaction + agility*1.5) * (cloak_target / cloak_attacker)
	float defchance = this->target->ship_reference->tpl.value_6 * 1.1 + this->target->ship_reference->tpl.value_8 * 2.4f;

	defchance *= (this->target->ship_reference->tpl.value_12==0 ? 1 : this->target->ship_reference->tpl.value_12) / (this->ship_reference->tpl.value_12==0 ? 1 : this->ship_reference->tpl.value_12);
	defchance *= 0.1f;

	if (defchance>15) defchance = 15;
	if (defchance<1) defchance = 1;
	if (rand()%30 <= defchance) return 0;

	// calculate damage:
	// damage = hw  + hw * experience/6000
	float damage = 0;

	// DC ---- Damages from torpedoes  ROF
	float torpedoes_dmg = this->ship_reference->tpl.value_2 + (this->ship_reference->tpl.value_2 * ((float)this->ship_reference->experience/500.0f));
	torpedoes_dmg *= 0.40;


	/* DC ---- Damages from torpedoes senza ROF
	float torpedoes_dmg = this->ship_reference->tpl.value_2 + this->ship_reference->tpl.value_2 * ((float)this->ship_reference->experience/500.0f);
	torpedoes_dmg *= 0.25;
	*/

	// tilefactor = 0.5 + reaktion/100 + bereitschaft / 200 + rand(0,100) / 1000
	float tile = 0.5f +
	this->target->ship_reference->tpl.value_6 * 0.01f +
	this->target->ship_reference->tpl.value_7 * 0.005f +
	((float)(rand()%100)) / 1000.0f;

	torpedoes_dmg /= tile;
	if (torpedoes_dmg > 10000.0f) torpedoes_dmg = 10000.0f;
	//DEBUG_LOG("Adjusted torpedoes's damage: %.3f\n",torpedoes_dmg);


	int restdamage = 0;

#if VERBOSE >= 5
	DEBUG_LOG("\nTorpedo turn of the: %s\n",(this->party==ATTACKER)?"ATTACKER":"DEFENDER");
#endif

	this->target->ship_reference->changed = true;
	if (this->target->ship_reference->shields>0)
	{
		torpedoes_dmg *= 0.50;
		if (torpedoes_dmg > this->target->ship_reference->shields)
		{
			restdamage = (int)torpedoes_dmg - (int)this->target->ship_reference->shields;
			this->target->ship_reference->shields=0;
			this->target->ship_reference->hitpoints -= (int)restdamage/3;
		}
		else
		{
			this->target->ship_reference->shields -= (int)torpedoes_dmg;
			if (this->target->ship_reference->shields<0) this->target->ship_reference->shields=0;
                        torpedoes_dmg *= 0.20;
                        this->target->ship_reference->hitpoints -= (int)torpedoes_dmg;
                        if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;                        
		}

	}
	else
	{
		this->target->ship_reference->hitpoints -= (int)torpedoes_dmg;
		if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;
	}

	if (this->target->ship_reference->hitpoints<=0)	return 1;

	return 0;
}

s_ship *create_large_orbital()
{
	s_ship *tmp = new s_ship;

	tmp->ship_id=-1;
	tmp->fleet = NULL;
	tmp->experience=10;
	tmp->hitpoints=PLANETARY_DEFENSE_DEFENSE;
	tmp->unit_1=0;
	tmp->unit_2=0;
	tmp->unit_3=0;
	tmp->unit_4=0;
	tmp->firststrike=100;
	tmp->shields=0;
	tmp->rof=10;
        tmp->rof2=5;
	tmp->torp=600;
        tmp->atk_lvl=1;
	tmp->captured = false;
	tmp->fleed = false;
	tmp->knockout = false;
	tmp->surrendered = false;
	tmp->changed = false;
	tmp->tpl.ship_torso=4;
	tmp->tpl.value_1=PLANETARY_DEFENSE_ATTACK;
	tmp->tpl.value_2=PLANETARY_DEFENSE_ATTACK2;
	tmp->tpl.value_3=0;
	tmp->tpl.value_4=0;
	tmp->tpl.value_5=0;
	tmp->tpl.value_6=20;
	tmp->tpl.value_7=20;
	tmp->tpl.value_8=20;
	tmp->tpl.value_10=0;
	tmp->tpl.value_11=36;
	tmp->tpl.value_12=0;
	return tmp;
}



s_ship *create_small_orbital()
{
	s_ship *tmp = new s_ship;

	tmp->ship_id=-2;
	tmp->fleet = NULL;
	tmp->experience=10;
	tmp->hitpoints=SPLANETARY_DEFENSE_DEFENSE;
	tmp->unit_1=0;
	tmp->unit_2=0;
	tmp->unit_3=0;
	tmp->unit_4=0;
	tmp->firststrike=100;
	tmp->shields=0;
	tmp->rof=6;
        tmp->rof2=0;
	tmp->torp=0;
        tmp->atk_lvl=0;        
	tmp->captured = false;
	tmp->fleed = false;
	tmp->knockout = false;
	tmp->surrendered = false;
	tmp->changed = false;
	tmp->tpl.ship_torso=4;
	tmp->tpl.value_1=SPLANETARY_DEFENSE_ATTACK;
	tmp->tpl.value_2=SPLANETARY_DEFENSE_ATTACK2;
	tmp->tpl.value_3=0;
	tmp->tpl.value_4=0;
	tmp->tpl.value_5=0;
	tmp->tpl.value_6=20;
	tmp->tpl.value_7=20;
	tmp->tpl.value_8=20;
	tmp->tpl.value_10=0;
	tmp->tpl.value_11=36;
	tmp->tpl.value_12=0;
	return tmp;
}




int process_combat(s_move_data* move_data)
{
	srand(time(NULL));
	list<cshipclass*> global_ship_list; // This list is iterated with the battle and includes all vessels
	list<cshipclass*> attacker_ship_list; // This list is iterated with the battle and includes all vessels
	list<cshipclass*> defender_ship_list; // This list is iterated with the battle and includes all vessels

	int c=0;
        int primary=0;
        int secondary=0;
        bool knockout=false;

	// First comes the check whether at least one ship has weapons (if only normal scouts has to fight, enters this case)
	float sum_weapons=0;
	for (c=0; c<move_data->n_atk_ships; ++c)
		sum_weapons+=move_data->atk_ships[c].tpl.value_1+move_data->atk_ships[c].tpl.value_2;
	for (c=0; c<move_data->n_dfd_ships; ++c)
		sum_weapons+=move_data->dfd_ships[c].tpl.value_1+move_data->dfd_ships[c].tpl.value_2;

	if (sum_weapons==0 && move_data->n_large_orbital_defense==0 && move_data->n_small_orbital_defense==0)
	{
		DEBUG_LOG("none of the ships carries a weapon, enable scout-fight-mode\n");

		for (c=0; c<move_data->n_atk_ships; ++c)
			move_data->atk_ships[c].tpl.value_1=move_data->atk_ships[c].tpl.value_2=100;
		for (c=0; c<move_data->n_dfd_ships; ++c)
			move_data->dfd_ships[c].tpl.value_1=move_data->dfd_ships[c].tpl.value_2=100;
	}

	// attacking vessels in the list
	for (c=0; c<move_data->n_atk_ships; ++c)
	{
		cshipclass *tmp=new cshipclass;
		tmp->ship_reference=&move_data->atk_ships[c];
		tmp->party=ATTACKER;
		tmp->target=NULL;
		tmp->num_attackers=0;
		global_ship_list.push_back(tmp);
		attacker_ship_list.push_back(tmp);
	}


	// defending ships in the list
	for (c=0; c<move_data->n_dfd_ships; ++c)
	{
		cshipclass *tmp=new cshipclass;
		tmp->ship_reference=&move_data->dfd_ships[c];
		tmp->party=DEFENDER;
		tmp->target=NULL;
		tmp->num_attackers=0;
		global_ship_list.push_back(tmp);
		defender_ship_list.push_back(tmp);
	}

	// large  orbital in the list
	for (c=0; c<move_data->n_large_orbital_defense; ++c)
	{
		cshipclass *tmp=new cshipclass;
		tmp->ship_reference=create_large_orbital();
		tmp->party=DEFENDER;
		tmp->target=NULL;
		tmp->num_attackers=0;
		global_ship_list.push_back(tmp);
		defender_ship_list.push_back(tmp);
	}
	// small orbital in the list
	for (c=0; c<move_data->n_small_orbital_defense; ++c)
	{
		cshipclass *tmp=new cshipclass;
		tmp->ship_reference=create_small_orbital();
		tmp->party=DEFENDER;
		tmp->target=NULL;
		tmp->num_attackers=0;
		global_ship_list.push_back(tmp);
		defender_ship_list.push_back(tmp);
	}

	// all vessels in the global list after first strike sort (attacker and defender lists do not need to be sorted)
	global_ship_list.sort(cshipclass_ptr_cmp());

#if VERBOSE >= 3
	double last_time=get_current_time();
	int num_shots=0;
	int num_targetchanges=0;
	int num_loops=0;
	int num_victims=0;

	DEBUG_LOG("\n");
#endif

	int attacker_count=attacker_ship_list.size();
	int defender_count=defender_ship_list.size();

	if (defender_count==0) return -1;
	if (attacker_count==0) return 1;

	// now is fight!
	list<cshipclass*>::iterator it=global_ship_list.begin();
	atk_ship_it=attacker_ship_list.begin();
	dfd_ship_it=defender_ship_list.begin();

	for(list<cshipclass*>::iterator search = global_ship_list.begin(); search != global_ship_list.end(); ++search)
	if ( !(*search)->get_target( &(((*search)->party==ATTACKER) ? defender_ship_list : attacker_ship_list) ) ) // target search
	{

		DEBUG_LOG("unrecoverable error during target-selection, fight aborted\n");

		return 0;
	}

#if VERBOSE >= 5
	DEBUG_LOG("Starting with %i attackers and %i defenders",attacker_count,defender_count);
#endif

	while(attacker_count>0 && defender_count>0)
	{
#if VERBOSE >= 3
		num_loops++;
#endif

		if((*it)->ship_reference->knockout)
		{
			++it;
			if (it==global_ship_list.end()) it=global_ship_list.begin();
			continue;
		}

		// Damage Control
		if((*it)->check_systems())
		{
			if ((*it)->party==0) attacker_count--;
			else defender_count--;
#if VERBOSE >= 5
			DEBUG_LOG("Ship escaping combat, now we have %i attackers and %i defenders",attacker_count,defender_count);
#endif
			++it;
			if (it==global_ship_list.end()) it=global_ship_list.begin();
			continue;
		}

                
                switch((*it)->ship_reference->atk_lvl) {
                    case 0:
                        // Fire Primary & Secondary coupled
                        primary   = (*it)->ship_reference->rof;
                        secondary = (*it)->ship_reference->rof2;
                        while(primary > 0 || secondary > 0)
                        {
                            if (!(*it)->check_target()) // no target?
                            {
                                if ( !(*it)->get_target( &(((*it)->party==ATTACKER) ? defender_ship_list : attacker_ship_list) ) ) // target search
                                {
                                    DEBUG_LOG("unrecoverable error during target-selection, fight aborted\n");

                                    return 0;
                                }
                            }
                            
                            knockout=false;
                            
                            if (primary > 0)
                            {
                                
                                if ((*it)->primary_shoot())
                                {
                                    float div1=(*it)->ship_reference->tpl.value_1+(*it)->ship_reference->tpl.value_2;
                                    float div2=(*it)->target->ship_reference->tpl.value_1+(*it)->target->ship_reference->tpl.value_2;
                                    (*it)->ship_reference->xp_gained += (float) (10.0f * fmin(750,(div2/((div1 == 0) ? 1 : div1))));
                                    (*it)->ship_reference->experience +=(float) (10.0f * fmin(10,(div2/((div1 == 0) ? 1 : div1))));

                                    (*it)->target->ship_reference->knockout = true;
                                    knockout=true;

                                    (*it)->ship_reference->changed = true;
                                    (*it)->ship_reference->deathblows++;

                                    if ((*it)->target->party==ATTACKER) attacker_count--;
                                    else defender_count--;

                                    (*it)->target->num_attackers--;
                                    (*it)->target=NULL;

                                }                                
                                
                                primary--;
                            
                            }                            

                            if (!knockout && secondary > 0)
                            {
                                
                                if ((*it)->secondary_shoot())
                                {
                                    float div1=(*it)->ship_reference->tpl.value_1+(*it)->ship_reference->tpl.value_2;
                                    float div2=(*it)->target->ship_reference->tpl.value_1+(*it)->target->ship_reference->tpl.value_2;
                                    (*it)->ship_reference->xp_gained += (float) (10.0f * fmin(750,(div2/((div1 == 0) ? 1 : div1))));
                                    (*it)->ship_reference->experience +=(float) (10.0f * fmin(10,(div2/((div1 == 0) ? 1 : div1))));

                                    (*it)->target->ship_reference->knockout = true;

                                    (*it)->ship_reference->changed = true;
                                    (*it)->ship_reference->deathblows++;

                                    if ((*it)->target->party==ATTACKER) attacker_count--;
                                    else defender_count--;

                                    (*it)->target->num_attackers--;
                                    (*it)->target=NULL;

                                }                                
                                
                                secondary--;
                            
                            }
                            
                            // Escape check # 1.
                            if(attacker_count <= 0 || defender_count <= 0) primary = secondary = 0;
                            
                        }
                        break;
                    case 1:
                        // Fire Primary for shields, Secondary for hulls
                        primary   = (*it)->ship_reference->rof;
                        secondary = (*it)->ship_reference->rof2;
                        while(primary > 0 || secondary > 0) {
                            
                            if (!(*it)->check_target()) // no target?
                            {
                                if ( !(*it)->get_target( &(((*it)->party==ATTACKER) ? defender_ship_list : attacker_ship_list) ) ) // target search
                                {
                                    DEBUG_LOG("unrecoverable error during target-selection, fight aborted\n");

                                    return 0;
                                }
                            }
                            
                            knockout=false;

                            if(secondary > 0 && (*it)->target->ship_reference->shields == 0)
                            {
                                // Target shields are down, firing secondary
                                knockout = (*it)->secondary_shoot();
                                secondary--;
                            }
                            else if(primary > 0)
                            {
                                // Target shields are up, firing primary
                                knockout = (*it)->primary_shoot();
                                primary--;
                                
                            }
                            else secondary = 0; // Escape check #2.

                            
                            if (knockout)
                            {
                                float div1=(*it)->ship_reference->tpl.value_1+(*it)->ship_reference->tpl.value_2;
                                float div2=(*it)->target->ship_reference->tpl.value_1+(*it)->target->ship_reference->tpl.value_2;
                                (*it)->ship_reference->xp_gained += (float) (10.0f * fmin(750,(div2/((div1 == 0) ? 1 : div1))));
                                (*it)->ship_reference->experience +=(float) (10.0f * fmin(10,(div2/((div1 == 0) ? 1 : div1))));

                                (*it)->target->ship_reference->knockout = true;

                                (*it)->ship_reference->changed = true;
                                (*it)->ship_reference->deathblows++;

                                if ((*it)->target->party==ATTACKER) attacker_count--;
                                else defender_count--;

                                (*it)->target->num_attackers--;
                                (*it)->target=NULL;
                            }
                                
                            // Escape check # 1.
                            if(attacker_count <= 0 || defender_count <= 0) primary = secondary = 0;
                            
                        }
                        
                        break;
                }
                                
		++it;
		if (it==global_ship_list.end()) it=global_ship_list.begin();

	}; // end of the battle loops

#if VERBOSE >= 3
	DEBUG_LOG("\n");
#endif

	// the battle has ended, we count the dead orbital defense (-> which could also destroy already in the making!)

	for(list<cshipclass*>::iterator search = defender_ship_list.begin(); search != defender_ship_list.end(); ++search)
	{
		if ((*search)->ship_reference->ship_id==-1 && (*search)->ship_reference->hitpoints<=0) ++move_data->destroyed_large_orbital_defense;
		if ((*search)->ship_reference->ship_id==-2 && (*search)->ship_reference->hitpoints<=0) ++move_data->destroyed_small_orbital_defense;
	}

	return ((attacker_count == 0) ? 1 : -1);
}
