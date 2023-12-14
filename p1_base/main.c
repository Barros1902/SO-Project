#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int CONST_SIZE = 1024;
int MAX_PROC;
int MAX_THREAD;
int proc_counter = 0;
int *status;
thread_args* current_thread;


int main(int argc, char *argv[]){
    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    int fid = 0;
    if (argc > 4) {
        char *endptr;
        unsigned long int delay = strtoul(argv[4], &endptr, 10);
        MAX_PROC = atoi(argv[2]);
        MAX_THREAD = atoi(argv[3]);
        current_thread = (thread_args *)malloc((size_t)MAX_THREAD * sizeof(thread_args));
        if (*endptr != '\0' || delay > UINT_MAX) {
            fprintf(stderr, "Invalid delay value or value too large\n%li\n%s\n",
                    delay, endptr);
            return 1;
        }

        state_access_delay_ms = (unsigned int)delay;
    }
    if (argc == 4) {
        if (ems_init(state_access_delay_ms)) {
            fprintf(stderr, "Failed to initialize EMS\n");
            return 1;
        }
        MAX_PROC = atoi(argv[2]);
        MAX_THREAD = atoi(argv[3]);
        current_thread = (thread_args *)malloc((size_t)MAX_THREAD * sizeof(thread_args));
        
        char *path = argv[1];
        DIR *current_dir = opendir(path);
        if (current_dir == NULL) {
            printf("opendir failed on '%s'", path);
            return -1;
        }

        struct dirent *current_file;

        while ((current_file = readdir(current_dir)) != NULL) {

            // char path_copy[strlen(path) + strlen(current_file->d_name) + 3];
            char path_copy[CONST_SIZE];
            strcpy(path_copy, path);

            if (current_file == NULL)
                break;
            size_t size = strlen(current_file->d_name);
            if (!(size >= 5 &&
                  (strcmp(current_file->d_name + size - 5, ".jobs") == 0)))
                continue; /* Skip . and ..  and files that are not .jobs*/

            strcat(strcat(path_copy, "/"), current_file->d_name);
            char path_copy_fd[CONST_SIZE];
            char path_copy_fd_out[CONST_SIZE];
            strcpy(path_copy_fd, path_copy);
            path_copy[strlen(path_copy) - 5] = '\0';
            strncat(path_copy, ".out", 5);
            strcpy(path_copy_fd_out, path_copy);

            if (proc_counter == MAX_PROC) {
                wait(status);
                proc_counter--;
            }
            proc_counter++;

            fid = fork();

            if (fid != 0) {
                continue;

            } else {
                pthread_t tid[MAX_THREAD];
                sem_t semaforo;
                sem_init(&semaforo, 0, 0);
                int fd_out = open(path_copy_fd_out, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR);
                for (int i = 0; i < MAX_THREAD; i++) {

                    int fd = open(path_copy_fd, O_RDONLY);
                    current_thread[i].fd = fd;
                    current_thread[i].fd_out = fd_out;
                    current_thread[i].thread_num = i + 1;
                    current_thread[i].max_thread = MAX_THREAD;
                    current_thread[i].semaforo = &semaforo;
                    strcpy(current_thread[i].path, path_copy_fd);
                    pthread_create(&tid[i],NULL,parse_start,(void*)&current_thread[i]);
                }
                for (int i = 0; i < MAX_THREAD; i++) {
                    pthread_join(tid[i],NULL);
                   
                    
                }
                
            
                close(fd_out);
                sem_destroy(&semaforo);
                ems_terminate();
                exit(0);
                  
            }
            
        }
       
        for (int i = 0; i < proc_counter; i++) {
            wait(status);

        }
        closedir(current_dir);
        ems_terminate();
        exit(0);
    }
}
