# -*- Makefile -*-

#format:
#target: dependencies
# [tab] action

.SILENT:

lab0:
	gcc -g -Wextra -Wall -o lab0 lab0.c

check:
	echo "hello world" > hello.txt;
	echo "please work" > work.txt;
	echo "how's it going" > ask.txt;
	touch testfile.txt;
	./lab0 --input=hello.txt > testfile.txt;
	if [ $$? -ne 0 ]; \
	then \
		echo "test1 failed"; \
		exit 5; \
	fi
	diff testfile.txt hello.txt;
	./lab0 --output=work.txt < hello.txt;
	if [ $$? -ne 0 ]; \
	then \
		echo "test2 failed"; \
		exit 5; \
	fi
	diff work.txt hello.txt;
	./lab0 --output=newfile.txt < hello.txt;
	if [ $$? -ne 0 ]; \
	then \
		echo "test3 failed"; \
		exit 5; \
	fi
	diff newfile.txt hello.txt;
	echo "please work" > work.txt;
	./lab0 --input=ask.txt --output=hello.txt;
	if [ $$? -ne 0 ]; \
        then \
                echo "test4 failed"; \
                exit 5; \
        fi
	diff ask.txt hello.txt;
	./lab0 --output=hello.txt --input=work.txt;
	if [ $$? -ne 0 ]; \
        then \
                echo "test5 failed"; \
                exit 5; \
        fi
	diff hello.txt work.txt;
	rm testfile.txt
	./lab0 --input=testfile.txt > /dev/null 2> /dev/null ; \
	if [ $$? -ne 1 ] ; \
	then \
		echo "test6 failed"; \
		exit 5 ; \
	fi
	chmod -r hello.txt
	./lab0 --input=hello.txt > /dev/null 2> /dev/null; \
	if [ $$? -ne 2 ]; \
	then \
		echo "test7 failed"; \
		exit 5; \
	fi
	chmod -w ask.txt;
	./lab0 --output=ask.txt > /dev/null 2> /dev/null; \
	if [ $$? -ne 3 ]; \
	then \
		echo "test8 failed"; \
		exit 5; \
	fi
	./lab0 --segfault --catch > /dev/null 2> /dev/null; \
	if [ $$? -ne 4 ]; \
	then \
		echo "test9 failed"; \
		exit 5; \
	fi
	echo "Smoke Test Passed";
	rm -f hello.txt work.txt ask.txt newfile.txt

clean:
	rm -f lab0 lab0-304784519.tar.gz

dist:
	tar -czf lab0-304784519.tar.gz lab0.c Makefile README backtrace.png breakpoint.png
