/* SPDX-License-Identifier: Apache-2.0 */
/**       
 * @file        testbench.c
 *
 * @brief       Testbench code file for Pointer Queue library
 *   
 * @copyright   Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *       
 * @date        Feb 2024
 * @author      Barrett Edwards <code@jrlabs.io>
 *          
 */

/* INCLUDES ==================================================================*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/types.h>

#include "main.h"

/* MACROS ====================================================================*/

#define QUEUE_CAPACITY 10
#define ITERATIONS 10000

/* ENUMERATIONS ==============================================================*/

/* STRUCTS ===================================================================*/

/* GLOBAL VARIABLES ==========================================================*/

/* PROTOTYPES ================================================================*/

void *consumer(void *arg)
{
	struct ptr_queue *pq;
	void *ptr;

	pq = (struct ptr_queue*) arg;

	printf("%s started \n", __FUNCTION__);

	for ( int i = 1 ; i < QUEUE_CAPACITY ; i++ ) {
		ptr = pq_pop(pq,1);
		printf("%s popped %p\n", __FUNCTION__, ptr);
	}

	return NULL;
}

void *producer(void *arg)
{
	struct ptr_queue *pq;
	int rv;

	pq = (struct ptr_queue*) arg;

	printf("%s started \n", __FUNCTION__);

	for ( __u64 i = 1 ; i < QUEUE_CAPACITY ; i++ ) {
		rv = pq_push(pq, (void*) i);
		printf("%s pushed val:%llu rv:%d\n", __FUNCTION__, i, rv);
	}

	return NULL;
}

void fill(struct ptr_queue *pq)
{
	int rv;
	__u64 i;

	i = 1;

	printf("-----------------------------\n");
	printf("filling queue\n");

	do {
		rv = pq_push(pq, (void*) i);
		printf("pushed val:%llu result:%d\n", i, rv);
		i++;
	} while (rv == 0);
}

void empty(struct ptr_queue *pq)
{
	int i;
	void *ptr;

	i = 1; 

	printf("-----------------------------\n");
	printf("emptying queue\n");

	do {
		ptr = pq_pop(pq,0);
		printf("popped i:%d val:%p\n", i, ptr);
		i++;
	} while (ptr != NULL);
}

void threads(struct ptr_queue *pq)
{
	int rv;

	empty(pq);

	pthread_t producer_thread;
	pthread_t consumer_thread;

	rv = pthread_create( &consumer_thread, NULL, consumer, (void*) pq );
	if (rv != 0) {
		printf("Error creating producer thread\n");
		exit(1);
	}

	sleep(1);

	rv = pthread_create( &producer_thread, NULL, producer, (void*) pq );
	if (rv != 0) {
		printf("Error creating producer thread\n");
		exit(1);
	}

	sleep(1);

	printf("%s Waiting for threads to exit\n", __FUNCTION__);
	pthread_join( producer_thread, NULL);
	printf("%s joined with producer thread\n", __FUNCTION__);
	pthread_join( consumer_thread, NULL);
	printf("%s joined with consumer thread\n", __FUNCTION__);
}

void iterate(struct ptr_queue *pq)
{
	__u64 i;
	int rv;
	void *ptr;

	printf("-----------------------------\n");
	printf("iterations %d\n", ITERATIONS);

	for ( i = 1 ; i < ITERATIONS ; i++) {
		rv = pq_push(pq, (void*) i);
		if (rv) {
			printf("%llu rb_push() returned an error %d\n", i, rv);
			exit(-1);
		}

		ptr = pq_pop(pq,0);
		if (ptr == NULL) {
			printf("%llu rb_pop() returned an error %p\n", i, ptr);
			exit(-1);
		}
	}
}

int main()
{
	struct ptr_queue *pq;

	pq = pq_init(QUEUE_CAPACITY, 0);

	pq_print(pq);

	fill(pq);

	pq_print(pq);

	empty(pq);

	pq_print(pq);

	iterate(pq);

	threads(pq);

	pq_free(pq);

	return 0;
}
