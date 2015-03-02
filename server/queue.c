#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "queue.h"


int count(int queue_size) {
	int i, counter = 0;
	for (i=0 ; i<queue_size ; i++) {
		if (queue[i].empty == 1)
			counter++;
	}
	
	if (counter > 0 && counter < queue_size)
		return 1;
	if (counter == 0)
		return 0;
	if (counter == queue_size)
		return -1;
}

void print(int queue_size) {
	int i;
	for (i=0 ; i<queue_size ; i++) {
		if (queue[i].empty == 0)
			printf("%s\n", queue[i].file);
	}
}

/*add file to queue*/
void addToQueue(int newsock, int queue_size, char *path, pthread_mutex_t *client_mtx) {
	int i;
	/*lock mutex*/
	pthread_mutex_lock(&mtx);
	/*check if queue is full*/
	while (count(queue_size) == 0) {
		pthread_cond_wait(&cond_nonfull, &mtx);
	}
	/*find the first empty position in the queue*/
	for (i=0 ; i<queue_size ; i++) {
		if (queue[i].empty == 1) {
			//printf("[Thread: %ld] : Adding file %s to the queue...\n", pthread_self(), path);
			queue[i].empty = 0;
			queue[i].client = newsock;
			queue[i].mtx = client_mtx;
			queue[i].file = malloc(sizeof(char) * (strlen(path) + 1));
			strcpy(queue[i].file, path);
			break;
		}
	}
	/*unlock mutex*/
	pthread_mutex_unlock(&mtx);
	pthread_cond_signal(&cond_nonempty);

}

/*remove file from queue*/
struct Return_values* removeFromQueue(int queue_size) {
	int i, length;
	struct Return_values *rvalues;
	/*save removed file in another struct to return*/
	rvalues = malloc(sizeof(struct Return_values));
	/*remove the first file found*/
	for (i=0 ; i<queue_size ; i++) {
		if (queue[i].empty != 1) {
			//printf("[Thread: %ld] : Received task: <%s, %d>\n", pthread_self(), queue[i].file, queue[i].client);
			//printf("[Thread: %ld] : About to read file %s\n", pthread_self(), queue[i].file);
			rvalues->newsock = queue[i].client;
			rvalues->mtx = queue[i].mtx;
			rvalues->path = malloc(sizeof(char) * (strlen(queue[i].file) + 1));
			strcpy(rvalues->path, queue[i].file);
			queue[i].empty = 1;
			queue[i].client = 0;
			queue[i].mtx = NULL;
			free(queue[i].file);
			break;
		}
	}
	return rvalues;
}


