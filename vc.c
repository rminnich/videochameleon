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
	int failure;
	unsigned char result[256];
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
	lua_pushlstring(L, (const char *)&result[2], result[0]-2);
	return 2;                   /* number of results */
}

static int dumpedid (lua_State *L)
{
	extern void parse_edid(const unsigned char *);
	int n = lua_gettop(L);    /* number of arguments */
	/* one and only one arg: the command block */
	if (n != 1){
		lua_pushstring(L, "Need an EDID block\n");
		lua_error(L);
	}
	if (!lua_isstring(L, 1)) {
		lua_pushstring(L, "Arg must be a LUA string");
		lua_error(L);
	}
	parse_edid((const unsigned char *)lua_tostring(L, 1));
	lua_pushnumber(L, 0);
	lua_pushlstring(L, "", 0);
	return 2;                   /* number of results */
}

int main(int argc, char* argv[])
{
	int i;
	int failure;
	char *command;
	int senddebugon = 0;
	/* first set up hardware ... */
	Setup("/dev/ttyUSB0", &usbfd, &pipefd);
	/* not a fan of GNU arg processing. Can we do this somehow
	 * in LUA? Really, we need command timeouts ...
	 */
	for(i = 1; i < argc; i++){
		if (! strcmp(argv[i], "-d")) /* send debugon */
			senddebugon = 1;
	}

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
	lua_register(luaVM, "dumpedid", dumpedid);
	lua_register(luaVM, "help", help);

	if (senddebugon){
		failure = luaL_dostring(luaVM, "vc(\"debugon\")");
		printf("Debug Startup Command %s\n", failure? "NAK": "ACK" );
		if (failure)
			printf("It did not respond, continue at your own risk of frustration\n");
	}

	if (luaL_loadfile(luaVM, "vc.lua") || lua_pcall(luaVM, 0, 0, 0))
		printf("cannot run configuration file: %s",
		      lua_tostring(luaVM, -1));
	
	printf("Enter lua commands. type 'exit' to exit\n");
	using_history();
	read_history(VC_HISTORY_FILE);
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

	write_history(VC_HISTORY_FILE);
	
	return 0;
}
/* cc -o vc vc.c msg.c /usr/local/lib/liblua.a -ldl -lm -lreadline */

