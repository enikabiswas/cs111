/*
NAME: Kubilay Agi
EMAIL: kubilayagi@g.ucla.edu
ID: 304784519
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>
#include <signal.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>

#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <sys/socket.h>

#define AIO_TEMP 1
#define DIO_BUTTON 62

#define LOG "LOG"
#define SCALE_F "SCALE=F"
#define SCALE_C "SCALE=C"
#define START "START"
#define STOP "STOP"
#define OFF "OFF"
#define PERIOD "PERIOD="

//option argument values
static int period = 1;
static char scale = 'F';
static int log_flag = 0;
static char filename[1024] = NULL;
static FILE* fp = NULL;
static int stop = 0;
static int shutdown = 0;
static char id[10] = "304784519";
static char host[1024] = NULL;
static int port = 0;
static char id_command[13] = "ID="

mraa_aio_context tempSensor;
mraa_gpio_context button;

void error(int check, char* func, char* routine) {
	if (check < 0) {
		fprintf(stderr, "*** Error in the %s function in the %s routine\n%s\n", func, routine, strerror(errno));
		exit(1);
	}
}

void sighandler(int signum) {
	char message[] = "*** Received signal interrupt!\n";
	if (signum == SIGINT) {
		write(2, message, strlen(message));
		exit(1);
	}
}

//TODO: change this shit breh
void usage(void) {
	fprintf(stderr, "*** Invalid Argument\nUsage: --period=<#seconds> --log=<filename.txt> --scale=[F,C]\n");
	exit(1);
}

void gettime(char* time_str) {
	//http://www.cplusplus.com/reference/ctime/localtime/
	time_t cur_time;
	struct tm *timeinfo;
	time(&cur_time);
	timeinfo = localtime(&cur_time);
	sprintf(time_str, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	time_str[8] = '\0';
}

double f_to_c(double fahr) {
	double celc = (fahr - 32)/1.8;
	return celc;
}

double c_to_f(double celc) {
	double fahr = (celc * 1.8) + 32;
	return fahr;
}

double gettemp(void) {
	//printf("yuh\n");
	double read = mraa_aio_read(tempSensor);
	//printf("yuh\n");
	int B = 4275;
	double R0 = 100000.0;
	double R = 1023.0/((double) read) - 1.0;
	R *= R0;
	double C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	double F = c_to_f(C);
	return (scale == 'F') ? F : C;
}

void getoptions(int argc, char **argv) {
	int c;
	static struct option long_options[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'},
		{"id", required_argument, 0, 'i'},
		{"host", required_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	while ((c = getopt_long(argc, argv, "p:s:l:i:h:", long_options, NULL)) != -1) {
                switch (c) {
		case 'p':
			period = atoi(optarg);
			break;
		case 's':
			if (strlen(optarg) != 1) {
				usage();
			}
			scale = optarg[0];
			break;
		case 'l':
			log_flag = 1;
			strcpy(filename, optarg);
			fp = fopen(filename, "w+");
			if (fp == NULL) {
				error(-1, "fopen", "getoptions");
			}
			break;
		case 'i':
			if (strlen(optarg) != 9) {
				fprintf(stderr, "*** Error: ID needs to be 9 digits long\n");
				exit(1);
			}
			strcat(id, optarg);
			strcat(id_command, optarg);
			break;
		case 'h':
			strcpy(host, optarg);
			break;
		default:
			usage();
			break;
		}
	}

	if (log_flag == 1) {
		fputs(id_command, fp);
	}
	if (optind < argc) {
		port = atoi(argv[optind]);
	}
	else {
		fprintf(stderr, "*** Error: port argument is required\n");
		exit(1);
	}
	if (port <= 0) {
		fprintf(stderr, "*** Error: Invalid port number!\n");
		exit(1);
	}

	if (host == NULL) {
		fprintf(stderr, "*** Error: host argument is required!\n");
		exit(1);
	}
	return;
}

int socket_setup(void) {
	int sockfd = 0;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	error(sockfd, "socket", "setupSocket");
	//TODO: setsockopt
	server = gethostbyname(host);
	if (server == NULL) {
		error(-1, "gethostbyname", "socket_setup");
	}
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portnum);
	int check = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	error(check, "connect", "setupSocket");
	return sockfd;
}

void processinput(char* cmd) {
	//do a whole bunch of strncmp's for the first however many characters of the command
	//printf("cmd = %s\n", cmd);
	int len = strlen(cmd);
	int check = 0;
	if (len >= 7) {
		if ((check = strncmp(cmd, SCALE_F, 7)) == 0) {
			scale = 'F';
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
		else if ((check = strncmp(cmd, SCALE_C, 7)) == 0) {
			scale = 'C';
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
	        else if ((check = strncmp(cmd, PERIOD, 7)) == 0) {
			char *arg = &cmd[7];
			period = atoi(arg);
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
		else if ((check = strncmp(cmd, LOG, 3)) == 0) {
			if (!log_flag) {
			        fprintf(stderr, "*** Error: cannot log because no file was specified");
		        	return;
			}
			char message[2048];
			sprintf(message, "%s\n", cmd);
			fputs(message, fp);
			return;
		}
		else {
			fprintf(stderr, "Invalid command received\n");
			return;
		}
	}
	else if (len >= 5) {
		if ((check = strncmp(cmd, START, 5)) == 0) {
			stop = 0;
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
		else if ((check = strncmp(cmd, LOG, 3)) == 0) {
			if (!log_flag) {
				fprintf(stderr, "*** Error: cannot log because no file was specified");
				return;
			}
                	char message[2048];
                	sprintf(message, "%s\n", cmd);
                	fputs(message, fp);
			return;
		}
		else {
			fprintf(stderr, "Invalid command received\n");
			return;
		}
	}
	else if (len == 4) {
		if ((check = strncmp(cmd, STOP, 4)) == 0) {
			stop = 1;
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
		else {
			fprintf(stderr, "Invalid command received\n");
			return;
		}
	}
	else if (len == 3) {
		if ((check = strncmp(cmd, OFF, 3)) == 0) {
			shutdown = 1;
			if (log_flag) {
                char message[2048];
                sprintf(message, "%s\n", cmd);
                fputs(message, fp);
			}
			return;
		}
		else {
			fprintf(stderr, "Invalid command received\n");
			return;
		}
	}
	else {
		fprintf(stderr, "Invalid command received\n");
		return;
	}
	return;
}


void ssl_setup(int sockfd, SSL *sslclient) {
	if (strstr(argv[0], "tls")) {
		SSL_library_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
		SSL_CTX *newContext = SSL_CTX_new(TLSv1_client_method());
		if (newContext == NULL) {
			fprintf(stderr, "*** Error: unable to get SSL context\n");
			exit(2);
		}
		sslclient = SSL_new(newContext);
		if (sslclient == NULL) {
			fprintf(stderr, "*** Error: unable to complete SSL setup\n");
			exit(2);
		}
		if (!SSL_set_fd(sslclient, sockfd)) {
			fprintf(stderr, "*** Error: unable to associate fd with SSL\n");
			exit(2);
		}
		if (SSL_connect(sslclient) != 1) {
			fprintf(stderr, "*** Error: SSL connection rejected\n");
			exit(2);
		}
	}
}


void interrupted() {
	shutdown = 1;
	return;
}


int main(int argc, char **argv) {
	SSL *sslclient = NULL;
	getoptions(argc, argv);
	int sockfd = socket_setup();
	ssl_setup(sockfd, sslclient);
	//printf("%s\n", gettime());

	//init
	tempSensor = mraa_aio_init(AIO_TEMP);
	button = mraa_gpio_init(DIO_BUTTON);
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &interrupted, NULL); 

	//printf("yuh\n");
	struct pollfd pollCmd = {sockfd, POLLIN | POLLHUP | POLLERR, 0};
	struct timeval clk;
	time_t nexttime = 0;
	char input[1024];
	char output[1024];
	char cmd[1025];
	char *time_str = (char*) malloc(sizeof(char) * 10);	
	memset(input, '\0', 1024);
	memset(output, '\0', 1024);
	memset(cmd, '\0', 1025);
	//printf("%s\n", input);

	while(!shutdown) {
		//run
		gettimeofday(&clk, 0);
		//printf("yuh\n");
		if (!stop && clk.tv_sec >= nexttime) { //its time for another update
			double temp_read = gettemp();
			//printf("yuh\n");
			int temp = temp_read * 10;
			gettime(time_str);
			sprintf(output, "%s %d.%1d\n", time_str, temp/10, temp%10);
			if (sslclient != NULL) {
				(void) SSL_write(sslclient, output, strlen(output));
			}
			else {
				(void) write(sockfd, output, strlen(output));
			}
			if (log_flag) {
				fputs(output, fp);
			}
			nexttime = clk.tv_sec + period;
		}
		
		//check poll
		pollCmd.revents = 0;
		int ret = poll(&pollCmd, 1, 0); //immediate return if no input to be found
		//printf("%d\n", ret);
		if (ret >= 1) {
			//get input		
			memset(input, '\0', 1024);
			if (sslclient != NULL) {
				SSL_read(sslclient, input, 1024);
			}
			else {
				read(sockfd, input, 1024);  //********** changed from stdin_fileno
			}
			//split input into individual commands
			int i = 0, len = strlen(input);
			for (i = 0; i < len; i++) {
				if (input[i] != '\n') {
					strncat(cmd, &input[i], 1);
				}
				else if (input[i] == '\n') {
					processinput(cmd);
					memset(cmd, '\0', 1024);
				}
			}
		}
		else if (ret == 0) {
			//continue;
			//eof?
			//fprintf(stderr, "*** Received EOF\n");
			//break;
		}
		else {
			//error
			fprintf(stderr, "*** Poll Error: %s\n", strerror(errno));
			break;
		}
	}
	
	//shutdown process
	gettime(time_str);
	sprintf(output, "%s SHUTDOWN\n", time_str);
	write(sockfd, output, strlen(output));
	if (log_flag) {
		fputs(output, fp);
	}
	

	if (log_flag) {
		fclose(fp);
	}
	mraa_aio_close(tempSensor);
	mraa_gpio_close(button);
	
	exit(0);
}
