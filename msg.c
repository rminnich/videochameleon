#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "vc.h"

char errstr[1024];
/* a little bit of crudity: we toss around the unsigned char[255] thing a lot in this file. 
 * I think it's ok: it's not visible outside this file, as it should not be; and C
 * is too primitive to let you do stuff like this:
 * typedef unsigned char[255] Message;
 * so screw it.
 */
int
SendMsg(int fd, char *msg)
{
	unsigned char csum;
	int amt, datalen, i;
	/* length is in the first byte. Since it's a byte, the only 
	 * invalid value is zero, meaning nothing to send.
	 */
	if (! msg[0])
		return 0;
	/* length has to include checksum */
	datalen = msg[0];
	msg[0]++;
	/* write it out and while that is happening ... */

	amt = write(fd, msg, datalen);
	if (amt < datalen){
		sprintf(errstr, "SendMsg: tried to send %d but only sent %d: %s", datalen, amt, strerror(errno));
		return amt;
	}
	for(i = 0; i < datalen; i++)
		csum += msg[i];

	csum = (~csum) + 1;
	/* maybe it will fail, but we'll find out later. */
	write(fd, &csum, 1);
}

/* note this works for both the usbfd and the pipefd -- we're using a filter model */
int
RecvMsg(int fd, char *msg)
{
	int amt, datalen, i;
	unsigned char csum;
	amt = read(fd, &msg[0], 1);
	if (amt < 0)
		sprintf(errstr, "RecvMsg: tried to read 1: %s", strerror(errno));
	if (amt == 0)
		return 0;

	amt = read(fd, &msg[1], msg[0]);
	if (amt < datalen){
		sprintf(errstr, "RecvMsg: tried to read %d, got %d:%s", msg[0], amt, strerror(errno));
		return 0;
	}

	datalen = msg[0]-1;
	/* length includes checksum */
	for(i = csum = 0; i < datalen; i++)
		csum += msg[i];

	if (csum + msg[datalen] != 0){
		sprintf(errstr, "CSUM fails: computed %02x message checksum %02x", csum, msg[datalen]);
		return 0;
	}
	
	return datalen;
}

/* print a message formatted in the bizarro format */
/* this is an asynchronous activity but it seems the easiest path. */
void
PrintMsg(char *msg)
{
	/* it starts at msg[2] */
	char *print = &msg[2];
	int printlen = strlen(print);
	char *data = &print[printlen+1];
	int index = 0;
	int formatting = 0;
	unsigned long val;

	/* the formatting variable helps guarantee we don't execute this loop with a NULL *print */
	while (*print){
		if (*print == '%'){
			formatting = 1;
			continue;
		}
		if (! formatting){
			printf("%c", *print);
			continue;
		}
		/* such fun we will have! */
		switch(*print++){
		case 's':
			printf(data);
			break;
		case 'l':
			val = data[index] << 24 | data[index+1] << 16 | data[index+2] << 8 | data[index+3];
			index += 4;
			printf("%04x", val);
			break;
		default: 
			val = data[index+2] << 8 | data[index+3];
			index += 2;
			printf("%02x", val);
			break;
		}
	}
	
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
			write(pipefd, msg, msg[0]);
			continue;
		}
		PrintMsg(msg);
	}
}

/* here's an example handler for commands which create data you want dumped. */
/* note you can put aliases for the same command in the table, and for some you can dump memory
 * and for others you can do something else, e.g. dumpedid and edid
 * it is assumed that if this is called, the ack was correct. Also, it is called with the 
 * result data, not including the length and response code (i.e. with &result[2]
 */
void dumpresult(struct command *c, unsigned long *args, unsigned char *result, int resultlen)
{
	int i, j;
	for(i = 0; i < resultlen; i += 16){
		printf("%08x:", args[0]+i);
		for(j = 0; j < 16 && i + j < resultlen; j++){
			printf(" %02x", result[i+j]);
		}
		printf("\n");
	}
}

/* this should probably be in a different file. */
/* there are only 256 commands. Just index the table by the command number. */
struct command commands[] = {
	['\t'] {"debugon", '\t', 0, NULL, '\f', "debugon", NULL},
	[0x11] {"param", 0x11, 1, "bi", 0x12, "param <param #> <value>", NULL},
	[0x52] {"rm", 0x52, 2, "ii", 'R', "read base length", dumpresult},
};

int
Command(const char *name, unsigned char *result, int usbfd, int pipefd, unsigned long args[], int nargs)
{
	int i;
	struct command *command;
	unsigned char msg[255], paramresult[255];
	int msglen, amt;
	int index;
	int paramfail;

	for(i = 0, command = NULL; i < 256 && ! command; i++){
		if (! commands[i].name)
			continue;
		if (! strcmp(commands[i].name, name))
			command = &commands[i];
	}

	if (! command)
		return -ENOENT;

	if (nargs != command->nargs){
		sprintf(errstr, "Usage: %s", command->usage);
		return -EINVAL;
	}

	msg[0] = 2;
	msg[1] = command->athenacommand;
	index = 2;

	for(i = 0; i < command->nargs; i++){
		switch(command->format[i]){
		case 'i':
			msg[index++] = args[i]>>24;
			msg[index++] = args[i]>>16;
			msg[index++] = args[i]>>8;
			msg[index++] = args[i];
			msg[0] += 4;
			break;
		case 'b':
			msg[index++] = args[i];
			msg[0]++;
			break;
		case 'p':
			paramfail = Command("param", paramresult, usbfd, pipefd, &args[i], 2);
			i += 2;
			if (paramfail)
				return paramfail;
			break;
		default: 
			sprintf(errstr, "Arg %d: bad format '%c': only b or i or p allowed", command->format[i]);
			return -EINVAL;
		}
	}
	amt = SendMsg(usbfd, msg);
	if (amt < msg[0])
		return amt;

	/* we receive from the pipe. We are just scavenging what's left of debug prints */
	/* note because this is structured as a filter (think shell) we get to use the same function
	 * with different fds.
	 */
	RecvMsg(pipefd, result);
	if (result[0] == command->ack){
		if (command->handler)
			command->handler(command, args, &result[2], result[0]-2);
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

	*ufd = open(serialport, O_RDWR);

	if (*ufd < 0)
		return -1;

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
