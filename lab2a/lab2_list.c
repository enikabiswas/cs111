#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>

#include "SortedList.h" 

#define BILLION 1000000000L

//flags for sync options
int opt_sync = 0;
int m_flag = 0;
int s_flag = 0;

//parameters passed in
int num_threads = 1;
int num_iters = 1;
int num_lists = 1;
unsigned long long num_operations = 1;
int opt_yield = 0;

//list stuff
SortedList_t *list;
SortedListElement_t *elem_arr;

//locks
pthread_mutex_t lock;
int spinlock = 0;

//for csv output
char message[69] = "list";

typedef struct thread_args
{
	int thread_num;
	SortedList_t *list;
	SortedListElement_t *elements;
} thread_args;

void errorcheck(int checkval, char* funcname, char* routinename) {
	if (checkval != 0) {
		fprintf(stderr, "Error occured with the %s function in the %s routine.\n%s\n", funcname, routinename, strerror(errno));
		exit(1);
	}
}

void init_elements(int num_operations) {
	int i, j; 
	for (i = 0; i < num_operations; i++) {
		char *elem_key = (char*) malloc(5 * sizeof(char));
		for (j = 0; j < 4; j++) {
			elem_key[j] = rand() % 256;
		}
		elem_key[4] = '\0';
		SortedListElement_t add_me = {NULL, NULL, elem_key};
		memcpy(&elem_arr[i], &add_me, sizeof(SortedListElement_t));
	}
}

void getOptions(int argc, char **argv) {
	int c = 0;
	while(1) {
		if (c == -1) {
			break;
		}
		static struct option long_options[] = {
                        {"threads", required_argument, 0, 't'},
                        {"iterations", required_argument, 0, 'i'},
                        {"sync" , required_argument, 0, 's'},
                        {"yield", required_argument, 0, 'y'},
                        {0, 0, 0, 0}
                };

                int option_index = 0;
                c = getopt_long(argc, argv, "y:t:i:s:", long_options, &option_index);

		if (c == -1) {
			break;
		}
		int i, size;
		switch(c) {
			case 't': 
				//--threads
				num_threads = atoi(optarg);
				break;
			case 'i': 
				//--iterations
				num_iters = atoi(optarg);
				break;
			case 'y':
				size = strlen(optarg);
				for (i = 0; i < size; i++) {
					if (optarg[i] == 'i') {
						opt_yield |= INSERT_YIELD;
					}
					else if (optarg[i] == 'd') {
						opt_yield |= DELETE_YIELD;
					}
					else if (optarg[i] == 'l') {
						opt_yield |= LOOKUP_YIELD;
					}
					else {
						fprintf(stderr, "Incorrect usage!. Proper usage: --yield=[idl]");
						exit(1);
					}
				}
				break;
			case 's':
				if (strlen(optarg) != 1) {
					fprintf(stderr, "invalid argument size. proper usage: --sync=[m/s/c]\n");
					exit(1);
				}
				if (optarg[0] == 'm') {
					m_flag = 1;
				}
				else if (optarg[0] == 's') {
					s_flag = 1;
				}
				break;
			default:
				fprintf(stderr, "unrecognized argument\n");
				exit(1);
				break;
		}
	}

	if (opt_yield) {
		strcat(message, "-");
		if (opt_yield & INSERT_YIELD) {strcat(message, "i");}
		if (opt_yield & DELETE_YIELD) {strcat(message, "d");}
		if (opt_yield & LOOKUP_YIELD) {strcat(message, "l");}
	}
	else {
		strcat(message, "-none");
	}

	if (opt_sync) {
		if (m_flag) {
			strcat(message, "-m");
			pthread_mutex_init(&lock, NULL);
		}
		else if (s_flag) {strcat(message, "-s");}
	}
	else {
		strcat(message, "-none");
	}
}

void *do_work(void* arguments) {
	thread_args *passed_args = (thread_args*) arguments;
	int tnum = passed_args->thread_num;
	SortedList_t* list_ptr = passed_args->list;
	SortedListElement_t *elem_ptr = passed_args->elements;

	//each thread should only do their share of operations and no more
	int startpos = tnum * num_iters;
	int endpos = (tnum + 1) * num_iters;

	//printf("%d\n", SortedList_length(list_ptr));

	int i;
	//inserts
	for (i = startpos; i < endpos; i++) {
		if (m_flag) {
			pthread_mutex_lock(&lock);
			SortedList_insert(list_ptr, &elem_ptr[i]);
			pthread_mutex_unlock(&lock);
		}
		else if (s_flag) {
			while(__sync_lock_test_and_set(&spinlock, 1));
			SortedList_insert(list_ptr, &elem_ptr[i]);
			__sync_lock_release(&spinlock);
		}
		else {
			SortedList_insert(list_ptr, &elem_ptr[i]);
		}
	}

	//get size
	if (m_flag) {
		pthread_mutex_lock(&lock);
		if (SortedList_length(list_ptr) == -1) {
			fprintf(stderr, "error finding length in m_flag block\n");
			exit(2);
		}
		pthread_mutex_unlock(&lock);
	}
	else if (s_flag) {
		while(__sync_lock_test_and_set(&spinlock, 1));
		if (SortedList_length(list_ptr) == -1) {
			fprintf(stderr, "error finding length in s_flag block\n");
			exit(2);
		}
		__sync_lock_release(&spinlock);
	}
	else {
		if (SortedList_length(list_ptr) == -1) {
			fprintf(stderr, "error finding length\n");
			exit(2);
		}
	}
	//printf("%d\n", SortedList_length(list_ptr));


	//deletes
	for (i = startpos; i < endpos; i++) {
		if (m_flag) {
			pthread_mutex_lock(&lock);
			SortedListElement_t *found = SortedList_lookup(list_ptr, elem_ptr[i].key);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup in m_flag block\n");
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting in m_flag block\n");
				exit(2);
			}
			pthread_mutex_unlock(&lock);
		}
		else if (s_flag) {
			while(__sync_lock_test_and_set(&spinlock, 1));
			SortedListElement_t *found = SortedList_lookup(list_ptr, elem_ptr[i].key);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup in m_flag block\n");
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting in s_flag block\n");
				exit(2);
			}
			__sync_lock_release(&spinlock);
		}
		else {
			SortedListElement_t *found = SortedList_lookup(list_ptr, elem_ptr[i].key);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup\n");
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting\n");
				exit(2);
			}
		}
	}
	//printf("%d\n", SortedList_length(list_ptr));
	//printf("-------------\n");
	return NULL;
}

int main(int argc, char *argv[]) {
	getOptions(argc, argv);
	//printf("1\n");
	SortedListElement_t startnode;
	list = &startnode;
	list->key = NULL;
	list->next = list;
	list->prev = list;

	num_operations = num_iters * num_threads;
	//printf("num_operations == %s\n", num_operations);
	elem_arr = (SortedListElement_t*) malloc(num_operations * sizeof(SortedListElement_t));
	init_elements(num_operations);
	//printf("2\n");

	pthread_t *t_ids = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
	thread_args *t_args = (thread_args*) malloc(num_threads * sizeof(thread_args));

	//start timer
	struct timespec start, finish;
	int timecheck = clock_gettime(CLOCK_MONOTONIC, &start);
	errorcheck(timecheck, "clock_gettime", "main");
	//printf("3\n");

	//create threads
	int i;
	for (i = 0; i < num_threads; i++) {
		t_args[i].thread_num = i;
		t_args[i].list = list;
		t_args[i].elements = elem_arr;
		int threadcheck = pthread_create(&t_ids[i], NULL, do_work, (void*) &t_args[i]);
		errorcheck(threadcheck, "pthread_create", "main");
	}
	//printf("4\n");

	for (i = 0; i < num_threads; i++) {
		int threadcheck = pthread_join(t_ids[i], NULL);
		errorcheck(threadcheck, "pthread_join", "main");
	}
	//printf("5\n");

	//stop timer
	timecheck = clock_gettime(CLOCK_MONOTONIC, &finish);
	errorcheck(timecheck, "clock_gettime", "main");

	long long total_time = BILLION * (finish.tv_sec - start.tv_sec) + finish.tv_nsec - start.tv_nsec;
	int total_ops = num_iters * num_threads * 3;
	int time_per_op = total_time / total_ops;
	fprintf(stdout, "%s,%d,%d,%d,%d,%lli,%d\n", message, num_threads, num_iters, num_lists, total_ops, total_time, time_per_op);

	//printf("6\n");

	free(t_ids);
	free(t_args);
	free(elem_arr);

	exit(0);
}