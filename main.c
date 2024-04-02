/* SPDX-License-Identifier: Apache-2.0 */
/**       
 * @file        ptrqueue.c
 *
 * @brief       Code file for Pointer Queue library
 *   
 * @copyright   Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *       
 * @date        Feb 2024
 * @author      Barrett Edwards <code@jrlabs.io>
 *          
 */

/* INCLUDES ==================================================================*/

/* free()
 */
#include <stdlib.h>

/* printf()
 */
#include <stdio.h>

/* errno 
 */
#include <errno.h>

/* memset()
 */
#include <string.h>

/* true 1 
 * false 0
 */
#include <stdbool.h>

/* pthread_mtx
 */
#include <pthread.h>

/* __u32
 * __s32
 */
#include <linux/types.h>

#include "ptrqueue.h"

/* MACROS ====================================================================*/

/* ENUMERATIONS ==============================================================*/

/* STRUCTS ===================================================================*/

/* PROTOTYPES ================================================================*/

/* GLOBAL VARIABLES ==========================================================*/

/* FUNCTIONS =================================================================*/

/*
 * Return 1 if empty,
 *        0 if not empty 
 *       -1 if there was an error and set errno
 * 
 * STEPS:
 * 1: Validate inputs 
 * 2: Obtain lock 
 * 3: Check if head and tail are equal
 * 4: Unlock mutex and return 
 */
int pq_empty(struct ptr_queue *pq)
{
	int rv;

	// Initialize variables 
	rv = -1;

	// STEP 1: Validate inputs 
	if (pq == NULL) 
	{
		errno = EINVAL;
		goto end;
	}

	// STEP 2: Obtain lock 
	pthread_mutex_lock(&pq->mtx); 

	// STEP 3: Check if head and tail are equal
	if (pq->head == pq->tail)
		rv = 1;
	else 
		rv = 0;

	// STEP 4: Unlock mutex and return 
	pthread_mutex_unlock(&pq->mtx); 

end:

	return rv;
}

/* 
 * Relese allocated memory for this ring buffer
 *
 * Return 0 upon success, 1 otherwise and set errno
 *
 * STEPS
 * 1: Validate inputs 
 * 2: Free mutex variables
 * 3: Free data buffer 
 * 4: Free object pointer 
 */
int pq_free(struct ptr_queue *pq)
{
	 int rv; 

	 // Initialize variables 
	 rv = 1;

	 // STEP 1: Validate inputs 
	 if (pq == NULL) 
	 {
		errno = EINVAL;
		goto end;
	 }

	// STEP 2: Free mutex variables
	pthread_mutex_destroy(&pq->mtx);
	pthread_cond_destroy(&pq->cond);

	// Free buffer for entries 
	if (pq->buf != NULL) 
		free(pq->buf);
	pq->buf = NULL;
	
	// STEP 3: Free data buffer 
	if ( pq->data != NULL )
		free(pq->data);
	pq->data = NULL;

	// STEP 4: Free object ptr 
	free(pq);

	rv = 0;

end:

	return rv;
}

/*
 * Create and initialize a pointer queue
 *
 * Param: 
 *	count   : This is the number of buffer entries 
 *  wr_mtx 	: A thread mutex that will wake up the writer thread when the queue goes not full
 *  rd_mtx 	: A thread mutex that will wake up the reader thread when the queue goes non empty
 * 
 * Returns a pointer to a struct ptr_queue upon success. Upon error, returns NULL and sets errno 
 *
 * STEPS
 * 1. Validate inputs
 * 2. Allocate memory for ring_buffer struct 
 * 3. Allocate memory for data buffer 
 * 4: Initialize mutex variables
 */
struct ptr_queue *pq_init(
	size_t count,
	size_t obj_size)
{
	struct ptr_queue *pq;

	// Initialize variables 
	pq = NULL;

	// STEP 1. Validate inputs
	if (count == 0) 
	{
		errno = EINVAL;
		goto end;
	}
	
	// STEP 2. Allocate memory for ptr_queue struct 
	pq = (struct ptr_queue*) calloc(1, sizeof(struct ptr_queue));
	if (pq == 0) 
	{
		errno = ENOMEM;
		goto end;
	}
	pq->array_capacity = count + 1;
	pq->user_capacity = count;

	// STEP 3. Allocate memory for ptr uffer 
	pq->data = (void **) calloc (pq->array_capacity, sizeof(void *)); 
	if (pq->data == 0) 
	{
		errno = ENOMEM;
		free(pq);
		pq = NULL;
		goto end;
	}
	
	// STEP 4: Allocate memory for objects and insert them into queue 
	if (obj_size > 0)
	{
		pq->buf = (__u8*) calloc (count, obj_size); 
		if (pq->buf == 0) 
		{
			errno = ENOMEM;
			goto end;
		}

		for ( size_t i = 0 ; i < count ; i++ )
			pq_push(pq, &pq->buf[i*obj_size]);	
	}

	// STEP 4: Initialize mutex variables
	pthread_mutex_init(&pq->mtx, NULL);
	pthread_cond_init(&pq->cond, NULL);

end:

	return pq;
}

/* 
 * Returns the length in number of entries. Returns a negative number upon error and sets errno.
 *
 * STEPS
 * 1: Validate inputs
 * 2: Obtain lock
 * 3: Compute length 
 * 4: Unlock 
 */
__s32 pq_len(struct ptr_queue *pq)
{
	__s32 rv;

	// Inittialize variables
	rv = 0;

	// STEP 1: Validate inputs
	if (pq == NULL) 
	{ 
		errno = EINVAL;
		rv = -EINVAL;
		goto end;
	}

	// STEP 2: Obtain lock
	pthread_mutex_lock(&pq->mtx);

	// STEP 3: Compute length 

	// Empty state 
	if (pq->head == pq->tail) 
		rv = 0;

	// Normal state 
	else if (pq->tail > pq->head)
		rv = pq->tail - pq->head;

	// wrap around state 
	else if (pq->head > pq->tail) 
		rv = (pq->array_capacity - pq->head) + pq->tail; 

	// STEP 4: Unlock 
	pthread_mutex_unlock(&pq->mtx);
	
end:

	return rv;
}

/*
 * Return the pointer at the current head location 
 * 
 * Return pointer  upon success, 0 otherwise and set errno
 * 
 * STEPS
 * 1: Validate input 
 * 2: Obtain lock
 * 3: Check if we are empty 
 * 4: Get the value out of the array
 * 5: Compute new head index 
 * 6: Unlock mutex and return 
 */
void *pq_pop(struct ptr_queue *pq, int wait)
{
	void *rv;

	// Initialize variables 
	rv = NULL;

	// STEP 1: Validate input 
	if (pq == NULL) 
	{
		errno = EINVAL; 
		goto end;
	}

	// STEP 2: Obtain lock
	// If the caller wants to wait, then pend upon the lock
	// Caller does not want to wait so try the lock and return immediately 
	if (wait) 
		pthread_mutex_lock(&pq->mtx); 
	else 
		if ( pthread_mutex_trylock(&pq->mtx) != 0 )
			goto end;		

	// STEP 3: Check if we are empty 
	/* If head == tail then the queue is empty
	 * Set a waiting bit to tell the other threads that we are waiting and need to be signaled 
	 * Pend upon the condition signal to wake up when the queue goes non empty
	 */
	if (pq->head == pq->tail) 
	{
		// Return immediately if caller does not want to wait
		if (!wait)
			goto unlock;

		// Set bit to indicate the consumer is waiting
		pq->waiting = 1;

		// Check again if queue is still empty 
		if (pq->head == pq->tail) 
		{
			// Set call back condition when queue goes non empty
			pthread_cond_wait(&pq->cond, &pq->mtx);
		} 

		// Reset waiting bit to 0
		pq->waiting = 0;

		// At this point the queue is no longer empty, get the value and return normally
	}

	// STEP 4: Get the value out of the array
	rv = pq->data[pq->head];
	pq->data[pq->head] = NULL;

	// STEP 5: Compute new head
	pq->head = pq->head + 1;
	if (pq->head >= pq->array_capacity)
		pq->head -= pq->array_capacity;

unlock:

	// STEP 6: Unlock mutex and return 
	pthread_mutex_unlock(&pq->mtx); 

end:

	return rv;
}

void pq_print(struct ptr_queue *pq)
{
	if (pq == NULL) {
		return;
	}

	printf("Printing ptr_queue ------------------------------\n");
	printf("pq address:            %p\n", pq);
	printf("pq->data:              %p\n", pq->data);
	printf("pq->array_capacity:    %u\n", pq->array_capacity);
	printf("pq->user_capacity:     %u\n", pq->user_capacity);
	printf("pq->len:               %u\n", pq_len(pq));
	printf("pq->head:              %u\n", pq->head);
	printf("pq->tail:              %u\n", pq->tail);
	printf("pq->waiting:           %d\n", pq->waiting);

	for ( __u32 i = 0 ; i < pq->array_capacity ; i++ ) 
	{
		printf("data[%02d]:       %p\n", i, pq->data[i]);
	}
}

/*
 * Insert a new entry at the current tail location
 *
 * Return 0 upon success, 1 if error and set errno
 *
 * STEPS
 * 1: Validate inputs
 * 2: Obtain lock 
 * 3: Compute new tail
 * 4: Check if we are full
 * 5: Store the new ptr at the current tail 
 * 6: Store the new tail index
 * 7: If consumer thread is waiting for queue to go nonempty, signal it
 * 8: Unlock and exit 
 */
int pq_push(struct ptr_queue *pq, void *ptr)
{
	int rv; 
	__u32 new_tail; 

	// Initialize variables 
	rv = 1;

	// STEP 1: Validate inputs
	if (pq == NULL) 
	{
		errno = EINVAL;
		goto end;
	}

	// STEP 2: Obtain lock 
	pthread_mutex_lock(&pq->mtx); 

	// STEP 3: Compute new tail
	new_tail = pq->tail + 1;
	if (new_tail >= pq->array_capacity)
		new_tail -= pq->array_capacity; 

	// STEP 4: Check if we are full
	if (new_tail == pq->head) 
	{
		errno = ENOMEM;;
		goto unlock;
	}

	// STEP 5: Store the new ptr at the current tail 
	pq->data[pq->tail] = ptr;

	// STEP 6: Store the new tail index
	pq->tail = new_tail;

	// STEP 7: If consumer thread is waiting for queue to go nonempty, signal it
	if (pq->waiting == 1) 
		pthread_cond_signal(&pq->cond);

	rv = 0;

unlock:

	// STEP 8: Unlock and exit 
	pthread_mutex_unlock(&pq->mtx); 

end:

	return rv;
}

