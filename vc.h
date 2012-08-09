struct command {
	char *name;
	unsigned char athenacommand;
	int nargs;
	char *format;
	unsigned char ack;
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
