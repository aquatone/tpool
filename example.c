#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "tpool.h"

void *thread_func(void *arg){

	int i;
	sleep(0.2);
	printf("%d\n", (int)arg);

}

#define NUM_TASKS 15000

int main(){
	
	tpool_t *tpool;
	int i, j;

	tpool = tpool_create(10);

	for(i = 0; i < NUM_TASKS; i++)
		tpool_add_task(tpool, thread_func, (void *)i);
	
	
	tpool_finish(tpool);
	/* implicitly calls tpool_wait and tpool_destroy
	 * but once finish is called, cannot perform any
	 * add_task operations
	 */
	
	return 0;
}
