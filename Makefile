vc: vc.c vc.h msg.c commands.c
	cc -I/usr/local/include -o vc vc.c msg.c commands.c /usr/local/lib/liblua.a -ldl -lm -lreadline -Wall -Werror

