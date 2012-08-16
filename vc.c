#include "vc.h"

static int usbfd = -1, pipefd = -1;

static void l_message (const char *pname, const char *msg) {
	if (pname) luai_writestringerror("%s: ", pname);
	luai_writestringerror("%s\n", msg);
}

static int help (lua_State *L)
{
	int n = lua_gettop(L);    /* number of arguments */
	const char *name = NULL;
	const struct command *command;

	if (n != 1) {
		lua_pushstring(L, "usage: help(\"cmd\")");
		lua_error(L);
	} else {
		name = lua_tostring(L, 1);
		command = findCommandByName(name);

		lua_pushstring(L, command->usage);
		lua_error(L);
	}
	return 1;
}

static int luacommand (lua_State *L)
{
	const char *command;
	int n = lua_gettop(L);    /* number of arguments */
	int i;
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
	failure = Command(L, command, result, usbfd, pipefd); 
	if (failure) {
		lua_pushstring(L, errstr);
		lua_error(L);
	}
	/* lua 'strings' include a null at the end -- or so the docs say */
	/* the math here is a bit confusing. We don't need to return 
	 * the response type (we think) in the result.
	 * So the length is too long by 1. But we do need to return
	 * a trailing null. That makes the length right again.
	 * Also, recall that result[length] points to the next byte
	 * past the end of the data -- handy!
	 * We need to put a null at the end of the string.
	 * That nicely fits together for us.
	 */
	result[result[0]] = 0;
	lua_pushnumber(L, result[1]);
	lua_pushlstring(L, (const char *)&result[2], result[0]);
	return 2;                   /* number of results */
}

int main(int argc, char* argv[])
{
	int failure;
	char *command;
	unsigned char result[255];
	/* first set up hardware ... */
	Setup("/dev/ttyUSB0", &usbfd, &pipefd);

	sleep(1);

	/* and make sure the thing stays active */
	lua_State *luaVM = luaL_newstate();  /* create state */
	if (luaVM == NULL) {
		l_message(argv[0], "cannot create state: not enough memory");
		return EXIT_FAILURE;
	}
	
	// initialize lua standard library functions
	luaL_openlibs(luaVM);
	lua_register(luaVM, "vc", luacommand);
	lua_register(luaVM, "help", help);
	printf("Simple Functional lua interpreter\n");
	printf("Based on lua version 4.0.1\n");
	printf("Registering Custom C++ Functions.\n");

	failure = Command(luaVM, "debugon", result, usbfd, pipefd);
	printf("Debug Startup Command %s\n", failure? "NAK": "ACK" );
	if (failure)
		printf("It did not respond, continue at your own risk of frustration\n");

	if (luaL_loadfile(luaVM, "vc.lua") || lua_pcall(luaVM, 0, 0, 0))
		printf("cannot run configuration file: %s",
		      lua_tostring(luaVM, -1));
	
	printf("Enter lua commands. type 'exit' to exit\n");
	while (1){
		int status;
       		command = readline ("A>");
		if (command && strlen(command) > 0)
			add_history(command);
		if (!command || !strcmp(command, "exit"))
			break;

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
	
	printf("Exiting...\n");
	lua_close(luaVM);
	
	return 0;
}
/* cc -o vc vc.c msg.c /usr/local/lib/liblua.a -ldl -lm -lreadline */

