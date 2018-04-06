#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "SortedList.h" 

#define BILLION 1000000000L

//typedefs
typedef struct sublist {
	SortedList_t list;
	pthread_mutex_t lock;
	int spinlock;
} sublist_t;


//flags for sync options
int opt_sync = 0;
int m_flag = 0;
int s_flag = 0;

//parameters passed in
static int num_lists = 1;
static int num_threads = 1;
static int num_iters = 1;


unsigned long long num_operations = 1;
int opt_yield = 0;

//list stuff
SortedListElement_t *elem_arr;
sublist_t *multi_list;
long long int *waiting = NULL;

//for csv output
char message[69] = "list";


void errorcheck(int checkval, char* funcname, char* routinename) {
	if (checkval != 0) {
		fprintf(stderr, "Error occured with the %s function in the %s routine.\n%s\n", funcname, routinename, strerror(errno));
		exit(1);
	}
}

long long int calculate_time(struct timespec *start, struct timespec *end) {
	long long total_time = BILLION * (end->tv_sec - start->tv_sec) + end->tv_nsec - start->tv_nsec;
	return total_time;
}

int  hash_func(const char* key) {
	unsigned int list_id = ((unsigned)(key[0] + key[1] + key[2])) % num_lists;
	return (int) list_id;
}

void sighandler(int signum) {
	char sigmessage[] = "Segmentation fault signal received.\n";
	if (signum == SIGSEGV) {
		write(STDERR_FILENO, sigmessage, strlen(sigmessage));
	}
	exit(2);
}

void init_elements(int num_operations) {
	int i, j, x;
	multi_list = (sublist_t*) malloc(sizeof(sublist_t) * num_lists);
	if (multi_list == NULL) {
		errorcheck(-1, "malloc", "init_elements");
	}
	
	for (x = 0; x < num_lists; x++) {
		sublist_t *sublist = &multi_list[x];
		SortedList_t *list = &(sublist->list);
		list->key = NULL;
		list->next = list;
		list->prev = list;
		if (s_flag)
			sublist->spinlock = 0;
		if (m_flag)
			pthread_mutex_init(&sublist->lock, NULL);
		/*SortedList_t *list = (SortedList_t*) malloc(sizeof(SortedList_t));
		if (list == NULL)
			errorcheck(-1, "malloc", "init_elements");
		SortedList_t templist = {list, list, NULL};
		memcpy(list, &templist, sizeof(SortedList_t));

	        pthread_mutex_t *lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
		pthread_mutex_t templock;
		if (lock == NULL)
			errorcheck(-1, "malloc", "init_elements");
		pthread_mutex_init(&templock, NULL);
		memcpy(lock, &templock, sizeof(pthread_mutex_t));

		int *spinlock = (int*) malloc(sizeof(int));
		int tempspinlock = 0;
		if (spinlock == NULL)
			errorcheck(-1, "malloc", "init_elements");
		memcpy(spinlock, &tempspinlock, sizeof(int));

		multi_list[x].list = list;
		multi_list[x].lock = lock;
		multi_list[x].spinlock = spinlock;*/
	}	

	elem_arr = (SortedListElement_t*) malloc(num_operations * sizeof(SortedListElement_t));
	if (elem_arr == NULL) {
		errorcheck(-1, "malloc", "init_lements");
	}

	srand((unsigned int) time(NULL));
	for (i = 0; i < num_operations; i++) {
		char *elem_key = (char*) malloc(5 * sizeof(char));
		for (j = 0; j < 3; j++) {
			elem_key[j] = rand() % 256;
		}
		elem_key[3] = '\0';
		SortedListElement_t add_me = {NULL, NULL, elem_key};
		memcpy(&elem_arr[i], &add_me, sizeof(SortedListElement_t));
	}

	if (opt_sync) {
		waiting = (long long int*) malloc(sizeof(long long int) * num_threads);
		if (waiting == NULL) {
			errorcheck(-1, "malloc", "init_lements");
		}
	}
	return;
}

void getOptions(int argc, char **argv) {
	int c = 0;
	static struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"sync" , required_argument, 0, 's'},
		{"yield", required_argument, 0, 'y'},
		{"lists", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "t:i:s:y:l:", long_options, 0)) != -1) {

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
		case 'l':
			num_lists = atoi(optarg);
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
			opt_sync = 1;
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
		}
		else if (s_flag) {strcat(message, "-s");}
	}
	else {
		strcat(message, "-none");
	}
	return;
}


void *do_work(void* arg) {
	signal(SIGSEGV, sighandler);
	
	int tnum = *((int*) arg);
	
	int *spinlock;
	pthread_mutex_t *lock;
	
	//each thread should only do their share of operations and no more
	int startpos = tnum * num_iters;
	int endpos = (tnum + 1) * num_iters;

	struct timespec start, end;
	
	sublist_t *sublist;
	SortedList_t* list_ptr;
	int i;
	//inserts
	for (i = startpos; i < endpos; i++) {
		const char* key_lookup = elem_arr[i].key;
		int list_index = hash_func(key_lookup);
		sublist = &multi_list[list_index];
		//printf("%d", list_index);
		list_ptr = &sublist->list;
		if (m_flag) {
			lock = &sublist->lock;
			clock_gettime(CLOCK_MONOTONIC, &start);
			pthread_mutex_lock(lock);
			clock_gettime(CLOCK_MONOTONIC, &end);
			waiting[tnum] += calculate_time(&start, &end);
			SortedList_insert(list_ptr, &elem_arr[i]);
			pthread_mutex_unlock(lock);
		}
		else if (s_flag) {
			spinlock = &sublist->spinlock;
			clock_gettime(CLOCK_MONOTONIC, &start);
			while(__sync_lock_test_and_set(spinlock, 1));
			clock_gettime(CLOCK_MONOTONIC, &end);
			waiting[tnum] += calculate_time(&start, &end);
			SortedList_insert(list_ptr, &elem_arr[i]);
			__sync_lock_release(spinlock);
		}
		else {
			SortedList_insert(list_ptr, &elem_arr[i]);
		}
	}

	/*
	need to get the locks of all the lists so that the lengths don't change while we are getting the lengths of the lists
	
	was originally trying to only lock one list length, but the others' lengths changed as I was getting the current
	list's length 
	*/
	if (m_flag) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (i = 0; i < num_lists; i++) {
			pthread_mutex_lock(&(multi_list[i].lock));
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		waiting[tnum] += calculate_time(&start, &end);
		for (i = 0; i < num_lists; i++) {
			if (SortedList_length(&(multi_list[i].list)) == -1) {
				fprintf(stderr, "error finding length in m_flag block\n\nThread number: %d\nList number: %d", tnum, i);
				exit(2);
			}
		}
		for (i = 0; i < num_lists; i++) {
			pthread_mutex_unlock(&(multi_list[i].lock));
		}
	}
	else if (s_flag) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		for (i = 0; i < num_lists; i++) {
			while(__sync_lock_test_and_set(&(multi_list[i].spinlock), 1));
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		waiting[tnum] += calculate_time(&start, &end);
		for (i = 0; i < num_lists; i++) {
			if (SortedList_length(&(multi_list[i].list)) == -1) {
				fprintf(stderr, "error finding length in s_flag block\nThread number: %d\nList number: %d", tnum, i);
				exit(2);
			}
		}
		for (i = 0; i < num_lists; i++) {
			__sync_lock_release(&(multi_list[i].spinlock));
		}
	}
	else {
		for (i = 0; i < num_lists; i++) {
			if (SortedList_length(&(multi_list[i].list)) == -1) {
				fprintf(stderr, "error finding length in unprotected block\n\nThread number: %d\nList number: %d", tnum, i);
				exit(2);
			}
		}
	}
	
	//deletes
	for (i = startpos; i < endpos; i++) {
		const char* key_lookup = elem_arr[i].key;
		int list_index = hash_func(key_lookup);
		sublist = &multi_list[list_index];
		list_ptr = &sublist->list;
		if (m_flag) {
			lock = &sublist->lock;
			clock_gettime(CLOCK_MONOTONIC, &start);
			pthread_mutex_lock(lock);
			clock_gettime(CLOCK_MONOTONIC, &end);
			waiting[tnum] += calculate_time(&start, &end);
			SortedListElement_t *found = SortedList_lookup(list_ptr, key_lookup);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup in m_flag block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting in m_flag block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
			pthread_mutex_unlock(lock);
		}
		else if (s_flag) {
			spinlock = (&sublist->spinlock);
			clock_gettime(CLOCK_MONOTONIC, &start);
			while(__sync_lock_test_and_set(spinlock, 1));
			clock_gettime(CLOCK_MONOTONIC, &end);
			waiting[tnum] += calculate_time(&start, &end);
			SortedListElement_t *found = SortedList_lookup(list_ptr, key_lookup);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup in m_flag block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting in s_flag block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
			__sync_lock_release(spinlock);
		}
		else {
			SortedListElement_t *found = SortedList_lookup(list_ptr, key_lookup);
			if (found == NULL) {
				fprintf(stderr, "error doing lookup in unprotected block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
			if (SortedList_delete(found) != 0) {
				fprintf(stderr, "error deleting in unprotected block\n");
				fprintf(stderr, "Thread number: %d\n", tnum);
				exit(2);
			}
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	getOptions(argc, argv);

	num_operations = num_iters * num_threads;
	init_elements(num_operations);

	pthread_t *t_ids = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
	int *tnums = (int*) malloc (sizeof(int) * num_threads);
	int j;
	for (j = 0; j < num_threads; j++) {
		tnums[j] = j;
	}

	//start timer
	struct timespec start, finish;
	int timecheck = clock_gettime(CLOCK_MONOTONIC, &start);
	errorcheck(timecheck, "clock_gettime", "main");

	//create threads
	int i;
	for (i = 0; i < num_threads; i++) {
		int threadcheck = pthread_create(&t_ids[i], NULL, do_work, (void*) &tnums[i]);
		errorcheck(threadcheck, "pthread_create", "main");
	}

	for (i = 0; i < num_threads; i++) {
		int threadcheck = pthread_join(t_ids[i], NULL);
		errorcheck(threadcheck, "pthread_join", "main");
	}

	//stop timer
	timecheck = clock_gettime(CLOCK_MONOTONIC, &finish);
	errorcheck(timecheck, "clock_gettime", "main");

	long long int total_time = BILLION * (finish.tv_sec - start.tv_sec) + finish.tv_nsec - start.tv_nsec;
	long long int total_ops = num_iters * num_threads * 3;
	long long int time_per_op = total_time / total_ops;
	long long int lock_wait_time = 0;
	
	if (m_flag || s_flag) {
		int y;
		for (y = 0; y < num_threads; y++) {    
			lock_wait_time += waiting[y];  
                }
		lock_wait_time /= (num_operations * 2) + (num_lists * num_threads);
	}

	fprintf(stdout, "%s,%d,%d,%d,%lli,%lli,%lli,%lli\n", message, num_threads, num_iters, num_lists, total_ops, total_time, time_per_op, lock_wait_time);

	free(t_ids);
	free(elem_arr);

	exit(0);
}
