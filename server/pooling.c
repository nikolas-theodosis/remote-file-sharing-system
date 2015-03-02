#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dataServer.h"
#include "queue.h"

/*worker thread's function*/
void *pooling(void* argp) {
	int i, queue_size;
	char *path;
	struct Return_values *rvalues;
	/*struct for thread's arguments*/
	struct Thread_args *args_pool = (struct Thread_args*) argp;
	queue_size = args_pool->queue_size;
	while (1) {
		/*lock mutex*/
		pthread_mutex_lock(&mtx);
		/*check if queue is empty*/
		while(count(queue_size) == -1) {
			pthread_cond_wait(&cond_nonempty, &mtx);
		}
		/*rvalues contains the file removed from queue*/
		rvalues = removeFromQueue(queue_size);
		/*unlock mutex*/
		pthread_mutex_unlock(&mtx);
		pthread_cond_signal(&cond_nonfull);
		/*lock client's mutex*/
		pthread_mutex_lock(rvalues->mtx);
		/*send the files back to client*/
		sendResponse(rvalues->newsock, rvalues->path);
		/*unlock the client's mutex*/
		pthread_mutex_unlock(rvalues->mtx);
		free(rvalues->path);
		free(rvalues);
	}
	
}

/*it writes back to client the path to each file*/
void sendResponse(int newsock, char *path) {
	int length, nread, fd, file_size, bla = 1;
	char temp;
	length = strlen(path) + 1;
	/*write length of the path*/
	if (write(newsock, &length, sizeof(int)) < 0) {
		perror("write");
	}
	/*write the path itself*/
	if (write(newsock, path, length) < 0) {
		perror("write");
	}
	/*open the file to copy data*/
	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open"); 
		exit (1) ;
	}

	long size = sysconf (_SC_PAGESIZE);
	send_data(newsock, fd, size);
	
}

/*send file's data back to client*/
void send_data(int newsock, int fd, int size) {
	long file_size;
	int counter=0;
	struct stat statbuf;
	fstat(fd, &statbuf);
	/*save file's size*/
	file_size = statbuf.st_size;
	/*write file's size back to client*/
	write(newsock, &file_size, sizeof(long));
	/*while there are bytes to read*/
	while(1) {
		char buff[size];
		/*read "size" bytes from file*/
        int nread = read(fd, buff, size);
		counter = counter + nread;
        /* If read was success, send data. */
        if(nread > 0) {
            buff[nread] = '\0';
            write(newsock, buff, size);
        }
	    buff[0] = '\0';
        /*
        * There is something tricky going on with read .. 
        * Either there was error, or we reached end of file.
        */
		/*if there are no more bytes to read, break*/
        if (nread == 0) {
			break;
	    }
            
    }
	printf("READ : %d\n", counter);
    close(fd);
    
}


