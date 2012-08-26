vc: vc.c vc.h msg.c commands.c
	cc -g -I/usr/local/include -o vc vc.c msg.c commands.c edid.c /usr/local/lib/liblua.a -ldl -lm -lreadline -Wall -Werror
clean:
	rm -f vc *.o
