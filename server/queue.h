int count(int);
void print(int);
void addToQueue(int, int, char*, pthread_mutex_t*);
struct Return_values* removeFromQueue(int);
void sendResponse(int, char*);
int write_all(int, void*, size_t);
void send_data(int, int, int);

struct Queue_data {
	char *file;
	int client;
	int empty;
	pthread_mutex_t *mtx;
};

struct Return_values {
	int newsock;
	char *path;
	pthread_mutex_t *mtx;
};

extern struct Queue_data *queue; 
extern pthread_mutex_t mtx;
extern pthread_cond_t cond_nonfull;
extern pthread_cond_t cond_nonempty;
