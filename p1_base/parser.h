#ifndef EMS_PARSER_H
#define EMS_PARSER_H

#include <stddef.h>
#include <semaphore.h>
#include <pthread.h>
enum Command {
    CMD_CREATE,
    CMD_RESERVE,
    CMD_SHOW,
    CMD_LIST_EVENTS,
    CMD_BARRIER,
    CMD_WAIT,
    CMD_HELP,
    CMD_EMPTY,
    CMD_INVALID,
    EOC // End of commands
};

typedef struct{
    char path[1024];
    int fd;
    int fd_out;
    int thread_num;
    int max_thread;
    sem_t *semaforo;

} thread_args;


/// Reads a line and returns the corresponding command.
/// @param fd File descriptor to read from.
/// @return The command read.
enum Command get_next(int fd,  int *line);

/// Recieves a file descriptor to start parsing.
/// @param fd File descriptor to read from.
/// @param fd_out File descriptor to write to.
/// @return The return of the switch cases.
void* parse_start(void* args);

int my_line( int thread_num, int line, int max_thread);

/// Parses a CREATE command.
/// @param fd File descriptor to read from.
/// @param fd_out File descriptor to write to.
/// @param event_id Pointer to the variable to store the event ID in.
/// @param num_rows Pointer to the variable to store the number of rows in.
/// @param num_cols Pointer to the variable to store the number of columns in.
/// @return 0 if the command was parsed successfully, 1 otherwise.
int parse_create(int fd, unsigned int *event_id, size_t *num_rows,
                 size_t *num_cols);

/// Parses a RESERVE command.
/// @param fd File descriptor to read from.
/// @param max Maximum number of coordinates to read.
/// @param event_id Pointer to the variable to store the event ID in.
/// @param xs Pointer to the array to store the X coordinates in.
/// @param ys Pointer to the array to store the Y coordinates in.
/// @return Number of coordinates read. 0 on failure.
size_t parse_reserve(int fd, size_t max, unsigned int *event_id, size_t *xs,
                     size_t *ys);

/// Parses a SHOW command.
/// @param fd File descriptor to read from.
/// @param event_id Pointer to the variable to store the event ID in.
/// @return 0 if the command was parsed successfully, 1 otherwise.
int parse_show(int fd, unsigned int *event_id);

/// Parses a WAIT command.
/// @param fd File descriptor to read from.
/// @param delay Pointer to the variable to store the wait delay in.
/// @param thread_id Pointer to the variable to store the thread ID in. May not
/// be set.
/// @return 0 if no thread was specified, 1 if a thread was specified, -1 on
/// error.
int parse_wait(int fd, unsigned int *delay, unsigned int *thread_id);

void wait_for_all(sem_t *semaforo, int max_thread);

#endif // EMS_PARSER_H
