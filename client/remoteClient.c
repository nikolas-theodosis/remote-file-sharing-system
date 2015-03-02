#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "remoteClient.h"

void main(int argc, char *argv[]) {
	int port, i, j, k;
	if (argc != 7) {
		printf("Arguments should be in the following format\n\t");
		printf("-i <server_ip> -p <server_port> -d <directory>\n");
		exit(1);
	}

	i = 1;
	while(argv[i] != NULL) {
		if (strcmp(argv[i], "-p") == 0)
			port = atoi(argv[i+1]);
		else if (strcmp(argv[i], "-i") == 0)
			j = i + 1;
		else if (strcmp(argv[i], "-d") == 0)
			k = i + 1;
		i++;
	}
	printf("Client's parameters are :\n");
	printf("\tserverIP: %s\n\tport: %d\n\tdirectory: %s\n", argv[j], port, argv[k]);
	client(port, argv[j], argv[k]);

}

void client(int port, char* server_ip, char* directory) {
	int sock, length, length_read, count_files;
	long bytes;
	char path_read[256];
	struct sockaddr_in server;
	struct sockaddr *serverptr = (struct sockaddr*) &server;
	struct hostent *rem;
	struct in_addr ip_addr;

	ip_addr.s_addr = inet_addr(server_ip); 

	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	if ((rem = gethostbyaddr(&ip_addr, sizeof(ip_addr), AF_INET)) == NULL) {
		herror("gethostbyaddr");
		exit(1);
	}

	server.sin_family = AF_INET;
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
	server.sin_port = htons(port);
	/*connect to server*/
	if (connect(sock, serverptr, sizeof(server)) < 0) {
		perror("connect");
		exit(1);
	}

	printf("Connecting to %s on port %d\n", server_ip, port);	

	length = strlen(directory);
	long size = 0;
	/*read server's page size*/
	if (read(sock, &size, sizeof(long)) < 0) {
		perror("read");
		exit(1);
	}
	/*write length of the path*/
	if (write(sock, &length, sizeof(int)) < 0) {
		perror("write");
		exit(1);
	}
	/*write the path itself*/
	if (write(sock, directory, length) < 0) {
		perror("write");
		exit(1);
	}
	/*read the number of files found*/
	if (read(sock, &count_files, sizeof(int)) < 0) {
		perror("read");
		exit(1);
	}

	int counter = 0, i;
	char temp;
	char dot[2], dot2[2];
	dot2[0] = '/';
	dot2[1] = '\0';
	dot[0] = '.';
	dot[1] = '\0';
	char *path_file;
	/*loop for every file returned*/
	for (i=0 ; i<count_files ; i++) {
		int j=0;
		/*read length of the path will be sent*/
		if (read(sock, &length_read, sizeof(int)) < 0) {
			perror("read");
			exit(1);
		}
		/*read the path itself*/
		while (j < length_read) {
			if (j < ( j += read(sock , &path_read[j] , length_read - j )))
				exit ( -3) ;		
		}
		path_read[j] = '\0';
		path_file = malloc(sizeof(char) * (j + 1));
		/*if the condition is true it means that*/
		/*the path sent back is absolute*/
		if (path_read[0] == '/') {
			strcpy(path_file, dot);
			/*attach the chars "./" in the path*/
			strcat(path_file, path_read);
			/*create directories*/
			dir(path_read, sock);
			/*create files*/
			file_data(sock, size, path_file);
		}
		/*it means that the path is relative*/
		else {
			strcpy(path_file, dot2);
			strcat(path_file, path_read);
			dir(path_file, sock);
			file_data(sock, size, path_read);
		}
			
		free(path_file);
	}
	/*write flag so the server will exit main thread for this client*/
	int exit_flag = -1;
	if (write(sock, &exit_flag, sizeof(int)) < 0) {
		perror("write");
		exit(1);
	}
	close(sock);

}

/*the function gets the files and creates them in client's local folder*/
void file_data(int sock, long size, char *path) {
	int bytesReceived, counter = 0, fd, length, result;
	char recvBuff[size];
	long file_size;
	struct stat statinfo;
	read(sock, &file_size, sizeof(long));

	result = stat(path, &statinfo);
	if (result < 0) {
		if (errno == ENOENT) {
	        	//do nothing
		}
		else {
			unlink(path);
		}
	}
	/*open the file. if exists, create new one*/
	if ((fd = open(path, O_CREAT|O_WRONLY, 0777)) < 0) {
		perror("open"); 
		return;
	}
	int test;
	/*read file's content if the file's size is smaller that server's page size*/
	if (file_size < size) {
		bytesReceived = read(sock, recvBuff, size);
		length = strlen(recvBuff);
		printf("LENGTH_1 : %d\n", length);
		if (write(fd, recvBuff, length) < 0) {
			perror("write");
		}
	}
	/*read file's content if the file's size is larger than server's page size*/
	else {
		while (counter < file_size) {
			bytesReceived = read(sock, recvBuff, size);
			counter += strlen(recvBuff);
			length = strlen(recvBuff);
			printf("LENGTH_2 : %d\n", bytesReceived);
			write(fd, recvBuff, strlen(recvBuff));
   	 	}
	}
	printf("-----------------------------------\n");
	//printf("Received: %s\n", path);   
        
	
	close(fd);
	recvBuff[0] = '\0';
}

/*it creates all directories and subdirectories*/
void dir(char *path, int newsock) {
	char *file, *s, *dir, *final;
	char tmp[256], dotString[256], dot[2];;
        char *p = NULL;
        size_t len;
	int i, found, file_length, path_length, value, bla = 0;
	/*save the file's name*/
	file = basename(path);
	file_length = strlen(file);
	/*separate file name from path*/
	path_length = strlen(path);
	path[path_length - (file_length + 1)] = '\0';

	dot[0] = '.';
	dot[1] = '\0';	
        snprintf(tmp, sizeof(tmp),"%s",path);
        len = strlen(tmp);
	/*remove possible "/" in the end of path*/
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
		/*when "/" is found, remove it*/
                if(*p == '/') {
                        *p = 0;
			strcpy(dotString, dot);
			strcat(dotString, tmp);
                        mkdir(dotString, S_IRWXU);
			/*put "/" again in the end to continue*/
                        *p = '/';
                }
	strcpy(dotString, dot);
	strcat(dotString, tmp);
    mkdir(dotString, S_IRWXU);


}






