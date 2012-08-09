vc: vc.c vc.h msg.c
	cc -I/usr/local/include -o vc vc.c msg.c /usr/local/lib/liblua.a -ldl -lm -lreadline

