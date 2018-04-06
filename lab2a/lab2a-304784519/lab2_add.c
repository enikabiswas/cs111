#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define BILLION 1000000000L

#define M 1
#define S 2
#define C 3
#define YIELD 4

//to determine functionality of add wrapper
int opt_total = 0;
//given global variable for add function
int opt_yield = 0;

//lock variable
pthread_mutex_t lock;
int spinlock = 0;

//passed in arguments
static int num_threads = 1;
static int num_iters = 1;

//sync version
char sync_version[69];
int syncflag = 0;

void errorcheck(int checkval, char* funcname, char* routinename) {
	if (checkval != 0) {
		fprintf(stderr, "Error occured with the %s function in the %s routine.\n%s\n", funcname, routinename, strerror(errno));
		exit(1);
	}
}

long long calculate_time(struct timespec start, struct timespec finish) {
	//https://www.cs.rutgers.edu/~pxk/416/notes/c-tutorials/gettime.html
	long long diff = BILLION * (finish.tv_sec - start.tv_sec) + finish.tv_nsec - start.tv_nsec;
	return diff;
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
    	sched_yield();
    *pointer = sum;
}

void add_comp_swap(long long *pointer, long long value) {
	long long prev;
    do {
    	prev = *pointer;
    	if(opt_yield) {
    		sched_yield();
    	}
    } while((__sync_val_compare_and_swap(pointer, prev, prev + value)) != prev);
}

void *add_wrapper(void *counter) {
	int i;
	int opt_by_itself = (opt_total > 3) ? opt_total - 4 : opt_total;
	for(i = 0; i < num_iters; i++) {
		switch(opt_by_itself) {
			case 0:
				add((long long*) counter, 1);
				break;
			case 1:
				pthread_mutex_lock(&lock);
				add((long long*) counter, 1);
				pthread_mutex_unlock(&lock);
				break;
			case 2:
				while(__sync_lock_test_and_set(&spinlock, 1));
				add((long long*) counter, 1);
				__sync_lock_release(&spinlock);
				break;
			case 3:
				add_comp_swap((long long*) counter, 1);
				break;
			default:
				add((long long*) counter, 1);
				break;
		}
		
	}

	for (i = 0; i < num_iters; i++) {
		switch(opt_by_itself) {
			case 0:
				add((long long*) counter, -1);
				break;
			case 1:
				pthread_mutex_lock(&lock);
				add((long long*) counter, -1);
				pthread_mutex_unlock(&lock);
				break;
			case 2:
				while(__sync_lock_test_and_set(&spinlock, -1));
				add((long long*) counter, -1);
				__sync_lock_release(&spinlock);
				break;
			case 3:
				add_comp_swap((long long*) counter, -1);
				break;
			default:
				add((long long*) counter, -1);
				break;
		}
	}
	return NULL;
}

void getOptions(int argc, char **argv) {
	int c = 0, optlen = 0;
	while(1) {
		if (c == -1) {
			break;
		}
		static struct option long_options[] = {
                        {"threads", required_argument, 0, 't'},
                        {"iterations", required_argument, 0, 'i'},
                        {"sync" , required_argument, 0, 's'},
                        {"yield", no_argument, 0, 'y'},
                        {0, 0, 0, 0}
                };

                int option_index = 0;
                c = getopt_long(argc, argv, "y:t:i:s:", long_options, &option_index);

		if (c == -1) {
			break;
		}
		switch(c) {
			case 't': 
				//--threads
				num_threads = atoi(optarg);
				break;
			case 'i': 
				//--iterations
				num_iters = atoi(optarg);
				break;
			case 's': 
				//--sync
				syncflag = 1;
				optlen = strlen(optarg);
				if (optlen != 1) {
					fprintf(stderr, "Invalid argument specified for --sync option.\nSpecify either m, s, or c for this option\n");
					exit(1);
				}
				switch(optarg[0]) {
					case 'm':
						opt_total += M;
						break;
					case 's':
						opt_total += S;
						break;
					case 'c':
						opt_total += C;
						break;
					default:
						break;
				}

				break;
			case 'y':
				opt_yield = 1;
				opt_total += YIELD;
				break;
			default:
				fprintf(stderr, "unrecognized argument");
				exit(1);
				break;
		}
	}

	switch(opt_total) {
		case 0:
			strcpy(sync_version, "add-none\0");
			break;
		case 1:
			strcpy(sync_version, "add-m\0");
			break;
		case 2:
			strcpy(sync_version, "add-s\0");
			break;
		case 3:
			strcpy(sync_version, "add-c\0");
			break;
		case 4:
			strcpy(sync_version, "add-yield-none\0");
			break;
		case 5:
			strcpy(sync_version, "add-yield-m\0");
			break;
		case 6:
			strcpy(sync_version, "add-yield-s\0");
			break;
		case 7:
			strcpy(sync_version, "add-yield-c\0");
			break;
		default:
			strcpy(sync_version, "add-none\0");
			break;
	}
}

int main(int argc, char *argv[]) {

	getOptions(argc, argv);

	//shared data counter
	long long counter = 0;
	if (opt_total == M || opt_total == M + YIELD) {
		pthread_mutex_init(&lock, NULL);
	}

	struct timespec start, finish;
	//get the starting time of the program
	int timecheck = clock_gettime(CLOCK_MONOTONIC, &start);
	errorcheck(timecheck, "clock_gettime", "main");

	pthread_t *tids = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
	int i, threadcheck;
	for (i = 0; i < num_threads; i++) {
		threadcheck = pthread_create(&tids[i], NULL, add_wrapper, &counter);
		errorcheck(threadcheck, "pthread_create", "main");
	}

	for (i = 0; i < num_threads; i++) {
		threadcheck = pthread_join(tids[i], NULL);
		errorcheck(threadcheck, "pthread_join", "main");
	}

	timecheck = clock_gettime(CLOCK_MONOTONIC, &finish);

	long long total_time = calculate_time(start, finish);

	int total_ops = num_iters * num_threads * 2;
	int time_per_op = total_time / total_ops;
	fprintf(stdout, "%s,%d,%d,%d,%lli,%d,%lli\n", sync_version, num_threads, num_iters, total_ops, total_time, time_per_op, counter);
	free(tids);
    
    exit(0);
}






