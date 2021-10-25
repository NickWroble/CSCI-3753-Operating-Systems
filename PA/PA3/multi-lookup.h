#ifndef array_H
#define array_H

#include "stdio.h"
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>

#define STACK_SIZE 			10 // max elements in array
#define MAX_NAME_LENGTH				255
#define MAX_INPUT_FILES 100
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define MAX_REQUESTOR_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define NOT_RESOLVED "NOT_RESOLVED"

typedef struct {
    char array[STACK_SIZE][MAX_NAME_LENGTH];                  //  array of strings
    int top;                                // array index indicating where the top is
	sem_t *empty_count, *fill_count, *mutex;
} stack_struct;

typedef struct {
	FILE  *res_outfile; //file pointer to single output log
	int *num_files;
	long int tid;
	stack_struct *stack;
	sem_t *outfp_mutex;
	sem_t *stdout_mutex;
} pthread_create_resolve_args; 


typedef struct {
	FILE *req_outfile;
	int *num_files;
	int *fp;
	char **argv;
	stack_struct *stack;
	sem_t *stdout_mutex, *args_mutex; 
} pthread_create_request_args;

int 	stack_init	(stack_struct *s);
void 	print		(stack_struct *s);						//hard coded print

void* 	producer(void *ptr);     	 // place element on the top of the array
void*  	consumer(void *ptr);    	// remove element from the top of the array

#endif

