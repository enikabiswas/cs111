# -*- Makefile -*-

.SILENT:

default: lab4b.c
	gcc -lm -lmraa -Wall -Wextra -o lab4b lab4b.c

check: default
	{ echo "STOP"; sleep 3; echo "START"; sleep 3; echo "OFF"; } | ./lab4b
	echo "passed 1"
	{ echo "PERIOD=2"; sleep 4; echo "SCALE=C"; sleep 2; echo "SCALE=F"; sleep 2; echo "OFF"; } | ./lab4b
	echo "passed 2"
	{ echo "STOP"; sleep 2; echo "LOG HELLO WORLD"; sleep 2; echo "START"; sleep 2; echo "LOG HELLO AGAIN WORLD"; sleep 2; echo "OFF"; } | ./lab4b --log=temp.txt
	echo "passed 3"
	{ printf "SCALE=C\nSCALE="; sleep 4; printf "F\n"; sleep 2; echo "OFF"; } | ./lab4b --log=temp.txt
	echo "passed 4"
	rm -f temp.txt

clean:
	rm -f lab4b lab4b-304784519.tar.gz

dist:
	tar -czf lab4b-304784519.tar.gz lab4b.c Makefile README
