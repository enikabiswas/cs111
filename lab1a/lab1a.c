#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>

struct termios attr_orig;
struct termios attr_new;
tcflag_t orig_iflag;
tcflag_t orig_oflag;
tcflag_t orig_lflag;

char cr_lf[3] = "\r\n";
char lf[2] = "\n";

static int shellflag;

//child process id
pid_t pid;

//pipe file descriptors
int pipefd_tochild[2];
int pipefd_fromchild[2];

//poll file descriptors
struct pollfd fds[2];	

void errorcheck(int check, char* funcname, char* context) {
	if (check == -1) {
		fprintf(stderr, "error with %s() function call in the %s routine\n", funcname, context);
		fprintf(stderr, "%s", strerror(errno));
		exit(1);
	}
}

void sighandler(int signum) {
	char sigpipe_message[] =  "all pipes have been closed.\n";
	char sigint_message[] = "received interrupt signal\n";
	if (signum == SIGPIPE) {
		int check = write(STDERR_FILENO, sigpipe_message, strlen(sigpipe_message));
		errorcheck(check, "write1", "sighandler");
	}
	else if (signum == SIGINT) {
		int killcheck = kill(getpid(), SIGINT);
		errorcheck(killcheck, "kill", "sighandler");
		int writecheck = write(STDERR_FILENO, sigint_message, strlen(sigint_message));
		errorcheck(writecheck, "write2", "sighandler");
	}
	exit(1);
}

void getOptions(int argc, char **argv) {
	int c = 0;
	while(1) {
		if (c == -1) {
			break;
		}
		static struct option long_options[] = {
                        {"shell", no_argument, 0, 's'},
                        {0, 0, 0, 0}
                };

                int option_index = 0;
                c = getopt_long(argc, argv, "s", long_options, &option_index);

		if (c == -1) {
			break;
		}
		switch(c) {
		case 's':
			shellflag = 1;
			break;
		default:
			fprintf(stderr, "unrecognized argument");
			exit(1);
			break;
		}
	}
}

void readNormal(void) {
	char buf[256];
	while(1) {
		int readLen = read(0, buf, 256);
		errorcheck(readLen, "read", "readNormal");
		int i;
		for (i = 0; i < readLen; i++) {
			char cur = buf[i];
			if (cur == '\r' || cur == '\n') {
				int writecheck = write(1, cr_lf, 2);
				errorcheck(writecheck, "write1", "readNormal");
			}
			//ctrl d                   
			else if (cur == 0x04) {
				exit(0);
			}
			else {
				//TODO: check for errors here             
				int check = write(1, &cur, 1);
				errorcheck(check, "write2", "readNormal");
			}
		}
	}
}


void exec_child(int* pipefd_fromchild, int* pipefd_tochild) {
	close(pipefd_tochild[1]);
	close(pipefd_fromchild[0]);
	int check = dup2(pipefd_tochild[0], STDIN_FILENO);
	errorcheck(check, "the first dup2", "exec_child");
	check = dup2(pipefd_fromchild[1], STDOUT_FILENO);
	errorcheck(check, "the second dup2", "exec_child");
	check = dup2(pipefd_fromchild[1], STDERR_FILENO);
	errorcheck(check, "the third dup2", "exec_child");
	close(pipefd_tochild[0]);
	close(pipefd_fromchild[1]);

	char* exec_args[2];
	char exec_filename[] = "/bin/bash";

	exec_args[0] = exec_filename;
	exec_args[1] = NULL;

	int execCheck = execvp(exec_filename, exec_args);
	errorcheck(execCheck, "exec", "exec_child");
}

void shutdown_processing(void) {
	if (shellflag) {
		char buf[256];
		while (1) {
			if (!(fds[1].revents & POLLIN)) {
				break;
			}
			int pollret = poll(fds, 2, -1);
		        errorcheck(pollret, "poll", "shutdown_processing");
			if (fds[1].revents & (POLLHUP|POLLERR)) {
				break;
			}
			int readLen = read(pipefd_fromchild[0], buf, 256);
			errorcheck(readLen, "read", "shutdown_processing");
			int i;
			for (i = 0; i < readLen; i++) {
				if (buf[i] == 0x04) {
					break;
				}
				else if (buf[i] == '\n') {
					int check = write(STDOUT_FILENO, cr_lf, 2);
					errorcheck(check, "write1", "shutdown_processing");
				}
				else {
					int check = write(STDOUT_FILENO, &buf[i], 1);
					errorcheck(check, "write2", "shutdown_processing");
				}
			}
		}
		
		close(pipefd_tochild[0]);
		close(pipefd_tochild[1]);
		
		close(pipefd_fromchild[0]);
		close(pipefd_fromchild[1]);
		
		int wstatus;
		pid_t wait = waitpid(pid, &wstatus, 0);
		if (wait == -1) {
			fprintf(stderr, "error calling waitpid()\n");
		}
		fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(wstatus), WEXITSTATUS(wstatus));
	}
	
	int setcheck = tcsetattr(0, TCSANOW, &attr_orig);
	if (setcheck == -1) {
		fprintf(stderr, "tcsetattr returned -1!");
	}
} 


int main(int argc, char **argv) {
	getOptions(argc, argv);
        //initTerm();
	
	int get = tcgetattr(0, &attr_orig);
        errorcheck(get, "tcgetattr1", "main");
	
	get = tcgetattr(0, &attr_new);
        errorcheck(get, "tcgetattr2", "main");

        attr_new.c_iflag = ISTRIP;
        attr_new.c_oflag = 0;
        attr_new.c_lflag = 0;

        int setcheck = tcsetattr(0, TCSANOW, &attr_new);
	errorcheck(setcheck, "tcsetattr", "main");

	atexit(shutdown_processing);
		
	char buf[256];
	int breakflag = 0;

	if (shellflag) {
		signal(SIGINT, sighandler);
		int ret = pipe(pipefd_fromchild);
		errorcheck(ret, "pipe1", "main");

		ret = pipe(pipefd_tochild);
		errorcheck(ret,"pipe2","main");

		pid = fork();
		errorcheck((int) pid, "fork", "main");
		
		if (pid == 0) {
			//child process,
			exec_child(pipefd_fromchild, pipefd_tochild);
		}
		else {
			//parent process
			signal(SIGPIPE, sighandler);
			close(pipefd_tochild[0]); //don't need to read what we send to the child
			close(pipefd_fromchild[1]); //no need to write to the pipe we're getting input from child
			
			fds[0].fd = STDIN_FILENO;
			fds[0].events = POLLIN+POLLHUP+POLLERR;
			fds[1].fd = pipefd_fromchild[0];
			fds[1].events = POLLIN+POLLHUP+POLLERR;

			while(1) {
				int pollret = poll(fds, 2, -1);
				errorcheck(pollret, "poll", "main");

				if (fds[0].revents & POLLIN) {
					int readLen = read(0, buf, 256);
					errorcheck(readLen, "read1", "main");
					
					int i;
					for (i = 0; i < readLen; i++) {
						//ctrl c
						if (buf[i] == 0x03) {
							int killcheck = kill(pid, SIGINT);
							errorcheck(killcheck, "kill", "main");
						}						
						//ctrl d
						else if (buf[i] == 0x04) {
							close(pipefd_tochild[1]);
							exit(0);
						}
						else if (buf[i] == '\r' || buf[i] == '\n') {
                            int writecheck = write(1, cr_lf, 2);
							errorcheck(writecheck, "write1", "main");
                                                        
							writecheck = write(pipefd_tochild[1], lf, 1);
							errorcheck(writecheck, "write2","main");
						}
						else {
							int writecheck = write(pipefd_tochild[1], &buf[i], 1);
							errorcheck(writecheck, "write3","main");
							
							writecheck = write(STDOUT_FILENO, &buf[i], 1);
							errorcheck(writecheck, "write4","main");
						}
					}
				}

				if (fds[1].revents & POLLIN) {
                    int readLen = read(pipefd_fromchild[0], buf, 256);
					errorcheck(readLen, "read2", "main");
                    int i;
                    for (i = 0; i < readLen; i++) {
                        if (buf[i] == '\n') {
                            int writecheck = write(STDOUT_FILENO, cr_lf, 2);
                            errorcheck(writecheck, "write5","main");
                        }
                        else {
							int writecheck = write(STDOUT_FILENO, &buf[i], 1);
							errorcheck(writecheck, "write6","main");
						}
					}
				}
				if (fds[1].revents & (POLLHUP|POLLERR)) {
					exit(0);
				}
				if (breakflag) {
					break;
				}
			}
		}
	}
	else {
		readNormal();
	}

	exit(0);
}
