#define PLUGIN_IMPLEMENT 1
#include <dlfcn.h>
#include <antd/plugin.h>

#include "includes/lua.h"
#include "includes/lualib.h"
#include "includes/lauxlib.h"

void init()
{
}
/**

* Plugin handler, reads request from the server and processes it
* 
*/
static void push_dict_to_lua(lua_State* L, dictionary_t d)
{
	lua_newtable(L);
	
	chain_t as;
	if(d)
		for_each_assoc(as, d)
		{
			lua_pushstring(L,as->key);
			//printf("KEY %s\n", as->key);
			if(EQU(as->key,"COOKIE") || EQU(as->key,"REQUEST_HEADER") || EQU(as->key,"REQUEST_DATA") )
				push_dict_to_lua(L, (dictionary_t)as->value);
			else
			{
				lua_pushstring(L,as->value);
				//printf("VALUE : %s\n",as->value );
			}
			lua_settable(L, -3);
		}
}
void* handle(void* data)
{
	antd_request_t* rq = (antd_request_t*) data;
	plugin_header_t* __plugin__ = meta();
	lua_State* L = NULL;
	//char * index = __s("%s/%s",__plugin__.htdocs,"router.lua");
	char * apis = __s("%s/lua/api.lua",__plugin__->pdir);
    char* cnf = __s("%s/lua/",__plugin__->pdir);
	L = luaL_newstate();
	luaL_openlibs(L);
    
	lua_newtable(L);
	lua_pushstring(L,"name");
	lua_pushstring(L, __plugin__->name);
	lua_settable(L,-3);
	
	lua_pushstring(L,"root");
	lua_pushstring(L, rq->client->port_config->htdocs);
	lua_settable(L,-3);
	
	lua_pushstring(L,"apiroot");
	lua_pushstring(L, cnf);
	lua_settable(L,-3);

	lua_pushstring(L,"tmpdir");
	lua_pushstring(L, tmpdir());
	lua_settable(L,-3);

	lua_pushstring(L,"dbpath");
	lua_pushstring(L, __plugin__->dbpath);
	lua_settable(L,-3);
	
	lua_setglobal(L, "__api__");
	
	// Request
	lua_newtable(L);
	lua_pushstring(L,"id");
	lua_pushlightuserdata(L, rq->client);
	//lua_pushnumber(L,client);
	lua_settable(L, -3);
	lua_pushstring(L,"request");
	push_dict_to_lua(L,rq->request);
	lua_settable(L, -3);
	lua_setglobal(L, "HTTP_REQUEST");
	
	// load major apis
	if(is_file(apis))
		if (luaL_loadfile(L, apis) || lua_pcall(L, 0, 0, 0))
		{
			ERROR( "cannot start API file: [%s] %s\n", apis, lua_tostring(L, -1));
		}
	
	/*if (luaL_loadfile(L, index) || lua_pcall(L, 0, 0, 0))
	{
		text(client);
		__t(client, "Cannot run router: %s", lua_tostring(L, -1));
	}
	free(index);*/
	// clear request
	if(L)
		lua_close(L);
	if(cnf)
		free(cnf);
	if(apis)
		free(apis);
	return antd_create_task(NULL, (void*)rq, NULL,rq->client->last_io);
	//lua_close(L);
}
void destroy()
{
}
