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
static char filename[1024];
static FILE* fp = NULL;
static int stop = 0;
static int shutdown = 0;

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
	//char *time_str = (char*) malloc(sizeof(char) * 10);
	sprintf(time_str, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	time_str[8] = '\0';
        //return time_str;
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
		{0, 0, 0, 0}
	};
	while ((c = getopt_long(argc, argv, "p:s:l:", long_options, NULL)) != -1) {
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
			fp = fopen(filename, "w");
			if (fp == NULL) {
				error(-1, "fopen", "getoptions");
			}
			break;
		default:
			usage();
			break;
		}
        }
	return;
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


void interrupted() {
	shutdown = 1;
	return;
}


int main(int argc, char **argv) {
	getoptions(argc, argv);
	//printf("%s\n", gettime());

	//init
	tempSensor = mraa_aio_init(AIO_TEMP);
	button = mraa_gpio_init(DIO_BUTTON);
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &interrupted, NULL); 

	//printf("yuh\n");
	struct pollfd pollStdin = {STDIN_FILENO, POLLIN | POLLHUP | POLLERR, 0};
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
			fputs(output, stdout);
			if (log_flag) {
				fputs(output, fp);
			}
			nexttime = clk.tv_sec + period;
		}
		
		//check poll
		pollStdin.revents = 0;
		int ret = poll(&pollStdin, 1, 0); //immediate return if no input to be found
		//printf("%d\n", ret);
		if (ret >= 1) {
			//get input		
			memset(input, '\0', 1024);
			read(STDIN_FILENO, input, 1024);
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

		/*if (mraa_gpio_read(button) == 1) {
			shutdown = 1;
		}*/
	}
	
	//shutdown process
	gettime(time_str);
	sprintf(output, "%s SHUTDOWN\n", time_str);
	fputs(output, stdout);
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
