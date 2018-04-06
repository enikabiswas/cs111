#-*- Makefile -*-
.SILENT:

default: lab3a.c
	gcc -lm -Wall -Wextra -g -o lab3a lab3a.c

dist: lab3a.c README
	tar -czf lab3a-304784519.tar.gz lab3a.c ext2_fs.h README Makefile

clean:
	rm -f lab3a-304784519.tar.gz lab3a