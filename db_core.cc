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

#include "defines.hh"
#include "db_core.hh"


// TODO: mehr Fehlerbehandlung!

c_db_result::c_db_result(MYSQL_RES* in_result_id) {
	this->result_id = in_result_id;
}

c_db_result::~c_db_result() {
	mysql_free_result(this->result_id);
}

bool c_db_result::fetch_row() {
	this->row = mysql_fetch_row(this->result_id);
	
	return (this->row != NULL);
}

my_ulonglong c_db_result::num_rows() {
	return mysql_num_rows(this->result_id);
}


c_db_core::c_db_core() {
	this->state = DBSTATE_UNCONNECTED;
	
	mysql_init(&this->mysql);
	
	this->db_host = this->db_name = this->db_user = this->db_passwd = NULL;
	
	this->last_result = NULL;
}

c_db_core::~c_db_core() {
	if(this->state == DBSTATE_CONNECTED) this->end();
		
	safe_delete_array(this->db_host);
	safe_delete_array(this->db_name);
	safe_delete_array(this->db_user);
	safe_delete_array(this->db_passwd);
}

bool c_db_core::raise_error(char* sql) {
	if(sql == NULL) {
		DEBUG_LOG("c_db_core: (%i) %s\n", mysql_errno(&this->mysql), mysql_error(&this->mysql));
	}
	else {
		DEBUG_LOG("c_db_core->query: at \"%s\" raised:\n(%i) %s\n", sql, mysql_errno(&this->mysql), mysql_error(&this->mysql));
	}
	
	return false;
}

bool c_db_core::init_by_str(char* in_host, char* in_name, char* in_user, char* in_passwd) {
	this->db_host = new char[strlen(in_host) + 1];
	strcpy(this->db_host, in_host);
	
	if(in_name != NULL) {
		this->db_name = new char[strlen(in_name) + 1];
		strcpy(this->db_name, in_name);
	}
	
	this->db_user = new char[strlen(in_user) + 1];
	strcpy(this->db_user, in_user);
	
	if(in_passwd != NULL) {
		this->db_passwd = new char[strlen(in_passwd) + 1];
		strcpy(this->db_passwd, in_passwd);
	}
	
	return true;
}

/*
bool c_db_core::init_by_lua(c_lua_base* lua_base) {
	const char* config_str;
	
	if(!(config_str = lua_base->get_global_string("db_host"))) {
		LOG_ERROR("c_db_core->init_by_lua: could not find valid host variable\n");
		
		return false;
	}
	
	this->db_host = new char[strlen(config_str) + 1];
	strcpy(this->db_host, config_str);
	
	if((config_str = lua_base->get_global_string("db_name"))) {
		this->db_name = new char[strlen(config_str) + 1];
		strcpy(this->db_name, config_str);	
	}
	
	if(!(config_str = lua_base->get_global_string("db_user"))) {
		LOG_ERROR("c_db_core->init_by_lua: could not find valid user variable\n");
		
		return false;
	}
	
	this->db_user = new char[strlen(config_str) + 1];
	strcpy(this->db_user, config_str);
	
	if((config_str = lua_base->get_global_string("db_passwd"))) {
		this->db_passwd = new char[strlen(config_str) + 1];
		strcpy(this->db_passwd, config_str);	
	}	
	
	return true;
}

bool c_db_core::init_by_xml(TiXmlHandle* config_handle) {
	TiXmlElement* db_section = config_handle->FirstChild("database").Element();
	
	if(!db_section) {
		LOG_ERROR("c_db_core->init_by_xml: could not find database config section\n");
		
		return false;
	}
	
	TiXmlElement* config_element;
	const char* config_str;
	
	if(!(config_element = db_section->FirstChildElement("host"))) {
		LOG_ERROR("c_db_core->init_by_xml: could not find host element in database config section\n");
		
		return false;
	}
		
	if(!(config_str = config_element->GetText())) {
		LOG_ERROR("c_db_core->init_by_xml: empty host element in database config section\n");
		
		return false;
	}
	
	this->db_host = new char[strlen(config_str) + 1];
	strcpy(this->db_host, config_str);
	
	if((config_element = db_section->FirstChildElement("name"))) {
		if((config_str = config_element->GetText())) {
			this->db_name = new char[strlen(config_str) + 1];
			strcpy(this->db_name, config_str);
		}
	}
	
	if(!(config_element = db_section->FirstChildElement("user"))) {
		LOG_ERROR("c_db_core->init_by_xml: could not find user element in database config section\n");
		
		return false;
	}
	
	if(!(config_str = config_element->GetText())) {
		LOG_ERROR("c_db_core->init_by_xml: empty user element in database config section\n");
		
		return false;
	}
	
	this->db_user = new char[strlen(config_str) + 1];
	strcpy(this->db_user, config_str);	
	
	if((config_element = db_section->FirstChildElement("passwd"))) {
		if((config_str = config_element->GetText())) {
			this->db_passwd = new char[strlen(config_str) + 1];
			strcpy(this->db_passwd, config_str);
		}
	}
	
	return true;
}
*/

bool c_db_core::open() {
	if(this->state == DBSTATE_CONNECTED) return true;
		
	if(mysql_real_connect(&this->mysql, this->db_host, this->db_user, this->db_passwd, this->db_name, 0, NULL, 0) != NULL) {
		this->state = DBSTATE_CONNECTED;
		
		return true;
	}
	else {
		return this->raise_error(NULL);
	}		
}

void c_db_core::end() {
	if(this->state == DBSTATE_UNCONNECTED) return;
		
	mysql_close(&this->mysql);
}

bool c_db_core::query(const char* sql_line) {
	if(this->state == DBSTATE_UNCONNECTED) {
		if(!this->open()) return false;
	}	
	
	if(mysql_real_query(&this->mysql, sql_line, strlen(sql_line)) != 0) {
		return this->raise_error((char*) sql_line);
	}
	
	return true;
}

bool c_db_core::query(char* sql_line, ...) {
	if(this->state == DBSTATE_UNCONNECTED) {
		if(!this->open()) return false;
	}
	
	int n_chars, buf_len = 128;
	char* buf = NULL;
	va_list arg_pointer;	
		
	while(1) {
		buf = new char[buf_len];
		
		va_start(arg_pointer, sql_line);
		
		n_chars = vsnprintf(buf, buf_len, sql_line, arg_pointer);

		va_end(arg_pointer);		
		
		if( (n_chars > -1) && (n_chars < buf_len) ) break;
		
		if(n_chars > -1) buf_len = n_chars + 1;
		else buf_len *= 2;
		
		safe_delete_array(buf);
	}
		
	if(mysql_real_query(&this->mysql, buf, strlen(buf)) != 0) {
		return this->raise_error(buf);
	}
	
	safe_delete_array(buf);	
	
	return true;
}

bool c_db_core::query(c_db_result** in_result_var, char* sql_line, ...) {
	if(this->state == DBSTATE_UNCONNECTED) {
		if(!this->open()) return false;
	}
	
	int n_chars, buf_len = 128;
	char* buf = NULL;
	va_list arg_pointer;	
		
	while(1) {
		buf = new char[buf_len];
		
		va_start(arg_pointer, sql_line);
		
		n_chars = vsnprintf(buf, buf_len, sql_line, arg_pointer);

		va_end(arg_pointer);		
		
		if( (n_chars > -1) && (n_chars < buf_len) ) break;
		
		if(n_chars > -1) buf_len = n_chars + 1;
		else buf_len *= 2;
		
		safe_delete_array(buf);
	}
		
	if(mysql_real_query(&this->mysql, buf, strlen(buf)) != 0) {
		return this->raise_error(buf);
	}
	
	safe_delete_array(buf);	
	
	MYSQL_RES* result = mysql_store_result(&this->mysql);
	
	if(result == NULL) {
		return this->raise_error(NULL);
	}
	
	*in_result_var = new c_db_result(result);
	
	return true;
}



void c_db_core::ping() {
	if(this->state == DBSTATE_CONNECTED) {
		mysql_ping(&this->mysql);
	}
}

int c_db_core::affected_rows() {
	return mysql_affected_rows(&this->mysql);
}

int c_db_core::last_insert_id() {
	return mysql_insert_id(&this->mysql);
}
