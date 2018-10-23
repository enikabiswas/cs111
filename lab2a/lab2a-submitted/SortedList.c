#include "SortedList.h"
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	//do error checking for the head of the list (check if key is null)
	if (list == NULL || element == NULL || list->key != NULL) {
		return;
	}
	//keep current pointer
	SortedList_t *cur = list->next;
	//go from head of list, keep comparing the value of key of current to the 'element' argument's key
	while(cur->key != NULL && strcmp(cur->key, element->key) < 0) {
		cur = cur->next;
	}
	if (opt_yield & INSERT_YIELD) {
		sched_yield();
	}
	//elements next goes to current's next, current's next's prev goes to element, currents next goes to element, elements prev goes to current
	element->prev = cur->prev;
	cur->prev = element;
	element->next = cur;
	element->prev->next = element;
	
}

int SortedList_delete( SortedListElement_t *element) {
	if (element == NULL || element->key == NULL) {
		return 1;
	}

	//same check as lookup, not part of critical section, so we do this before sched yield
	if (element->next->prev != element || element->prev->next != element) {
		return 1;
	}

	if (opt_yield & DELETE_YIELD) {
		sched_yield();
	}
	//don't need to formally free this because we have an array that points to all elements anyways
	//memory will not be lost because of this function
	element->next->prev = element->prev;
	element->prev->next = element->next;

	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	if (list == NULL || list->key != NULL) {
		return NULL;
	}
	SortedListElement_t *found = list->next;
	while (found != list) {
		if (strcmp(found->key, key) == 0) {
			return found;
		}
		if (opt_yield & LOOKUP_YIELD) {
			sched_yield();
		}
		found = found->next;
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) {
	if (list == NULL || list->key != NULL) {
		return -1;
	}
	SortedList_t *cur = list;
	int counter = 0;
	while(1) {
		if (cur == list) {
			break;
		}
		//checks all prev and next pointers:
		if (cur->next->prev != cur || cur->prev->next != cur) {
			return -1;
		}
		counter++;
		if (opt_yield & LOOKUP_YIELD) {
			sched_yield();
		}
		cur = cur->next;
	}
	return counter;
}
