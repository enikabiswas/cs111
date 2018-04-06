# -*- Makefile -*-
#NAME: Kubilay Agi
#EMAIL: kubilayagi@g.ucla.edu
#ID: 304784519

.SILENT:

default:
	gcc -Wall -Wextra -g -lmraa -lm lab4c_tls lab4c.c
	gcc -Wall -Wextra -g -lmraa -lssl -lm -lcrypto lab4c_tcp lab4c.c

dist:
	tar -czf lab4c-304784519.tar.gz lab4c.c Makefile README

clean:
	rm -f lab4c-304784519.tar.gz lab4c