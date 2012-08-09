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

struct command commands[] = {
	{"debugon", '\t', 0, NULL, '\f'},
};
/* I miss Go already */
int numcommands = sizeof(commands)/sizeof(commands[0]);

int
Command(const char *name, unsigned char *result, int usbfd, int pipefd, unsigned long args[], int nargs)
{
	int i;
	struct command *command;
	unsigned char msg[255];
	int msglen, amt;

	for(i = 0, command = NULL; i < numcommands && ! command; i++){
		if (! strcmp(commands[i].name, name))
			command = &commands[i];
	}

	if (! command)
		return -ENOENT;

	msg[0] = 2;
	msg[1] = command->athenacommand;

	for(i = 0; i < command->nargs; i++){
	}
	amt = SendMsg(usbfd, msg);
	if (amt < msg[0])
		return amt;

	/* we receive from the pipe. We are just scavenging what's left of debug prints */
	RecvMsg(pipefd, result);

	return result[1] == command->ack;
	
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
