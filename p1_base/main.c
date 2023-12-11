#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

int CONST_SIZE = 1024;
int MAX_PROC;
int proc_counter = 0;
int *status;
int main(int argc, char *argv[]) {
    unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;
    int fid = 0;
    if (argc > 3) {
        char *endptr;
        unsigned long int delay = strtoul(argv[3], &endptr, 10);
        MAX_PROC = atoi(argv[2]);

        if (*endptr != '\0' || delay > UINT_MAX) {
            fprintf(stderr, "Invalid delay value or value too large\n%li\n%s\n",
                    delay, endptr);
            return 1;
        }

        state_access_delay_ms = (unsigned int)delay;
    }
    if (argc == 3) {
        if (ems_init(state_access_delay_ms)) {
            fprintf(stderr, "Failed to initialize EMS\n");
            return 1;
        }
        MAX_PROC = atoi(argv[2]);
        char *path = argv[1];
        DIR *current_dir = opendir(path);
        if (current_dir == NULL) {
            printf("opendir failed on '%s'", path);
            return -1;
        }

        struct dirent *current_file;

        while ((current_file = readdir(current_dir)) != NULL) {

            //char path_copy[strlen(path) + strlen(current_file->d_name) + 3];
            char path_copy[CONST_SIZE];
            strcpy(path_copy, path);

            if (current_file == NULL)
                break;
            size_t size = strlen(current_file->d_name);
            if (!(size >= 5 &&
                  (strcmp(current_file->d_name + size - 5, ".jobs") == 0)))
                continue; /* Skip . and ..  and files that are not .jobs*/

            strcat(strcat(path_copy, "/"), current_file->d_name);

            if (proc_counter == MAX_PROC) {
                printf("===Waiting for proc to finish for file '%s'.\n",current_file->d_name);
                wait(status);
                proc_counter--;
            }
            proc_counter++;
            printf("===Launching process %d for file '%s'.===\n",proc_counter, current_file->d_name);
            fid = fork();

            if (fid != 0) {
                continue;
            } 
            else {
                int fd = open(path_copy, O_RDONLY);

                path_copy[strlen(path_copy) - 5] = '\0';
                strncat(path_copy, ".out", 5);
                int fd_out = open(path_copy, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR);
                parse_start(fd, fd_out);
                close(fd_out);
                close(fd);
                ems_terminate();
                exit(0);
            }
        }
        printf("Finished distributing jobs. Waiting for %d procs.\n",proc_counter);
        for (int i = 0; i < proc_counter; i++) {
            wait(status);
        }
        closedir(current_dir);
        ems_terminate();
        exit(0);
    }
}
