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

#define BUFSIZE 1024
#define USAGE "usage: ./lab1b-server --port=port#\n"


//option flags
static int portflag = -1;
static int compressflag = -1;

//port number to use
static int portnum = -1;

//pipefd's for child process
int tochild_pipefd[2];
int fromchild_pipefd[2];

//child process id
pid_t childpid;

//socket file descriptor
int sockfd = -1, newsockfd = -1;

//for reading from socket
char buf[BUFSIZE];

//poll file descriptors
struct pollfd pollfds[2];


void errorcheck(int ret, char *calledfunc, char *outerfunc) {
	if (ret == -1) {
		fprintf(stderr, "error calling the %s function in the %s routine\n", calledfunc, outerfunc);
		fprintf(stderr, "%s\n", strerror(errno));
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
                        {0, 0, 0, 0}
                };
                int option_index = 0;
                c = getopt_long(argc, argv, "p:", long_options, &option_index);
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

void execChild(void) {
	close(tochild_pipefd[1]);
	close(fromchild_pipefd[0]);
	int check = dup2(tochild_pipefd[0], STDIN_FILENO);
	errorcheck(check, "the first dup2", "execChild");
	check = dup2(fromchild_pipefd[1], STDOUT_FILENO);
	errorcheck(check, "the second dup2", "execChild");
	check = dup2(fromchild_pipefd[1], STDERR_FILENO);
	errorcheck(check, "the third dup2", "execChild");
	close(tochild_pipefd[0]);
	close(fromchild_pipefd[1]);

	char* exec_args[2];
	char exec_filename[] = "/bin/bash";

	exec_args[0] = exec_filename;
	exec_args[1] = NULL;

	int execCheck = execvp(exec_filename, exec_args);
	errorcheck(execCheck, "exec", "exec_child");
}

void setupSocket(void) {
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	/*   Create Socket   */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	errorcheck(sockfd, "socket", "setupSocket");
	memset((char *) &serv_addr, 0, sizeof(serv_addr)); //zero out the struct so that we don't get garbage values
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portnum); //htons checks to make sure that portnum and network have same endian
	/*   Call bind   */
	printf("%d\n", portnum);
	int check = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	errorcheck(check, "bind", "setupSocket");
	/*   Listen   */
	listen(sockfd, 5); //the 5 is kind of pointless here since we will only set up one connection
	clilen = sizeof(cli_addr);
	/*   Accept   */
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	errorcheck(newsockfd, "accept", "setupSocket");
}

void shutdown_process(void) {
	close(tochild_pipefd[0]);
	close(tochild_pipefd[1]);
	close(fromchild_pipefd[0]);
	close(fromchild_pipefd[1]);

	int wstatus;
	pid_t check = waitpid(childpid, &wstatus, 0);
	if (check == -1) {
		fprintf(stderr, "error calling waitpid()\n");
	}
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(wstatus), WEXITSTATUS(wstatus));
	shutdown(sockfd, SHUT_RDWR);
	shutdown(newsockfd, SHUT_RDWR);
	close(newsockfd);
	close(sockfd);
	//	int yes = 1;
	//	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	//	setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
}



int main (int argc, char *argv[]) {
	signal(SIGINT, sighandler);
	signal(SIGPIPE, sighandler);
	
	getOptions(argc, argv);
	atexit(shutdown_process);

	memset(buf, '\0', BUFSIZE);

	setupSocket();

	int pipecheck = pipe(fromchild_pipefd);
	errorcheck(pipecheck, "pipe", "main");

	pipecheck = pipe(tochild_pipefd);
	errorcheck(pipecheck, "pipe", "main");

	childpid = fork();
	errorcheck(childpid, "fork", "main");
	if (childpid == 0) { //child process
		execChild();
	} 
	else { //parent process
		close(tochild_pipefd[0]);
		close(fromchild_pipefd[1]);

		pollfds[0].fd = newsockfd;
		pollfds[0].events = POLLIN+POLLHUP+POLLERR;
		pollfds[1].fd = fromchild_pipefd[0];
		pollfds[1].events = POLLIN+POLLHUP+POLLERR;
		

		while(1) {
			int pollret = poll(pollfds, 2, -1);
			errorcheck(pollret, "poll", "main");
			char lf = '\n';

			if (pollfds[0].revents & POLLIN) {
				int bytesread = read(newsockfd, buf, BUFSIZE);
				errorcheck(bytesread, "read", "main");

				int i;
				for (i = 0; i < bytesread; i++) {
					if (buf[i] == 0x03) {
						int killcheck = kill(childpid, SIGINT);
						errorcheck(killcheck, "kill", "main");
					}
					else if (buf[i] == 0x04) {
						close(tochild_pipefd[1]);
						exit(0);
					}
					else if (buf[i] == '\r' || buf[i] == '\n') {
						int writebytes = write(tochild_pipefd[1], &lf, 1);
						errorcheck(writebytes, "write", "main");
					}
					else {
						int writebytes = write(tochild_pipefd[1], &buf[i], 1);
						errorcheck(writebytes, "write", "main");
					}
				}
			}

			if (pollfds[1].revents & POLLIN) {
				int bytesread = read(fromchild_pipefd[0], buf, BUFSIZE);
				errorcheck(bytesread, "read", "main");

				int i;
				for (i = 0; i < bytesread; i++) {
					if (buf[i] == 0x04) {
						exit(0);
					}
					else {
						int writebytes = write(newsockfd, &buf[i], 1);
						errorcheck(writebytes, "write", "main");
					}
				}
			}

			if (pollfds[1].revents & (POLLHUP|POLLERR)) {
				exit(0);
			}
		}
	}

	exit(0);
}
