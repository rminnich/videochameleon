struct command {
	char *name; /* used by the user */
	unsigned char athenacommand; /* for athena firmware */
	int nargs; /* required number of args */
	char *format; /* how args are encoded into the packet -- b is byte, i is 4-byte big-endian integer */
	unsigned char ack; /* how the board acknowledges the command */
	char *usage; /* what you have to type */
	/* generic handler. You can leave it empty.*/
	void (*handler)(struct command *command, unsigned long *args, unsigned char *result, int resultlen);
};

/* msg.c */
int SendMsg(int fd, char *msg);
int RecvMsg(int fd, char *msg);
void PrintMsg(char *msg);
void ProcessMessages(int usbfd, int pipefd);
int Command(const char *name, unsigned char *result, int usbfd, int pipefd, unsigned long args[], int nargs);
int SetUp(char *serialport, int *ufd, int *pfd);
extern struct command commands[];
extern int numcommands;
/* errstr is a global, but the difference is it has useful info as opposed to that other global, errno */
extern char errstr[];

/* command defines */
#define PARAMOK 0xc
