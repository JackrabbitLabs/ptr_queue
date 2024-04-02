/* SPDX-License-Identifier: Apache-2.0 */
/**       
 * @file        ptrqueue.h
 *
 * @brief       Header file for Pointer Queue library
 *   
 * @copyright   Copyright (C) 2024 Jackrabbit Founders LLC. All rights reserved.
 *       
 * @date        Feb 2024
 * @author      Barrett Edwards <code@jrlabs.io>
 *          
 */

#ifndef _PTRQUEUE_H
#define _PTRQUEUE_H

/* INCLUDES ==================================================================*/

#include <pthread.h>

#include <linux/types.h>

/* MACROS ====================================================================*/

/* ENUMERATIONS ==============================================================*/

/* STRUCTS ===================================================================*/

/**
 * Pointer Queue data structure
 */
struct ptr_queue {
	// Mutex fields
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	int waiting;

	// Data storage fields	
	void **data;
	__u8 *buf;
	__u32 head;
	__u32 tail;
	__u32 array_capacity;
	__u32 user_capacity;
};

/* GLOBAL VARIABLES ==========================================================*/

/* PROTOTYPES ================================================================*/

int pq_empty(struct ptr_queue *pq);
int pq_free(struct ptr_queue *pq);
struct ptr_queue *pq_init(size_t count, size_t obj_size);
__s32 pq_len(struct ptr_queue *pq);
void *pq_pop(struct ptr_queue *pq, int wait);
void pq_print(struct ptr_queue *pq);
int pq_push(struct ptr_queue *pq, void *ptr);

#endif /* ifndef _PTRQUEUE_H */
