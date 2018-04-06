#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void freeMe(char* ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
	//check for null bc otherwise we get seg fault, but not the one we want to cause
}

void segfault(void) {
	char *c = NULL;
	*c = 'a';
}

void readAndWrite(void) {
	int buffersize = 2000;
	char textin[buffersize+1];
	int numbytes = 0;
	while ((numbytes = read(0, &textin, buffersize)) > 0) {
		write(1, &textin, numbytes);
	} 
}
/*
char* readIn(int filedes) {
	int bufsize = 2001;
	char *textin = (char*) malloc(sizeof(char) * bufsize);
	int bytesread = 1;
	char* ptr = textin;
	while (bytesread > 0) {
		bytesread = read(filedes, ptr, 2000);
		ptr += bytesread;
		bufsize *= 2;
		textin = realloc(textin, sizeof(char) * bufsize);
	}
	return textin;
}

void writeOut(char* inputText) {
	int size = strlen(inputText);
	write(1, inputText, size);
}
*/

void sighandler(int signum) {
	char message[] = "Segmentation fault caught by --catch flag\n";
	int size = strlen(message);
	if (signum == SIGSEGV) {
		write(2, message, size);
	}
	exit(4);
}


int main (int argc, char **argv) {
	
	int input_flag = 0;
	int output_flag = 0;
	int segfault_flag = 0;
	int catch_flag = 0;

	int c; //this will be i, o, s, or c depending on the argument given

	//these two arrays are for holding the filenames so we can use them later
	char *readfile = NULL;
	char *writefile = NULL;
	
	while (1) { //want to get all of the options

		static struct option long_options[] = {
			{"segfault", no_argument, 0, 's'},
                        {"catch", no_argument, 0, 'c'},
			{"input", required_argument, 0, 'i'},
			{"output", required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		//code wouldn't work unless this was declared
		//in the scope of the while loop

		c = getopt_long(argc, argv, "sci:o:", long_options, &option_index);
		//based off an example from gnu.org, link included in README file
		
		if (c == -1) {
			break;
		} 
		//breaks after getting all options
		//once there are no more options, getopt_long returns -1
		//since the while loop is infinite, we need to end it somehow

		//these values are used when we are allocating memory for the filename arrays
		int inputNameLen = 0;
		int outputNameLen = 0;
		
		switch (c)
			{
			case 'i':
				//printf("%s\n", optarg);
				inputNameLen = strlen(optarg)+1; //+1 for the null byte
				readfile = malloc(sizeof(char) * inputNameLen);
				strcpy(readfile, optarg);
				input_flag = 1;
				break;
			case 'o':
				outputNameLen = strlen(optarg)+1;
				writefile = malloc(sizeof(char) * outputNameLen);
				strcpy(writefile, optarg);
				output_flag = 1;
				break;
			case 's':
				segfault_flag = 1;
				break;
			case 'c':
				catch_flag = 1;
				break;
			default:
				exit(1);
				break;
			}//end switch case
	}
        
	//*** handling the --input flag

	if (input_flag) {
		//open target file
		int fd = 0;
		fd = open(readfile, O_RDONLY);
		if (fd >=  0) {
			close(0);
			dup(fd);
			close(fd);
		}
		else {
			fprintf(stderr, "Error with --input option. Could not open '%s' because: %s.\n", readfile, strerror(errno));
			if (errno == 13) {
				fprintf(stderr, "Exiting with code 2\n");
				exit(2);
			}
			else {
				fprintf(stderr, "Exiting with code 1\n");
				exit(1);
			}
		}
	}


	//*** handling the --output flag
	
	if (output_flag) {
		//open target file or create a new one if it doesn't exist
		int fd = 1;
		fd = creat(writefile, 0666);
		if (fd >= 0) {
			close(1);
			dup(fd);
			close(fd);
		}
		else {
			fprintf(stderr, "Error with --output option. Unable to open '%s' because: %s.\nExit status 3\n", writefile, strerror(errno));
			exit(3);
		}
	}
	
	if (catch_flag) {
		signal(SIGSEGV, sighandler);
	}

	if (segfault_flag) {
		segfault();
	}
	

	readAndWrite();
	//was having problems reading in the whole file first, and then writing it
	//all out to STDOUT. Probably was allocating too much memory? It is easier
	//to read and write in chunks anyways.


	//	inputText = readIn(0); //0 is the file descriptor for any input b/c of dup()
	//	writeOut(inputText); //1 is file des for any output b/c of dup()

	//freeing the dynamically allocated char arrays using my own function

	freeMe(readfile);
	freeMe(writefile);

	//success
	exit(0);
}
