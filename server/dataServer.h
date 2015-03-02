
typedef struct List_data *Listptr;

struct Thread_args {
	int newsock;
	int queue_size;
};

struct List_data {
	char *file;
	Listptr next;
};



void server(int, int);
void *thread_f(void*);
int lookup(const char*, int, int, Listptr*, int);
void *pooling(void*);
void addToList(Listptr*, char*);
void call_queue_from_list(Listptr, int, pthread_mutex_t*);
void deleteList(Listptr*);
int countList(Listptr);
void exit_server(int);

extern int queue_size;
extern int pool_size;
extern pthread_t *cons;
