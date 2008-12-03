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


// This feature makes certain Torsos unattractive for attacks
float add_special(int torso_id)
{
	switch (torso_id)
	{
		case 0:
			return 16;
		break;
		case 1:
			return 31;
		break;
		case 2:
		case 7:
			return 46;
		break;
		default:
		break;
	}
	return 0;
}


// Examine the existence of targets
bool cshipclass::check_target()
{
	if (!this->target) return 0;
	if (this->target->ship_reference->hitpoints<=0)
	{
		this->target->num_attackers--;
		this->target=NULL;
		return 0;
	}
	return 1;
}

// Examines this ship a new target from the corresponding list
bool cshipclass::get_target(list<cshipclass*> *ship_list)
{
	if (!this->ship_reference) return 0;

	this->target = NULL;
	double rate = 1.7976931348623158e+308;

	// we are looking for the best target from the normal instance max 50
	int count=0;

	if (this->party==0)
	{
		while(1)
		{
			if (dfd_ship_it!=ship_list->end() && (*dfd_ship_it)->ship_reference->hitpoints>0)
			{
				double test_rate = fabs(this->ship_reference->firststrike - (*dfd_ship_it)->ship_reference->firststrike)
					+ (*dfd_ship_it)->num_attackers+add_special((*dfd_ship_it)->ship_reference->tpl.ship_torso);
				if (test_rate < rate)
				{
					rate=test_rate;
					this->target=(*dfd_ship_it);
				}
			}
			
			++dfd_ship_it;
			if (dfd_ship_it==ship_list->end()) dfd_ship_it=ship_list->begin();
			count++;
			if (count>100 && this->target)
				break;
		};
	}
	else
	{
		while(1)
		{
			if (atk_ship_it!=ship_list->end() && (*atk_ship_it)->ship_reference->hitpoints>0)
			{
				float test_rate = fabs(this->ship_reference->firststrike - (*atk_ship_it)->ship_reference->firststrike)
					+ (*atk_ship_it)->num_attackers+add_special((*atk_ship_it)->ship_reference->tpl.ship_torso);
				if (test_rate < rate)
				{
					rate=test_rate;
					this->target=(*atk_ship_it);
				}
			}

			++atk_ship_it;
			if (atk_ship_it==ship_list->end()) atk_ship_it=ship_list->begin();
			count++;
			if (count>100 && this->target)
			break;
		};
	}

	if (!this->target) return 0;
	this->target->num_attackers++;
	return 1;
}

// Fires a shot on the target
bool cshipclass::shoot()
{
//		if (!this->target) return 0;
//		if (!this->target->ship_reference) return 0;
//		if (!this->ship_reference) return 0;

	// hitchance = reaction + readiness + agility + sensors
	float hitchance = this->ship_reference->tpl.value_6 + this->ship_reference->tpl.value_7
			+ this->ship_reference->tpl.value_8 + this->ship_reference->tpl.value_11;
	hitchance *= 0.1f;
	if (hitchance>12)	hitchance = 12;
	if (hitchance<1)	hitchance = 1;

	int hit = rand()%14;
	if (hit>hitchance) return 0;


	// defchance = (reaction + agility*1.5) * (cloak_target / cloak_attacker)
	float defchance = this->target->ship_reference->tpl.value_6 + this->target->ship_reference->tpl.value_8 * 1.5f;
	defchance *= (this->target->ship_reference->tpl.value_12==0 ? 1 : this->target->ship_reference->tpl.value_12) / (this->ship_reference->tpl.value_12==0 ? 1 : this->ship_reference->tpl.value_12);
	defchance *= 0.1f;
	//if (defchance>18)	defchance = 18;
	// 04/04/08 - CD: Increase chance of defense 
	if (defchance>36)	defchance = 36;
	if (defchance<1)	defchance = 1;
	if (rand()%120 <= defchance) return 0;

	// calculate damage:
	// damage = lw + lw * experience/7500 + hw  + hw * experience/6000
	float damage /*= this->ship_reference->tpl.value_1 + 
	this->ship_reference->tpl.value_1 * ((float)this->ship_reference->experience/7500.0f) + 
	this->ship_reference->tpl.value_2 + 
	this->ship_reference->tpl.value_2 * ((float)this->ship_reference->experience/6000.0f);
	damage *= 0.25*/;

	// DC ---- Damages from phasers
	float phasers_dmg = this->ship_reference->tpl.value_1 + 
	this->ship_reference->tpl.value_1 * ((float)this->ship_reference->experience/7500.0f);
	phasers_dmg *= 0.25;
	//DEBUG_LOG("Calculated phasers's damage: %.3f\n",phasers_dmg);

	// DC ---- Damages from torpedoes
	float torpedoes_dmg = this->ship_reference->tpl.value_2 +
	this->ship_reference->tpl.value_2 * ((float)this->ship_reference->experience/6000.0f);
	torpedoes_dmg *= 0.25;
	//DEBUG_LOG("Calculated torpedoes's damage: %.3f\n",torpedoes_dmg);
	//DC ----

	// berechnen, wieviel durchkommt:
	// tilefactor = 0.5 + reaktion/100 + bereitschaft / 200 + rand(0,100) / 1000
	float tile = 0.5f + 
	this->target->ship_reference->tpl.value_6 * 0.01f + 
	this->target->ship_reference->tpl.value_7 * 0.005f + 
	((float)(rand()%100)) / 1000.0f; // gott würfelt doch!
	/*damage /= tile;
	if (damage>10000.0f) damage=10000.0f;
	*/

	// DC ---- Il fuoco dei phaser risente meno della capacit evasiva della nave rispetto a quello dei siluri
	phasers_dmg /= (tile*0.90);
	if (phasers_dmg > 10000.0f) phasers_dmg = 10000.0f;
	//DEBUG_LOG("Adjusted phasers's damage: %.3f\n",phasers_dmg);

	torpedoes_dmg /= (tile*1.10);
	if (torpedoes_dmg > 10000.0f) torpedoes_dmg = 10000.0f;
	//DEBUG_LOG("Adjusted torpedoes's damage: %.3f\n",torpedoes_dmg);
	// neue schild / hüllenwerte berechnen:

	int restdamage = 0;
	/*
	if (this->target->ship_reference->shields>0)
	{
		if (damage > this->target->ship_reference->shields)
		{
			restdamage = (int)damage - (int)this->target->ship_reference->shields;
			this->target->ship_reference->shields=0;
			this->target->ship_reference->hitpoints -= (int)restdamage/3;
	
			this->target->ship_reference->changed = true;
		}
		else
		{
			this->target->ship_reference->shields -= (int)damage;
			if (this->target->ship_reference->shields<0) this->target->ship_reference->shields=0;
		}
	}
	else
	{
		this->target->ship_reference->changed = true;

		this->target->ship_reference->hitpoints -= (int)damage;
		if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;
	}
	*/
	// DC ---- new dmg code
#if VERBOSE >= 3
	DEBUG_LOG("\nThis is the turn of the: %s\n",(this->party==ATTACKER)?"ATTACKER":"DEFENDER");
#endif

	// First: Firing phasers
	if (this->target->ship_reference->shields>0) 
	{
		damage = (phasers_dmg*1.15);

#if VERBOSE >= 3
		DEBUG_LOG("Shield up: phasers damage inflicted: %.3f\n",damage);
#endif

		if (damage > this->target->ship_reference->shields)
		{
			restdamage = (int)damage - (int)this->target->ship_reference->shields;
			this->target->ship_reference->shields=0;
			this->target->ship_reference->hitpoints -= (int)restdamage/3;

			this->target->ship_reference->changed = true;
		}
		else
		{
			this->target->ship_reference->shields -= (int)damage;
			if (this->target->ship_reference->shields<0) this->target->ship_reference->shields=0;
		}
	}
	else
	{
		damage = (phasers_dmg*0.25);

#if VERBOSE >= 3
		DEBUG_LOG("Shield down: phasers damage inflicted: %.3f\n",damage);
#endif

		this->target->ship_reference->changed = true;

		this->target->ship_reference->hitpoints -= (int)damage;
		if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;
	}
	// Then firing Torpedoes...
	if (this->target->ship_reference->shields>0) 
	{
		damage = (torpedoes_dmg*0.25);

#if VERBOSE >= 3
		DEBUG_LOG("Shield up: torpedoes damage inflicted: %.3f\n",damage);
#endif
		if (damage > this->target->ship_reference->shields)
		{
			restdamage = (int)damage - (int)this->target->ship_reference->shields;
			this->target->ship_reference->shields=0;
			this->target->ship_reference->hitpoints -= (int)restdamage/3;

			this->target->ship_reference->changed = true;
		}
		else
		{
			this->target->ship_reference->shields -= (int)damage;
			if (this->target->ship_reference->shields<0) this->target->ship_reference->shields=0;
		}
	}
	else
	{
		damage = (torpedoes_dmg*1.15);

#if VERBOSE >= 3
		DEBUG_LOG("Shield down: torpedoes damage inflicted: %.3f\n",damage);
#endif
		this->target->ship_reference->changed = true;

		this->target->ship_reference->hitpoints -= (int)damage;
		if (this->target->ship_reference->hitpoints<0) this->target->ship_reference->hitpoints=0;
	}
	// DC ----
	#ifdef KAZON_TORPEDO_SUICIDE
	if (this->ship_reference->tpl.ship_torso==0 && this->ship_reference->tpl.value_1>0) // only the Kazon scout has weapons, therefore, use these features for an identification
	{
		this->target->ship_reference->changed = true;

		this->ship_reference->hitpoints-=4;
		if (this->ship_reference->hitpoints<0) this->ship_reference->hitpoints=0;
	}
	#endif

	if (this->target->ship_reference->hitpoints<=0) return 1;
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
	tmp->tpl.value_11=0;
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
	tmp->tpl.value_11=0;
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
	if ( !(*search)->get_target( &(((*search)->party==0) ? defender_ship_list : attacker_ship_list) ) ) // target search
	{

		DEBUG_LOG("unrecoverable error during target-selection, fight aborted\n");

		return 0;
	}


	while(attacker_count>0 && defender_count>0)
	{
#if VERBOSE >= 3
		num_loops++;
#endif

		if((*it)->ship_reference->hitpoints<=0)
		{
			++it;
			if (it==global_ship_list.end()) it=global_ship_list.begin();
			continue;
		}

		// 1.) target examine:
		if (!(*it)->check_target()) // no target?
		{
			if ( !(*it)->get_target( &(((*it)->party==0) ? defender_ship_list : attacker_ship_list) ) ) // target search
			{
				DEBUG_LOG("unrecoverable error during target-selection, fight aborted\n");

				return 0;
			}
#if VERBOSE >= 3
			else num_targetchanges++;
#endif
		}

		// 2.) shoot:
		if ((*it)->shoot()) // if true, the target was destroyed
		{
			float div1=(*it)->ship_reference->tpl.value_1+(*it)->ship_reference->tpl.value_2;
			float div2=(*it)->target->ship_reference->tpl.value_1+(*it)->target->ship_reference->tpl.value_2;
			(*it)->ship_reference->xp_gained += (float) (10.0f * fmin(10,(div2/((div1 == 0) ? 1 : div1))));
			(*it)->ship_reference->experience +=(float) (10.0f * fmin(10,(div2/((div1 == 0) ? 1 : div1))));

			(*it)->ship_reference->changed = true;

			if ((*it)->target->party==0) attacker_count--;
			else defender_count--;
			(*it)->target->num_attackers--;
			(*it)->target=NULL;
#if VERBOSE >= 3
			num_victims++;
#endif
		} // end of: target was destroyed

		// 2a.) if the shooter was a Kazon torpedo and suicide is active, we still need to examine suicide
		#ifdef KAZON_TORPEDO_SUICIDE
		if ((*it)->ship_reference->hitpoints<=0)
		{
			if ((*it)->party==0) attacker_count--;
			else defender_count--;
		} // end of: ship committed suicide
		#endif


		++it;
		if (it==global_ship_list.end()) it=global_ship_list.begin();

#if VERBOSE >= 3
		num_shots++;

		if (get_current_time()-last_time>1.0f)
		{
			DEBUG_LOG("%d loops: %d shots, %d target-changes, %d+%d ships alive, %d victims\n",num_loops,num_shots,num_targetchanges, attacker_count, defender_count, num_victims);
			last_time=get_current_time();
			num_shots=0;
			num_targetchanges=0;
			num_loops=0;
		}
#endif

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
