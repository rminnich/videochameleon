#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <lualib.h>
#include <lauxlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "vc.h"

static int usbfd = -1, pipefd = -1;

static void l_message (const char *pname, const char *msg) {
  if (pname) luai_writestringerror("%s: ", pname);
  luai_writestringerror("%s\n", msg);
}
static int luacommand (lua_State *L) {
	unsigned long args[8];
	unsigned const char *command;
	int n = lua_gettop(L);    /* number of arguments */
	lua_Number sum = 0;
	int i;
	int nargs = 0;
	int failure;
	unsigned char result[256];
	for(i = 1; i <= n; i++)printf("%d: %s\n", i, lua_tostring(L, i));
	/* we at least need the command name */
	if (n < 1){
		lua_pushstring(L, "Need at least a command name!");
		lua_error(L);
	}
	if (!lua_isstring(L, 1)) {
		lua_pushstring(L, "First arg must be a string");
		lua_error(L);
	}
	command = lua_tostring(L, 1);
	for (i = 2; i <= n; i++) {
		if (!lua_isnumber(L, i)) {
			lua_pushstring(L, "incorrect argument: must be a number");
			lua_error(L);
		}
		args[i-2] = lua_tonumber(L, i);
		nargs++;
	}

	failure = Command(command, result, usbfd, pipefd, args, nargs); 
	if (failure) {
		lua_pushstring(L, errstr);
		lua_error(L);
	}
	/* future: consider pushing succcess/failure, ack value, and resulting array */
	/* lua is multivalue return ... */
	lua_pushnumber(L, failure);        /* first result */
	return 1;                   /* number of results */
}

int main(int argc, char* argv[])
{
	int success;
	char *command;
	unsigned char result[2];
	/* first set up hardware ... */
	Setup("/dev/ttyUSB0", &usbfd, &pipefd);
	/* and make sure the thing stays active */
	success = Command("debug", result, usbfd, pipefd, NULL, 0);
	printf("Debug Startup Command %s\n", success > 0? "ACK" : "NACK");
	if (! success)
		printf("It did not respond, continue at your own risk of frustration\n");

	lua_State *luaVM = luaL_newstate();  /* create state */
	if (luaVM == NULL) {
		l_message(argv[0], "cannot create state: not enough memory");
		return EXIT_FAILURE;
	}
	
	// initialize lua standard library functions
	luaL_openlibs(luaVM);
	lua_register(luaVM, "vc", luacommand);
	printf("Simple Functional lua interpreter\n");
	printf("Based on lua version 4.0.1\n");
	printf("Registering Custom C++ Functions.\n");

	
	printf("Enter lua commands. type 'exit' to exit\n");
	while (1){
		int status;
       		command = readline ("A>");
		status = luaL_dostring(luaVM, command);
		if (status != LUA_OK && !lua_isnil(luaVM, -1)) {
			const char *msg = lua_tostring(luaVM, -1);
			if (msg == NULL) msg = "(error object is not a string)";
			l_message(NULL, msg);
			errstr[0] = 0;
			lua_pop(luaVM, 1);
			/* force a complete garbage collection in case of errors */
			lua_gc(luaVM, LUA_GCCOLLECT, 0);
		}
	}
	
	lua_close(luaVM);
	
	return 0;
}
/* cc -o vc vc.c msg.c /usr/local/lib/liblua.a -ldl -lm -lreadline */

