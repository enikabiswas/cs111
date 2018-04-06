#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <getopt.h>
#include <netdb.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "zlib.h"

#define BUFSIZE 1024
#define USAGE "usage: ./lab1b-server --port=port#\n"
#define SENT 1
#define RECEIVED 2

//option flags
static int portflag = 0;
static int compressflag = 0;
static int logflag = 0;

//socket fd
static int sockfd = -1;

//port number
static int portnum = -1;

//log file name
char filename[BUFSIZE];

//log file descriptor
int logfd = -1;

//terminal setting structs
struct termios attr_orig;
struct termios attr_new;

//poll file descriptors
struct pollfd pollfds[2];


void errorcheck(int ret, char *calledfunc, char *outerfunc) {
	if (ret < 0) {
		fprintf(stderr, "error calling the %s function in the %s routine\n", calledfunc, outerfunc);
		exit(1);
	}
}

void sighandler(int signum) {
	if (signum == SIGPIPE) {
		exit(0);
	}
	if (signum == SIGINT) {
		exit(1);
	}
}

void getOptions(int argc, char *argv[]) {
	int c = 0;
        while(1) {
                if (c == -1) {
                        break;
                }
                static struct option long_options[] = {
                        {"port", required_argument, 0, 'p'},
						{"compress", no_argument, 0, 'c'},
						{"log", required_argument, 0, 'l'},
                        {0, 0, 0, 0}
                };
                int option_index = 0;
                c = getopt_long(argc, argv, "clp:", long_options, &option_index);
                if (c == -1) {
                        break;
                }
                switch(c) {
                case 'p':
                    portflag = 1;
					portnum = atoi(optarg);
                    break;
				case 'c':
					compressflag = 1;
					break;
				case 'l':
					logflag = 1;
					strcpy(filename, optarg);
					//open file if it exists, or make a new one
					logfd = creat(filename, 0666);
					errorcheck(logfd, "creat", "main");
					break;
                default:
                        fprintf(stderr, "unrecognized argument\n");
                        fprintf(stderr, "%s\n", USAGE);
                        exit(1);
                        break;
                }
        }
	if (portflag == -1) {
		fprintf(stderr, "--port is a required argument.\n");
		fprintf(stderr, "%s\n", USAGE);
		exit(1);
	}
}

void shutdown_process(void) {

	if (logflag) {
		close(logfd);
	}	
	int setcheck = tcsetattr(0, TCSANOW, &attr_orig);
	if (setcheck == -1) {
		fprintf(stderr, "tcsetattr returned -1! Terminal settings were not reset\n");
		fprintf(stderr, "To fix this type: stty sane^J\n");
	}
	close(sockfd);
}

void setupSocket(void) {
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	errorcheck(sockfd, "socket", "setupSocket");
	//TODO: figure out what the absolute fuck this is supposed to be what the fuck
	server = gethostbyname("localhost");
	if (server == NULL) {
		errorcheck(-1, "gethostbyname", "setupSocket");
	}
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portnum);
	int check = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	errorcheck(check, "connect", "setupSocket");
}

void updateLog(char *buf, int numbytes, int action) {
	char message[2048];
	if (action == SENT) {
		sprintf(message, "SENT %d bytes: %s\n", numbytes, buf);
		int size = strlen(message);
		write(logfd, message, size);
	}
	else if (action == RECEIVED) {
		sprintf(message, "RECEIVED %d bytes: %s\n", numbytes, buf);
		int size = strlen(message);
		write(logfd, message, size);
	}
}

char* def(char* to_deflate) {
	//TODO: FREE THIS MEMORY
	char *deflated = (char*) malloc(sizeof(char) * BUFSIZE);
	z_stream client_to_server;

	client_to_server.zalloc = Z_NULL;
	client_to_server.zfree = Z_NULL;
	client_to_server.opaque = Z_NULL;

	client_to_server.avail_in = (uInt) strlen(to_deflate) + 1; //to include null byte
	client_to_server.next_in = (Bytef *) to_deflate;
	client_to_server.avail_out = (uInt) BUFSIZE;
	client_to_server.next_out = (Bytef *) deflated;

	deflateInit(&client_to_server, Z_DEFAULT_COMPRESSION);
	do {
		deflate(&client_to_server, Z_SYNC_FLUSH);
	} while(client_to_server.avail_in > 0);

	deflateEnd(&client_to_server);

	return deflated;
}

int comp_size(char* compd) {
	int size = -1;
	char checksize[BUFSIZE+10];
	size = sprintf(checksize, "%s", compd);
	return size;
}

char* inf(char* to_inflate) {
	//TODO: FREE THIS MEMORY
	char *inflated = (char*) malloc(sizeof(char) * BUFSIZE);
	int numbytes = comp_size(to_inflate);
	errorcheck(numbytes, "comp_size", "inf");
	z_stream server_to_client;

	server_to_client.zalloc = Z_NULL;
	server_to_client.zfree = Z_NULL;
	server_to_client.opaque = Z_NULL;

	server_to_client.avail_in = (uInt) numbytes + 1;
	server_to_client.next_in = (Bytef *) to_inflate;
	server_to_client.avail_out = (uInt) BUFSIZE;
	server_to_client.next_out = (Bytef *) inflated;

	inflateInit(&server_to_client);
	do {
		inflate(&server_to_client, Z_SYNC_FLUSH);
	} while(server_to_client.avail_in > 0);

	inflateEnd(&server_to_client);

	return inflated;
}

int main(int argc, char *argv[]) {
	getOptions(argc, argv);
	atexit(shutdown_process);

	/*   Setting up terminal settings   */
	int getcheck = tcgetattr(0, &attr_orig);
	errorcheck(getcheck, "tcgetattr", "main");

	getcheck = tcgetattr(0, &attr_new);
	errorcheck(getcheck, "tcgetattr", "main");

	attr_new.c_iflag = ISTRIP;
	attr_new.c_oflag = 0;
	attr_new.c_lflag = 0;

	int setcheck = tcsetattr(0, TCSANOW, &attr_new);
	errorcheck(setcheck, "tcsetattr", "main");

	setupSocket();

	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN+POLLHUP+POLLERR;
	pollfds[1].fd = sockfd;
	pollfds[1].events = POLLIN+POLLHUP+POLLERR;

	while(1) {
		int pollret = poll(pollfds, 2, -1);
		errorcheck(pollret, "poll", "main");
		char cr_lf[] = "\r\n";

		

		if (pollfds[0].revents & POLLIN) {
			int bytesread = read(STDIN_FILENO, buf, BUFSIZE);
			errorcheck(bytesread, "read", "main");

			if (compressflag == 1) {
				char* compressed_buf = NULL;
				compressed_buf = def(buf);
				int size = comp_size(compressed_buf);
				if (compressed_buf != NULL) {
					int check = write(sockfd, compressed_buf, size);
					errorcheck(check, "compressed write", "main");
					//printf("Sent from client: %s", compressed_buf);
					free(compressed_buf);
				}
			}
			else {
				int check = write(sockfd, buf, bytesread);
				errorcheck(check, "write", "main");
			}

			int i;
			for(i = 0; i < strlen(buf); i++) {
				if(buf[i] == '\r' || buf[i] == '\n') {
					write(STDOUT_FILENO, cr_lf, 2);
				}
				else {
					write(STDOUT_FILENO, &buf[i], 1);
				}
			}
			if (bytesread > 0) {
				updateLog(buf, bytesread, SENT);
			}
			
		}
		memset(buf, 0, BUFSIZE);
		if (pollfds[1].revents & POLLIN) {
			int bytesread = read(sockfd, buf, BUFSIZE);
			errorcheck(bytesread, "read", "main");
			if (bytesread == 0) {
				exit(0);
			}
			//printf("Received from server before decompression: %s\n", buf);
			//TODO: do compression after updating log when receiving
			if (bytesread > 0) {
				updateLog(buf, bytesread, RECEIVED);
			}
			//inflate compressed input from server
			if (compressflag == 1) {
				char* decompressed_buf = inf(buf);
				if (decompressed_buf != NULL) {
					strcpy(buf, decompressed_buf);
					free(decompressed_buf);
				}
			}
			//printf("buf after decompression: %s\n", buf);
			int i;
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] == '\n') {
					write(STDOUT_FILENO, cr_lf, 2);
				}
				else {
					int writebytes = write(STDOUT_FILENO, &buf[i], 1);
					errorcheck(writebytes, "write", "main");
				}
			}
			
			
		}
		memset(buf, 0, BUFSIZE);
		if (pollfds[1].revents & (POLLHUP|POLLERR)) {
			exit(0);
		}
	}



	exit(0);
}















