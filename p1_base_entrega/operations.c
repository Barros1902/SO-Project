#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "constants.h"
#include "eventlist.h"


static struct EventList *event_list = NULL;
static unsigned int state_access_delay_ms = 0;
/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
    return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory
/// resource.
/// @param event_id The ID of the event to get.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event *get_event_with_delay(unsigned int event_id) {
    struct timespec delay = delay_to_timespec(state_access_delay_ms);
    nanosleep(&delay, NULL); // Should not be removed
    return get_event(event_list, event_id);
}

/// Gets the seat with the given index from the state.
/// @note Will wait to simulate a real system accessing a costly memory
/// resource.
/// @param event Event to get the seat from.
/// @param index Index of the seat to get.
/// @return Pointer to the seat.
static unsigned int *get_seat_with_delay(struct Event *event, size_t index) {
    struct timespec delay = delay_to_timespec(state_access_delay_ms);
    nanosleep(&delay, NULL); // Should not be removed

    return &event->data[index];
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event *event, size_t row, size_t col) {
    return (row - 1) * event->cols + col - 1;
}

int ems_init(unsigned int delay_ms) {
    if (event_list != NULL) {
        fprintf(stderr, "EMS state has already been initialized\n");
        return 1;
    }

    event_list = create_list();
    pthread_rwlock_init(&(event_list->rw_event_list),NULL);
    state_access_delay_ms = delay_ms;

    return event_list == NULL;
}

int ems_terminate() {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    free_list(event_list);
    return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {

    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }


    pthread_rwlock_wrlock(&(event_list->rw_event_list));
    if (get_event_with_delay(event_id) != NULL) {
        fprintf(stderr, "Event already exists\n");
        pthread_mutex_unlock(&(event_list->mutex_event_list));
        return 1;
    }


    struct Event *event = malloc(sizeof(struct Event));
    pthread_mutex_init(&event->mutex_event,NULL);

    if (event == NULL) {
        fprintf(stderr, "Error allocating memory for event\n");
        pthread_mutex_unlock(&(event_list->mutex_event_list));
        return 1;
    }

    event->id = event_id;
    event->rows = num_rows;
    event->cols = num_cols;
    event->reservations = 0;
    event->data = malloc(num_rows * num_cols * sizeof(unsigned int));
    
    

    if (event->data == NULL) {
        fprintf(stderr, "Error allocating memory for event data\n");
        free(event);
        pthread_rwlock_unlock(&(event_list->rw_event_list));
        return 1;
    }

    for (size_t i = 0; i < num_rows * num_cols; i++) {
        event->data[i] = 0;
    }


    if (append_to_list(event_list, event) != 0) {
        fprintf(stderr, "Error appending event to list\n");
        free(event->data);
        free(event);

        pthread_rwlock_unlock(&(event_list->rw_event_list)); 
        return 1;
    }

    pthread_rwlock_unlock(&(event_list->rw_event_list));
    return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs,size_t *ys) {
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }
    
    pthread_rwlock_rdlock(&(event_list->rw_event_list));
    struct Event *event = get_event_with_delay(event_id);
    if (event == NULL) {
        fprintf(stderr, "Event not found\n");
        pthread_rwlock_unlock(&(event_list->rw_event_list));
        return 1;
    }
    pthread_rwlock_unlock(&(event_list->rw_event_list));

    pthread_mutex_lock(&(event->mutex_event));
    unsigned int reservation_id = ++event->reservations;
    
    
    size_t i = 0;
    
    for (; i < num_seats; i++) {
       
        size_t row = xs[i];
        size_t col = ys[i];

        if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
            pthread_mutex_unlock(&(event->mutex_event));
            fprintf(stderr, "Invalid seat\n");
            break;
        }

        if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0) {
            pthread_mutex_unlock(&(event->mutex_event));
            fprintf(stderr, "Seat already reserved\n");
            break;
        }
        

        *get_seat_with_delay(event, seat_index(event, row, col)) =
            reservation_id;
    }

    // If the reservation was not successful, free the seats that were reserved.

    if (i < num_seats) {
        event->reservations--;
        for (size_t j = 0; j < i; j++) {
            *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
        }
        pthread_mutex_unlock(&(event->mutex_event));
        return 1;
    }
    pthread_mutex_unlock(&(event->mutex_event));
    return 0;
}

int ems_show(unsigned int event_id, int fd_out) {
     printf("CCCCCCCCC");
    if (event_list == NULL) {
        fprintf(stderr, "EMS state must be initialized\n");
        return 1;
    }

    pthread_rwlock_rdlock(&(event_list->rw_event_list));
    struct Event *event = get_event_with_delay(event_id);

    if (event == NULL) {
        fprintf(stderr, "Event not found\n");
        pthread_rwlock_unlock(&(event_list->rw_event_list));
        return 1;
    }
     printf("BBBBBBBBB");
    pthread_rwlock_unlock(&(event_list->rw_event_list));
    printf("AAAAAAA");
    pthread_mutex_lock(&(event->mutex_event));
    
    size_t len = 1;
    for (size_t i = 1; i <= event->rows; i++) {
        for (size_t j = 1; j <= event->cols; j++) {
            unsigned int *seat = get_seat_with_delay(event, seat_index(event, i, j));
            char seat_string[BUFFER_SIZE];
            sprintf(seat_string, "%u", *seat);

            ssize_t bytes_written = write(fd_out, seat_string, len);
            (void)bytes_written;

            if (j < event->cols) {
                
                write(fd_out, " ", len);
                
            }
        }
        write(fd_out, "\n ", len);
        
    }
    pthread_mutex_unlock(&(event->mutex_event));
    return 0;
}

int ems_list_events(int fd_out) {
    if (event_list == NULL) {
        write(fd_out, "EMS state must be initialized\n", 31);
        return 1;
    }

    pthread_rwlock_rdlock(&(event_list->rw_event_list));
    
    if (event_list->head == NULL) {
        write(fd_out, "No events\n", 10);
        pthread_rwlock_unlock(&(event_list->rw_event_list));
        return 0;
    }
    
    struct ListNode *current = event_list->head;

    pthread_mutex_lock(&(event_list->mutex_event_list));
    while (current != NULL) {
        printf("estou no while");
        write(fd_out, "Event: ", 7);
        char event_string[BUFFER_SIZE];
        sprintf(event_string, "%u", (current->event)->id);
        write(fd_out, event_string, strlen(event_string));
        write(fd_out, "\n ", 1);
        current = current->next;
    }

    pthread_mutex_unlock(&(event_list->mutex_event_list));
    pthread_rwlock_unlock(&(event_list->rw_event_list));
    printf("estou na list");

    return 0;
}

void ems_wait(unsigned int delay_ms) {
    struct timespec delay = delay_to_timespec(delay_ms);
    nanosleep(&delay, NULL);
}
