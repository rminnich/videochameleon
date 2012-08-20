#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <lualib.h>
#include <luaconf.h>
#include <lauxlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define VC_HISTORY_FILE "./.vchistory"

struct command {
	char *name; /* used by the user */
	char *athenacommand; /* for athena firmware  -- can by multiple bytes for apps*/
	int nargs; /* required number of args */
	char *argtypes; /* type for each arg: i for integer, s for "lua string" */
	char *format; /* how args are encoded into the packet -- b is byte, i is 4-byte big-endian integer */
	unsigned char ack; /* how the board acknowledges the command */
	char *usage; /* what you have to type */
	/* generic handler. You can leave it empty.*/
  void (*handler)(lua_State *L, const struct command *command, 
		  unsigned char *result, int resultlen);
};

/* msg.c */
int SendMsg(int fd, unsigned char *msg);
int RecvMsg(int fd, unsigned char *msg);
void PrintMsg(unsigned char *msg);
void ProcessMessages(int usbfd, int pipefd);
void dumpresult(lua_State *l, const struct command *c,  
		unsigned char *result, int resultlen);
void acknack(lua_State *l, const struct command *c, unsigned char ackornack);
int Command(lua_State *L, 
	    const char *name, unsigned char *result, int usbfd, int pipefd);
int Setup(char *serialport, int *ufd, int *pfd);
const struct command *findCommandByName(const char *name);

extern const struct command commands[];
extern const int numcommands;
/* errstr is a global, but the difference is it has useful info as opposed to that other global, errno */
extern char errstr[];

/* command defines */
#define PARAMOK 0xc
