#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "pflush.h"

enum operation {
	READ,
	WRITE,
	RAND_READ,
	RAND_WRITE,
};

typedef struct parameter {
	uint64_t mem_size;
	uint64_t block_size;
	uint64_t block_num;
	enum operation opt;
	uint64_t num_thread;
	char *pmem_file_path;
	bool init;
	bool isflush;
	int numa;	// bind the thread to given node
	uint64_t *pmem;
} parameter;

typedef struct thread_options {
	uint64_t num;
	uint64_t size;
	uint64_t thread_id;
	uint64_t **access_list;
	
	parameter *par;	

	// for test result
	double elapsed;
} thread_options;

/*
	it depends on your system
*/
static int numa_bind[2][36] = { {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
				36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,},
				{18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 
				54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,} };

double mysecond()
{
        /*
	struct timeval tp;

        gettimeofday(&tp, NULL);
        return ( (double)tp.tv_sec + (double)tp.tv_usec * 1.e-6 );
	*/
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ( (double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9 );
	
}

void* thread_task(void* opt)
{
	thread_options *options = (thread_options*)opt;
	parameter *par = options->par;	
	double start, end;
	uint64_t *buffer = (uint64_t*)malloc(options->size);	
	uint64_t i;	
	
	if (par->numa != -1) {
		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(numa_bind[par->numa][options->thread_id], &mask);
	}

	// init buffer
	memset(buffer, 0, options->size);

	options->access_list = (uint64_t**)malloc(sizeof(uint64_t*) * options->num);
	for (i = 0; i < options->num; i++) {
		options->access_list[i] = par->pmem + (par->mem_size / par->num_thread * options->thread_id + par->block_size * i) / sizeof(uint64_t);
	}

	if (par->opt == RAND_READ || par->opt == RAND_WRITE) {
		// we need to shuffle it
                for (i = 0; i < options->num; i++) {
                        uint64_t index = rand() % (options->num - i) + i; // i ~ BLOCK_NUM
                        uint64_t *tmp = options->access_list[i];
                        options->access_list[i] = options->access_list[index];
                        options->access_list[index] = tmp;
                }
	}
	
	if (par->opt == READ || par->opt == RAND_READ) {
		start = mysecond();
                for (i = 0; i < options->num; i++) {
                        memcpy(buffer, options->access_list[i], par->block_size);
                }
                end = mysecond();
	} else {
		if (par->isflush) {
			start = mysecond();
                	for (i = 0; i < options->num; i++) {
                        	memcpy(options->access_list[i], buffer, par->block_size);
                		// we need to persist 
			}
                	end = mysecond();
		} else {
			start = mysecond();
                        for (i = 0; i < options->num; i++) {
                                memcpy(options->access_list[i], buffer, par->block_size);
                        }
                        end = mysecond();
		}
	}

}

void run_test(parameter* par)
{
	pthread_t threads[72];

	// we do some prepare work out of the thread
	thread_options opts[72];
	uint64_t i;
	
	// we don't care about the sporadic
	// we can use multi threading to accelerate it 
	for (i = 0; i < par->num_thread; i++) {
		opts[i].size = par->block_size;
		opts[i].num = par->block_num / par->num_thread;
		opts[i].thread_id = i;
		opts[i].par = par;
		opts[i].access_list = NULL;
	}
	
	for (i = 0; i < par->num_thread; i++) {
		pthread_create(&threads[i], NULL, thread_task, (void*)&opts[i]);
	}

	for (i = 0; i < par->num_thread; i++) {
		pthread_join(threads[i], NULL);
	}
	
}
