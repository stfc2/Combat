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


#ifndef __DB_CORE_H__
#define __DB_CORE_H__

#include <mysql/mysql.h>

enum e_db_state { DBSTATE_UNCONNECTED, DBSTATE_CONNECTED };


class c_db_result {
	private:
		MYSQL_RES* result_id;
	
	public:
		MYSQL_ROW row;
	
		c_db_result(MYSQL_RES* in_result_id);
		~c_db_result();
	
		bool fetch_row();		
	
		my_ulonglong num_rows();
		
};

class c_db_core {
	private:
		e_db_state state;
	
		MYSQL mysql;
	
		char* db_host;
		char* db_name;	
		char* db_user;
		char* db_passwd;
	
		bool raise_error(char* sql);
	
		MYSQL_RES* last_result;
		
	public:
		c_db_core();
		~c_db_core();
	
		bool init_by_str(char* in_host, char* in_name, char* in_user, char* in_passwd);
		//bool init_by_lua(c_lua_base* lua_base);
		//bool init_by_xml(TiXmlHandle* config_handle);		
	
		bool open();
		void end();
	
		bool query(const char* sql_line);
		bool query(char* sql_line, ...);
		bool query(c_db_result** in_result_var, char* sql_line, ...);
	
		void ping();
	
		int affected_rows();
		int last_insert_id();	
	
};


#endif
