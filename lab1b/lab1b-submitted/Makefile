# -*- Makefile -*-

.SILENT:

default:
	gcc -lz -Wall -Wextra lab1b-server.c -o lab1b-server
	gcc -lz -Wall -Wextra lab1b-client.c -o lab1b-client
clean:
	rm -f lab1b-304784519.tar.gz lab1b-server lab1b-client
dist:
	tar -czf lab1b-304784519.tar.gz lab1b-server.c lab1b-client.c Makefile README
