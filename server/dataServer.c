#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ftw.h>
#include <dirent.h>
#include <signal.h>

#include "dataServer.h"
#include "queue.h"

#define perror2(s,e) fprintf(stderr, "%s: %s\n", s , strerror(e))

struct Queue_data *queue;
int queue_size; 
int pool_size;

pthread_t *cons;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonfull;
pthread_cond_t cond_nonempty;

void main(int argc, char *argv[]) {
	int port, thread_pool_size, i;
	if (argc != 7) {
		printf("Arguments should be in the following format\n\t");
		printf("-p <port> -s <thread_pool_size> -q <queue_size>\n");
		exit(1);
	}

	i = 1;
	while(argv[i] != NULL) {
		if (strcmp(argv[i], "-p") == 0)
			port = atoi(argv[i+1]);
		else if (strcmp(argv[i], "-q") == 0)
			queue_size = atoi(argv[i+1]);
		else if (strcmp(argv[i], "-s") == 0)
			thread_pool_size = atoi(argv[i+1]);
		i++;
	}
	printf("Server's parameters are :\n");
	printf("\tport: %d\n\tthread_pool_size: %d\n\tqueue_size: %d\n", port, thread_pool_size, queue_size);

	server(port, thread_pool_size);

}

void server(int port, int thread_pool_size) {
	int sock, newsock, err, i;	
	pthread_t thr; //cons[thread_pool_size];
	struct sockaddr_in server ,client;
	socklen_t clientlen;
	struct sockaddr *serverptr =(struct sockaddr*) &server;
	struct sockaddr *clientptr =(struct sockaddr*) &client;		
	struct hostent *rem;

	struct Thread_args *args;
	struct Thread_args *args_pool;

	signal(SIGINT, exit_server);

	pool_size = thread_pool_size;

	/*allocate space for the queue*/
	queue = malloc(queue_size * sizeof(struct Queue_data));
	/*initialize all cells as empty*/
	for (i=0 ; i<queue_size ; i++)
		queue[i].empty = 1;
	
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);
	
	if (bind(sock, serverptr, sizeof(server)) < 0) {
		perror("bind");
		exit(1);
	}
	/*listen up to 20 connections*/
	if (listen(sock, 20) < 0) {	
		perror("listen");
		exit(1);
	}
	
	printf("Server was successfully initialized...\n");
	printf("Listening for connections to port %d\n", port);

	clientlen = sizeof(*clientptr);
	/*initialize queue mutex and condition variables*/
	pthread_mutex_init(&mtx, 0);
	pthread_cond_init(&cond_nonfull, 0);
	pthread_cond_init(&cond_nonempty, 0);

	args_pool = malloc(sizeof(struct Thread_args));
	args_pool->queue_size = queue_size;

	/*allocate space for the thread pool*/
	cons = malloc(sizeof(pthread_t) * thread_pool_size);

	/*create thread pool*/
	for (i=0 ; i<thread_pool_size ; i++) {
		pthread_create(&cons[i], 0, pooling, (void*) args_pool);
	}

	while(1) {

		/*arguments passed in thread's function*/
		args = malloc(sizeof(struct Thread_args));
		
		if ((newsock = accept(sock, clientptr, &clientlen)) < 0){
			perror("accept");
			exit(1);
		}
		printf("Accepted connection from localhost\n");
		/*find page size*/
		long size = sysconf (_SC_PAGESIZE);
		if (write(newsock, &size, sizeof(long)) < 0) {
			perror("write");
			exit(1);
		}

		args->newsock = newsock;
		args->queue_size = queue_size;
			
		if (err = pthread_create(&thr, 0, thread_f, (void *) args)) {
			perror2("pthread_create", err);
			exit(1);
		}
	}

}
/*thread's function*/
void *thread_f(void *argp) {
	int length, newsock, sock, queue_size, count_files = 0;
	char *path, *directory, *dir, *temp;
	struct Thread_args *args = (struct Thread_args*) argp;
	pthread_mutex_t client_mtx;
	newsock = args->newsock;
	queue_size = args->queue_size;
	Listptr list;
	list = NULL;

	/*read length of path*/
	if (read(newsock, &length, sizeof(int)) < 0) {
		perror("read length");
		exit(1);
	}
	path = malloc((length+1) * sizeof(char));
	
	/*read the path itself*/
	if (read(newsock, path, length) < 0) {
		perror("read directory");
		exit(1);
	}
	path[length] = '\0';

	/*save a copy of the path to split it*/
	temp = malloc(sizeof(char) * (strlen(path)+1));
	strcpy(temp, path);

	temp = strtok(temp, "/");
	
	while (temp != NULL) {
		dir = temp;
		temp = strtok(NULL, "/");
	}
  	
	printf("[Thread: %ld] : About to scan directory %s\n", pthread_self(), dir);
	/*find all files and subdirectories*/
	lookup(path, newsock, queue_size, &list, count_files);
	/*count how many files were found*/
	count_files = countList(list);
	/*write the number of files back to client*/
	if (write(newsock, &count_files, sizeof(int)) < 0) {
		perror("write");
	}

	pthread_mutex_init(&client_mtx, 0);
	/*extract files from list and add them to the queue*/
	call_queue_from_list(list, newsock, &client_mtx);
	deleteList(&list);	
	
	free(path);
	dir = NULL;
	free(temp);
	int exit_flag, err;
	/*exit thread and release resources when read flag*/
	if (read(newsock, &exit_flag, sizeof(int)) < 0) {
		perror("read");
		exit(1);
	}
	
	if (exit_flag == -1) {
		
		close(newsock);
		free(args);
		pthread_mutex_destroy(&client_mtx);
		if ( err = pthread_detach ( pthread_self () )) {
			perror2 ( " pthread_detach " , err ) ;
			exit (1) ;
		}

		pthread_exit (0) ;
	}
		
}

/*signal handler when server is exited*/
void exit_server(int sig) {
	int i;
	printf("\nExiting...\n");
	/*detach each worker thread*/
	for (i=0 ; i<pool_size ; i++) {
		pthread_detach(cons[i]);
	}
	pthread_mutex_destroy(&mtx);
	pthread_cond_destroy(&cond_nonfull);
	pthread_cond_destroy(&cond_nonempty);
	free(cons); 
	free(queue);
	exit(0);
}

