/*
 * tpool.h
 * Simple threadpool implementation using Pthreads in C
 *	~kny8mare@gmail.com, indranilrayc@gmail.com
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>


#ifndef __TPOOL_H
#define __TPOOL_H


typedef void * (* thread_func_t)(void *);



/*
 * Task Queue node
 */
typedef struct __taskq{

	thread_func_t t_func;
	void *t_args;
	
	struct __taskq *next;

}taskq_t;


/*
 * Thread Pool record struct
 */
typedef struct{
	
	int tq_size;		/* task queue size (pending tasks) */
	taskq_t *tq_head;	/* task queue head */
	taskq_t *tq_tail;	/* task queue tail */

	int n_threads;		/* number of worker threads */
	pthread_t *w_tid;	/* threadID of worker threads */
	
	pthread_mutex_t taskq_lock;	/* task queue add/del mutex */
	pthread_cond_t taskq_not_empty;
	
	pthread_cond_t no_pending;
	int pending;		
	
	int stop;		/* stop/join all worker threads */
	int freeze;		/* stop accepting new tasks */

}tpool_t;


/*
 * Creates a thread pool with NO_OF_WORKER_THREADS worker threads
 * All tasks submitted to the thread pool will be performed by one of
 * the worker threads.
 * @param no_of_worker_threads fixed number of worker threads to spawn
 * @return a pointer to a tpool_t structure
 */
tpool_t * tpool_create(int no_of_worker_threads);


/*
 * Adds a task to the thread pool
 * (will not accept tasks once tpool_destroy or tpool_finish has been called)
 * @param tpool pointer to a thread_pool struct returned by tpool_create
 * @param task pointer to a task function, which the threadpool will execute
 * @param args pointer to an argument to pass to the task function
 */
void tpool_add_task(tpool_t *tpool, thread_func_t task, void *args);

/*
 * Wait for all tasks in the task queue (both in queue tasks, and those that are
 * currently executing) to finish before returning. 
 * @param tpool pointer to a tpool_t structure
 */
void tpool_wait(tpool_t *tpool);

/*
 * Destroys the threadpool, after completing all pending tasks.
 * (Note: once tpool_finish is called, the threadpool will not accept any
 * more tasks.) 
 * (implicitly calls tpool_wait and tpool_destroy)
 * @param tpool pointer to a tpool_t structure
 */
void tpool_finish(tpool_t *tpool);

/*
 * Destroys the threadpool and frees all thread pool structures.
 * (completes execution of running worker threads, but does not ensure
 * that the task queue is empty before returning, i.e there may be pending
 * tasks in the task queue, that havent started execution).
 * @param tpool pointer to a tpool_t structure
 */
void tpool_destroy(tpool_t *tpool);


#endif // __TPOOL_H
