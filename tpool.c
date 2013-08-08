/*
 * Simple Thread Pool Implementation using PThreads
 * 	~kny8mare@gmail.com, indranilrayc@gmail.com
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "tpool.h"

//#define _GNU_SOURCE 


#define MEMASSERT(PTR) do{										\
			if(!(PTR)){ 									\
				fprintf(stderr, "%s - %s : malloc failed.", __FILE__, __func__ );	\
				exit(EXIT_FAILURE);							\
			} 										\
			}while(0)

#define ERROR(STR) do{	fprintf(stderr, "%s - %s : %s", __FILE__, __func__, STR); exit(EXIT_FAILURE); }while(0)



void add_taskq(taskq_t **taskq_head, taskq_t **taskq_tail, thread_func_t t_func, void *t_args);
int del_taskq(taskq_t **taskq_head, taskq_t **taskq_tail, thread_func_t *t_func, void **t_args);
void * tpool_worker_thread(void *tp);


void add_taskq(taskq_t **taskq_head, taskq_t **taskq_tail, thread_func_t t_func, void *t_args){

	taskq_t *new;

	new = (taskq_t *)malloc(sizeof(taskq_t));
	MEMASSERT(new);

	new->t_func = t_func;
	new->t_args = t_args;
	new->next = NULL;
	
	if(*taskq_head == NULL)
		*taskq_head = *taskq_tail = new;
	else{
		(*taskq_tail)->next = new; 
		*taskq_tail = new;
	}
}

int del_taskq(taskq_t **taskq_head, taskq_t **taskq_tail, thread_func_t *t_func, void **t_args){
	
	taskq_t *th = *taskq_head;

	if(th == NULL)
		return 0;

	*t_func = th->t_func;
	*t_args = th->t_args;

	*taskq_head = th->next;
	if(th->next == (void *)NULL)
		*taskq_tail = (void *)NULL;
	
	free(th);
	return 1;
}


void * tpool_worker_thread(void *tp){
	
	thread_func_t t_func;
	void *t_args;
	tpool_t *tpool = (tpool_t *)tp; 
	
	while(1){

		if(tpool->stop)
			break;

		pthread_mutex_lock(&(tpool->taskq_lock));
	
		/* wait till there is a task in the task queue to execute */
		while(tpool->tq_size == 0){
			/* if stop signalled then quit */
			if(tpool->stop){
				pthread_mutex_unlock(&(tpool->taskq_lock));
				pthread_exit(NULL);
			}
	
			pthread_cond_wait(&(tpool->taskq_not_empty), &(tpool->taskq_lock));
		}
		
		/* get a task from the task queue */
		del_taskq(&(tpool->tq_head), &(tpool->tq_tail), &t_func, &t_args);
		
		tpool->tq_size--;
		
		pthread_mutex_unlock(&(tpool->taskq_lock));

		t_func(t_args);		/* call task routine */

		
		pthread_mutex_lock(&(tpool->taskq_lock));
		
		tpool->pending--;
		if(tpool->pending == 0)
			pthread_cond_signal(&(tpool->no_pending));
		
		pthread_mutex_unlock(&(tpool->taskq_lock));
	}

	pthread_exit(NULL);
}



tpool_t * tpool_create(int no_of_worker_threads){

	tpool_t *tpool;
	int i;


	tpool = (tpool_t *)malloc(sizeof(tpool_t));
	MEMASSERT(tpool);
	
	tpool->n_threads = no_of_worker_threads;
	tpool->w_tid = (pthread_t *)malloc(sizeof(pthread_t) * tpool->n_threads);
	MEMASSERT(tpool->w_tid);

	tpool->tq_size = 0;
	tpool->tq_head = tpool->tq_tail = NULL;
	tpool->pending = 0;

	tpool->stop = 0;
	tpool->freeze = 0;
	
	if(pthread_mutex_init(&(tpool->taskq_lock), NULL))
		ERROR("cannot init pthread_mutex.");

	if(pthread_cond_init(&(tpool->taskq_not_empty), NULL) || 
			pthread_cond_init(&(tpool->no_pending), NULL))
		ERROR("cannot init pthread_cond.");

	
	/* Start worker threads */
	for(i = 0; i < tpool->n_threads; i++){
		if(pthread_create(&(tpool->w_tid[i]), NULL, tpool_worker_thread, (void *)tpool))
			ERROR(" cannot init worker threads.");
	}

	return tpool;
}


void tpool_add_task(tpool_t *tpool, thread_func_t task, void *args){

	if(tpool->stop || tpool->freeze)
		return;
	
	pthread_mutex_lock(&(tpool->taskq_lock));
	
	add_taskq(&(tpool->tq_head), &(tpool->tq_tail), task, args);
	tpool->tq_size++;
	tpool->pending++;
	pthread_cond_signal(&(tpool->taskq_not_empty));

	pthread_mutex_unlock(&(tpool->taskq_lock));
}



void tpool_wait(tpool_t *tpool){
	
	pthread_mutex_lock(&(tpool->taskq_lock));

	while(tpool->pending != 0)
		pthread_cond_wait(&(tpool->no_pending), &(tpool->taskq_lock));
	
	pthread_mutex_unlock(&(tpool->taskq_lock));

}


void tpool_finish(tpool_t *tpool){

	tpool->freeze = 1;	/* Stop adding new tasks to queue */

	tpool_wait(tpool);	/* Finish pending tasks */
	
	tpool_destroy(tpool);
}


void tpool_destroy(tpool_t *tpool){

	int i;
	void *res;

	tpool->stop = 1;

	pthread_cond_broadcast(&(tpool->taskq_not_empty));
	
	/* Join threads */
	for(i = 0; i < tpool->n_threads; i++){
		pthread_join(tpool->w_tid[i], &res);
	}

	free(tpool->w_tid);
	pthread_mutex_destroy(&(tpool->taskq_lock));
	pthread_cond_destroy(&(tpool->taskq_not_empty));
	pthread_cond_destroy(&(tpool->no_pending));

}
