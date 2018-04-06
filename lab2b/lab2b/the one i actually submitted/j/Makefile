#-*- Makefile -*-
.SILENT:

build:
	gcc -pthread -Wall -Wextra lab2_list.c SortedList.c -o lab2_list

clean:
	rm -f lab2_list lab2b-304784519.tar.gz

graphs: tests
	gnuplot lab2_list.gp

dist:	graphs
	tar -czf lab2b-304784519.tar.gz lab2_list.c SortedList.h SortedList.c Makefile README lab2b_list.csv profile.out *.png

#profile:

tests: build
	-./lab2_list --threads=1 --iterations=1000 --sync=m >> lab2b_list.csv
	-./lab2_list --threads=2 --iterations=1000 --sync=m >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=1000 --sync=m >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=1000 --sync=m >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=1000  --sync=m >> lab2b_list.csv
	-./lab2_list --threads=24 --iterations=1000 --sync=m >> lab2b_list.csv

	-./lab2_list --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=1000 --sync=s >> lab2b_list.csv
	-./lab2_list --threads=24 --iterations=1000 --sync=s >> lab2b_list.csv
