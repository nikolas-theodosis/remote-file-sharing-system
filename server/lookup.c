
/**************************************************/
/*						  */
/*  http://www.lemoda.net/c/recursive-directory/  */
/*						  */
/**************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
/* "readdir" etc. are defined here. */
#include <dirent.h>
/* limits.h defines "PATH_MAX". */
#include <limits.h>

#include "dataServer.h"
#include "queue.h"

/* List the files in "dir_name". */

int lookup (const char * dir_name, int newsock, int queue_size, Listptr *list, int count_files)
{
        //printf("LOOK : %p\n", client_mtx);
    DIR * d;
    int i, files;
    struct dirent direntp;
    struct dirent *result;
    /* Open the directory specified by "dir_name". */
    d = opendir (dir_name);

    /* Check it was opened. */
    if (! d) {
        fprintf (stderr, "Cannot open directory '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    while (1) {
        struct dirent * entry;
        const char * d_name;

        /* "Readdir" gets subsequent entries from "d". */
		readdir_r(d, &direntp, &entry);
        if (! entry) {
            /* There are no more entries in this directory, so break
               out of the while loop. */
            break;
        }
        d_name = entry->d_name;
        /* Print the name of the file and directory. */
		int path_length_temp;
        char path_temp[PATH_MAX];
 
        path_length_temp = snprintf (path_temp, PATH_MAX,"%s/%s", dir_name, d_name);
        if (path_length_temp >= PATH_MAX) {
        	fprintf (stderr, "Path length has got too long.\n");
                exit (EXIT_FAILURE);
        }
	

        /* See if "entry" is a subdirectory of "d". */

        if (entry->d_type & DT_DIR) {

            /* Check that the directory is not "d" or d's parent. */
            
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                int path_length;
                char path[PATH_MAX];
 
                path_length = snprintf (path, PATH_MAX,
                                        "%s/%s", dir_name, d_name);
                if (path_length >= PATH_MAX) {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }
                /* Recursively call "list_dir" with the new path. */
                lookup (path, newsock, queue_size, list, count_files);
            }
        }
		else {
			/*add file to list*/
			addToList(list, path_temp);
		}
    }
    
    /* After going through all the entries, close the directory. */
    if (closedir (d)) {
        fprintf (stderr, "Could not close '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }

    return count_files;
}

void addToList(Listptr* list, char *path) {
	Listptr templist;
	templist = *list;
	/*first running job*/
	if (templist == NULL) {
		*list = malloc(sizeof(struct List_data));
		(*list)->file = malloc(sizeof(char) * (strlen(path) + 1));
		strcpy((*list)->file, path);
		(*list)->next = templist; 
	}
	/*not the first job in the list*/
	else {
		while (*list != NULL)
			list = &((*list)->next);
		*list = malloc(sizeof(struct List_data));
		(*list)->file = malloc(sizeof(char) * (strlen(path) + 1));
		strcpy((*list)->file, path);
		(*list)->next = NULL; 
	}
}

int countList(Listptr list) {
	int count = 0;
	while (list != NULL) {
		count++;
		list = list->next;
	}
	return count;
}

/*add to queue every file of the list*/
void call_queue_from_list(Listptr list, int newsock, pthread_mutex_t* client_mtx) {
	int count = 0;
	while (list != NULL) {
		count++;
		//printf("LIST : %s\n", list->file);
		addToQueue(newsock, queue_size, list->file, client_mtx);
		list = list->next;
	}
}

void deleteList(Listptr *list) {
	Listptr templist;
	while ((*list) != NULL) {
		templist = *list;
		*list = (*list)->next;
		free(templist->file);
		free(templist);
	}
}


