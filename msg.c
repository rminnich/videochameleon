#include "vc.h"

char errstr[1024];
/* a little bit of crudity: we toss around the unsigned char[255] thing a lot in this file. 
 * I think it's ok: it's not visible outside this file, as it should not be; and C
 * is too primitive to let you do stuff like this:
 * typedef unsigned char[255] Message;
 * so screw it.
 */
int
SendMsg(int fd, unsigned char *msg)
{
	unsigned char csum = 0;
	int amt, datalen, i;
	/* length is in the first byte. Since it's a byte, the only 
	 * invalid value is zero, meaning nothing to send.
	 */
	if (! msg[0])
		return 0;
	/* length has to include checksum */
	datalen = msg[0];
	msg[0]++;
	for(i = 0; i < datalen; i++)
		csum += msg[i];

	msg[datalen] = (~csum) + 1;

	amt = write(fd, msg, msg[0]);
	if (amt < msg[0]){
		sprintf(errstr, "SendMsg: tried to send %d but only sent %d: %s", msg[0], amt, strerror(errno));
		return amt;
	}

	return amt;
}

/* note this works for both the usbfd and the pipefd -- we're using a filter model */
int
RecvMsg(int fd, unsigned char *msg)
{
	int amt, datalen, i, readamt;
	unsigned char csum;
	unsigned char *cp;
	amt = read(fd, &msg[0], 1);
	if (amt < 0)
		sprintf(errstr, "RecvMsg: tried to read 1: %s", strerror(errno));
	if (amt == 0)
		return 0;

	datalen = msg[0] - 1;
	cp = &msg[1];
	amt = 0;
	while (amt < datalen){
		readamt = read(fd, cp, datalen-amt);
		if (readamt < 0)
			break;
		amt += readamt;
		cp += readamt;
	}
	if (amt < datalen){
		sprintf(errstr, "RecvMsg: tried to read %d, got %d:%s", msg[0], amt, strerror(errno));
		return 0;
	}

	/* length includes checksum */
	for(i = csum = 0; i < datalen; i++)
		csum += msg[i];

	if (csum + msg[datalen] != 0){
		sprintf(errstr, "CSUM fails: computed %02x message checksum %02x", csum, msg[datalen]);
		return 0;
	}
	
	return datalen - 1;
}

unsigned int parseInt(char **data, int numBytes)
{
	int j;
	unsigned long val = 0;
	for (j = 0; j < numBytes; j++)
		val = (val << 8) + *(*data)++;
	return val;
}

/* print a message formatted in the bizarro format */
/* this is an asynchronous activity but it seems the easiest path. */
void
PrintMsg(unsigned char *msg)
{
	/* it starts at msg[2] */
	char *print = (char *)&msg[2];
	int printlen = strlen(print);
	char *data = &print[printlen+1];
	int i, formatting = 0, val;

	val = 0;
	printf(">>>");
	for (i = 0; i < printlen; ++i) {
		if (!formatting) {
			if (print[i] == '%')
				formatting = 1;
			else
				printf("%c", print[i]);
		} else {
			switch(tolower(print[i])) {
			case 'd':
				printf("%u", val + parseInt(&data, 2));
				break;
			case 'x':
				printf("%04x", val + parseInt(&data, 2));
				break;
			case 'c':
				printf("%c", *data++);
				break;
			case 'l':
				val = parseInt(&data, 2) << 16;
				continue;
				break;
			case 's':
				printf("%s", data);
				data = data + strlen(data);
				break;
			default:
				printf("<<<Broken Formatter:%%%c>>>", print[i]);
				break;
			}
			val = 0;
			formatting = 0;
		}
	}
	printf("\n");
}

/* suck in messages. If they are just informational, print them out.
 * if they are responses to commands, push them up the pipe. 
 */
void
ProcessMessages(int usbfd, int pipefd)
{
	unsigned char msg[255];
	while (1){
		RecvMsg(usbfd, msg);
		if (msg[1] != '\r'){
			msg[0]--;
			write(pipefd, msg, msg[0]);
		} else {
			PrintMsg(msg);
		}
		
	}
}

/* here's an example handler for commands which create data you want dumped. */
/* note you can put aliases for the same command in the table, and for some you can dump memory
 * and for others you can do something else, e.g. dumpedid and edid
 * it is assumed that if this is called, the ack was correct. Also, it is called with the 
 * result data, not including the length and response code (i.e. with &result[2]
 */
void dumpresult(lua_State *L,
		const struct command *c,
		unsigned char *result, int resultlen)
{
	int i, j;
	/* The first arg is the base address */
	unsigned int base = lua_tonumber(L, 0);
	for(i = 0; i < resultlen; i += 16){
		printf("%08x:", base+i);
		for(j = 0; j < 16 && i + j < resultlen; j++){
			printf(" %02x", result[i+j]);
		}
		printf("\n");
	}
}

void acknack(lua_State *L, const struct command *c, unsigned char ackornack)
{
	if (ackornack == c->ack)
		printf("ACK!\n");
	else
		printf("NACK!\n");
}

/**
 * Given the name of a command, returns you a pointer to the command spec
 *
 * @param name		The name of the command to find
 * @return A pointer to the spec for the command, or NULL if there's an error
 */
const struct command *findCommandByName(const char *name)
{
	int i;

	if (!name)
		return NULL;

	for(i = 0; i < numcommands; i++){
		if (!commands[i].name)
			continue;
		if (!strcmp(commands[i].name, name))
			return &commands[i];
	}
	return NULL;
}

/**
 * Confirm that the arguments for a command have been set up correctly
 *
 * @param L		The lua state to check the command structure in
 * @param command	A pointer to the command spec to check against
 * @return 0 on success, or the index of the offending argument
 */
int checkCommandFormat(lua_State *L, const struct command *command)
{
	int i;
	char err[128];

	for (i = 0; i < command->nargs; i++) {
		switch(command->argtypes[i]){
		case 's': 
			if (lua_isstring(L, i + 2))
				continue;
			sprintf(err, "arg %d: must be a number", i + 2);
			break;
		case 'i':
			if (lua_isnumber(L, i + 2))
				continue;
			sprintf(err, "arg %d: must be a number", i + 2);
			break;
		default:
			sprintf(err, "arg %d is not a string or number", i + 2);
			break;
		}

		lua_pushstring(L, err);
		lua_error(L);
		return i + 2;
	}
	return 0;
}

/**
 * Builds a message in the msg buffer to send to the Athena board using the
 * command spec and the data in L.
 *
 * @param L		The lua state to draw the parameters from
 * @param command	A pointer to the command spec to use
 * @param msg		A buffer to store the message in
 * @return 0 on success, or an error code
 */
int buildCommandMessage(lua_State *L, const struct command* command, unsigned char *msg, int usbfd, int pipefd) {
	int cmdlen, i, index;
	unsigned int val, len;
	char paramcommand[32];
	int paramfail;
	int paramindex = 1;
	const char *cp;

	if (!msg)
		return -1;

	msg[0] = 1;
	cmdlen = strlen(command->athenacommand);
	memcpy(&msg[1], command->athenacommand, cmdlen);
	msg[0] += cmdlen;
	index = 1 + cmdlen;

	for(i = 0; i < command->nargs; i++){
		switch(command->format[i]){
		case 'i':
			val = lua_tonumber(L, i + 2);
			msg[index++] = val>>24;
			msg[index++] = val>>16;
			msg[index++] = val>>8;
			msg[index++] = val;
			msg[0] += 4;
			break;
		case 'b':
			val = lua_tonumber(L, i + 2);
			msg[index++] = val;
			msg[0]++;
			break;
		case 'p':
			/* params are sent as separate commands */
			sprintf(paramcommand, "vc(\"param\", %d, %d)", paramindex++, 
				(int)lua_tonumber(L, i + 2));
			paramfail = luaL_dostring(L, paramcommand);
			if (paramfail)
				return paramfail;
			break;
		/* the following are for variable length entities */
		/* it is an error for the length to be > 253 */
		case 's':
			/* push the length of the string, then the string */
			cp = lua_tostring(L, i + 2);
			len = lua_rawlen(L, i + 2);
			if (! cp){
				sprintf(errstr,
					"Arg %d is not a string", i);
				return -EINVAL;
			}
			if (index + len > 253){
				sprintf(errstr,
					"String at arg %d len %d too large",
					i, len);
				return -EINVAL;
			}
			msg[index++] = len;
			memmove(&msg[index], cp, len);
			index += len;
			msg[0] += 1 + len;
			break;
		default: 
			sprintf(errstr, "Arg %d: bad format '%c': only b or i or p or s allowed", i, command->format[i]);
			return -EINVAL;
		}
	}
	return 0;
}

int
Command(lua_State *L, const char *name, unsigned char *result, int usbfd, int pipefd)
{
	int rv;
	const struct command *command;
	unsigned char msg[255];
	unsigned int amt;

	command = findCommandByName(name);
	if (!command)
		return -ENOENT;

	rv = checkCommandFormat(L, command);
	if (rv) {
		sprintf(errstr, "Usage: %s", command->usage);
		return -EINVAL;
	}

	rv = buildCommandMessage(L, command, msg, usbfd, pipefd);
	amt = SendMsg(usbfd, msg);
	if (amt < msg[0])
		return -errno;

	/* we receive from the pipe. We are just scavenging what's left of debug prints */
	/* note because this is structured as a filter (think shell) we get to use the same function
	 * with different fds.
	 */
	RecvMsg(pipefd, result);
	if (0) {
		int i, j;
		unsigned char* msg = result;
		int datalen = msg[0]; 
		printf("Received: ");
		for(i = 0; i < msg[0]; i += 16){
		for(j = 0; j < 16 && i + j < datalen; j++){
			printf(" %02x", msg[i+j]);
		}
		printf("\n");
	}}

	acknack(L, command, result[1]);

	if (result[1] == command->ack){
		if (command->handler)
			command->handler(L, command, &result[2], result[0] - 2);
	} else {
		sprintf(errstr, "%s command received wrong reply type: expected %02x, got %02x", 
			command->name, command->ack, result[0]);
	}
	return result[1] != command->ack;
}

/* we could do pthreads or some such awful thing, but why bother? Pipes are great for synchronization and data
 * transmision for something this simple.
 */
int
Setup(char *serialport, int *ufd, int *pfd)
{
	int pipefd[2];
	int pid;
	struct termios newtio;

	*ufd = open(serialport, O_RDWR);

	if (*ufd < 0)
		return -1;

	tcgetattr(*ufd, &newtio);
	cfmakeraw(&newtio);
	cfsetspeed(&newtio, B115200);
	newtio.c_cflag &= ~CRTSCTS;
	tcsetattr(*ufd, TCSANOW, &newtio);

	if (pipe(pipefd) < 0){
		close(*ufd);
		return -1;
	}

	pid = fork();

	if (pid < 0)
		return -1;

	if (! pid){
		/* we only ever write */
		close(pipefd[0]);
		ProcessMessages(*ufd, pipefd[1]);
		exit(1);
	}

	*pfd = pipefd[0];
	/* we only ever read */
	close(pipefd[1]);
	return 0;
}

/* test */
#ifdef TESTING
int main(int argc, char *argv[])
{
	int pipefd, usbfd;
	unsigned char result[255];
	int success;

	Setup("/dev/ttyUSB0", &usbfd, &pipefd);
	success = Command("debug", result, usbfd, pipefd, NULL, 0);
	printf("Command %s\n", success ? "ACK" : "NACK");

	return 0;
}
#endif

/* for lua. */
