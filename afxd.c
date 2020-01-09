#define PLUGIN_IMPLEMENT 1
#include <dlfcn.h>
#include <antd/plugin.h>
#include <antd/ws.h>
#include <sys/select.h>
#include "includes/lua.h"
#include "includes/lualib.h"
#include "includes/lauxlib.h"

static int ready = 0;

void init()
{
	// check for dependencies
	if(require_plugin("lua"))
	{
		ready = 1;
		LOG("afxd loaded");
	}
	else
	{
		ERROR("[afxd]: cannot load dependencies: lua");
	}
}
/**

* Plugin handler, reads request from the server and processes it
* 

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
*/

void* process(void* data)
{
	antd_request_t* rq = (antd_request_t*) data;
	antd_task_t* task = antd_create_task(NULL, (void*)rq, NULL,rq->client->last_io);
	task->priority++;
	fd_set fd_in;
	int cl_fd = rq->client->sock;
	struct timeval timeout;      
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;
	// check if the connection is ok

	FD_ZERO(&fd_in);
	FD_SET(cl_fd, &fd_in);	

	lua_State* L = NULL;
	char buf[BUFFLEN];
	int rc = select(cl_fd + 1, &fd_in, NULL, NULL, &timeout);

	// stop
	if(rc == -1) // || (rc >0 && (recv(cl_fd, buf, 2, MSG_PEEK | MSG_DONTWAIT) == 0) ))
	{
		LOG("Socket closed, stop");
		return task;
	}

	// call lua script
	L = luaL_newstate();
	luaL_openlibs(L);
	
	lua_newtable(L);
	lua_pushstring(L,"name");
	lua_pushstring(L, __plugin__.name);
	lua_settable(L,-3);
	
	lua_pushstring(L,"root");
	htdocs(rq, buf);
	lua_pushstring(L, buf);
	lua_settable(L,-3);
	
	sprintf(buf, "%s/%s", __plugin__.pdir, __plugin__.name);

	lua_pushstring(L,"apiroot");
	lua_pushstring(L, buf);
	lua_settable(L,-3);

	lua_pushstring(L,"tmpdir");
	lua_pushstring(L, __plugin__.tmpdir);
	lua_settable(L,-3);

	lua_pushstring(L,"pdir");
	lua_pushstring(L, __plugin__.pdir);
	lua_settable(L,-3);

	lua_pushstring(L,"dbpath");
	lua_pushstring(L, __plugin__.dbpath);
	lua_settable(L,-3);
	
	lua_pushstring(L,"id");
	lua_pushlightuserdata(L, rq->client);
	lua_settable(L, -3);

	lua_pushstring(L,"pending");
	lua_pushboolean(L,rc > 0?1:0);
	lua_settable(L, -3);

	lua_setglobal(L, "__api__");

	// run entry point
	strcat(buf, "/afxd.lua");
	int error = 0;
	if (luaL_loadfile(L, buf) || lua_pcall(L, 0, 1, 0))
	{
		error = 1;
		ERROR( "cannot start API file: %s", lua_tostring(L, -1));
	}
	else
	{
		if(lua_isboolean(L,-1))
		{
			error = !lua_toboolean(L,-1);
		}
		else
		{
			ERROR("Lua script return is not boolean");
			error = 1;
		}
	}

	// return task
	if(L)
		lua_close(L);
	if(error)
	{
		ws_close(rq->client, 1011);
	}
	else
	{
		task->handle = process;
	}
	return task;
}

void* handle(void* data)
{
	antd_request_t* rq = (antd_request_t*) data;
	antd_task_t* task = antd_create_task(NULL, (void*)rq, NULL,rq->client->last_io);
	task->priority++;
	if(!ready)
	{
		antd_error(rq->client, 500, "Missing plugin(s) required to handle this request");
		return task;
	}

	if(!ws_enable( rq->request))
	{
		antd_error(rq->client, 400, "This service handles only websocket connections");
		return task;
	}

	free(task);
	return process(data);
}

void destroy()
{
}
