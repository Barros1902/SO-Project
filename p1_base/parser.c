#include "parser.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "constants.h"
#include "operations.h"





int parse_start(int fd, int fd_out){

    while (1) {
    unsigned int event_id, delay;
    size_t num_rows, num_columns, num_coords;
    size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];


    switch (get_next(fd)) {
      case CMD_CREATE:
        if (parse_create(fd, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }
        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fd, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fd, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(event_id, fd_out)) {
          fprintf(stderr, "Failed to show event\n");
        }

        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events(fd_out)) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(fd, &delay, NULL) == -1) {  // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        write(fd_out,
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n", HELP_SIZE);

        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;

      case EOC:
        
        return 0;
    }
  }
  }










static int read_uint(int fd, unsigned int *value, char *next) {
  char buf[16];

  int i = 0;
  while (1) {
    if (read(fd, buf + i, 1) == 0) {
      *next = '\0';
      break;
    }

    *next = buf[i];

    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }

    i++;
  }

  unsigned long ul = strtoul(buf, NULL, 10);

  if (ul > UINT_MAX) {
    return 1;
  }

  *value = (unsigned int)ul;

  return 0;
}

static void cleanup(int fd) {
  char ch;
  while (read(fd, &ch, 1) == 1 && ch != '\n')
    ;
}

enum Command get_next(int fd) {
  char buf[16];
  if (read(fd, buf, 1) != 1) {
    return EOC;
  }

  switch (buf[0]) {
    case 'C':
      if (read(fd, buf + 1, 6) != 6 || strncmp(buf, "CREATE ", 7) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_CREATE;

    case 'R':
      if (read(fd, buf + 1, 7) != 7 || strncmp(buf, "RESERVE ", 8) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_RESERVE;

    case 'S':
      if (read(fd, buf + 1, 4) != 4 || strncmp(buf, "SHOW ", 5) != 0) {
        cleanup(fd);

        return CMD_INVALID;
      }

      return CMD_SHOW;

    case 'L':
      if (read(fd, buf + 1, 3) != 3 || strncmp(buf, "LIST", 4) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      if (read(fd, buf + 4, 1) != 0 && buf[4] != '\n') {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_LIST_EVENTS;

    case 'B':
      if (read(fd, buf + 1, 6) != 6 || strncmp(buf, "BARRIER", 7) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      if (read(fd, buf + 7, 1) != 0 && buf[7] != '\n') {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_BARRIER;

    case 'W':
      if (read(fd, buf + 1, 4) != 4 || strncmp(buf, "WAIT ", 5) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_WAIT;

    case 'H':
      if (read(fd, buf + 1, 3) != 3 || strncmp(buf, "HELP", 4) != 0) {
        cleanup(fd);
        return CMD_INVALID;
      }

      if (read(fd, buf + 4, 1) != 0 && buf[4] != '\n') {
        cleanup(fd);
        return CMD_INVALID;
      }

      return CMD_HELP;

    case '#':
      cleanup(fd);
      return CMD_EMPTY;

    case '\n':
      return CMD_EMPTY;

    default:
      cleanup(fd);
      return CMD_INVALID;
  }
}

int parse_create(int fd, unsigned int *event_id, size_t *num_rows, size_t *num_cols) {
  char ch;

  if (read_uint(fd, event_id, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    return 1;
  }

  unsigned int u_num_rows;
  if (read_uint(fd, &u_num_rows, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    return 1;
  }
  *num_rows = (size_t)u_num_rows;

  unsigned int u_num_cols;
  if (read_uint(fd, &u_num_cols, &ch) != 0 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    return 1;
  }
  *num_cols = (size_t)u_num_cols;

  return 0;
}

size_t parse_reserve(int fd, size_t max, unsigned int *event_id, size_t *xs, size_t *ys) {
  char ch;

  if (read_uint(fd, event_id, &ch) != 0 || ch != ' ') {
    cleanup(fd);
    return 0;
  }

  if (read(fd, &ch, 1) != 1 || ch != '[') {
    cleanup(fd);
    return 0;
  }

  size_t num_coords = 0;
  while (num_coords < max) {
    if (read(fd, &ch, 1) != 1 || ch != '(') {
      cleanup(fd);
      return 0;
    }

    unsigned int x;
    if (read_uint(fd, &x, &ch) != 0 || ch != ',') {
      cleanup(fd);
      return 0;
    }
    xs[num_coords] = (size_t)x;

    unsigned int y;
    if (read_uint(fd, &y, &ch) != 0 || ch != ')') {
      cleanup(fd);
      return 0;
    }
    ys[num_coords] = (size_t)y;

    num_coords++;

    if (read(fd, &ch, 1) != 1 || (ch != ' ' && ch != ']')) {
      cleanup(fd);
      return 0;
    }

    if (ch == ']') {
      break;
    }
  }

  if (num_coords == max) {
    cleanup(fd);
    return 0;
  }

  if (read(fd, &ch, 1) != 1 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    return 0;
  }

  return num_coords;
}

int parse_show(int fd, unsigned int *event_id) {
  char ch;

  if (read_uint(fd, event_id, &ch) != 0 || (ch != '\n' && ch != '\0')) {
    cleanup(fd);
    return 1;
  }

  return 0;
}

int parse_wait(int fd, unsigned int *delay, unsigned int *thread_id) {
  char ch;

  if (read_uint(fd, delay, &ch) != 0) {
    cleanup(fd);
    return -1;
  }

  if (ch == ' ') {
    if (thread_id == NULL) {
      cleanup(fd);
      return 0;
    }

    if (read_uint(fd, thread_id, &ch) != 0 || (ch != '\n' && ch != '\0')) {
      cleanup(fd);
      return -1;
    }

    return 1;
  } else if (ch == '\n' || ch == '\0') {
    return 0;
  } else {
    cleanup(fd);
    return -1;
  }


  
}
