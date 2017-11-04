#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>

#include "defines.h"
#include "httpd.h"

client_t *g_clients = NULL;
uint8_t g_running = 1;

void usage(char **argv) {
  fprintf(stderr, "usage: %s ip port\n", argv[0]);
}

void exit_handler() {
  LOG_DEBUG("cleaning up...%s\n", "");
  g_running = 0;
  delete_all_clients(&g_clients);
}

// TODO: Manage zip compression
// TODO: Manage calling shell command as backend methods
// TODO: Manage CORS headers
int main(int argc, char **argv) {
  if (argc != 3) {
    usage(argv);
    return ERROR;
  }
  option_t options;
  options.address = argv[1];
  // Retrieve port number
  char *endptr;
  options.portno = strtol(argv[2], &endptr, 10);
  if (argv[2] == endptr || options.portno > MAX_PORT_NO) {
    LOG_ERROR("Invalid port: %s\n", argv[2]);
    return ERROR;
  }
  atexit(exit_handler);
  signal(SIGTERM, exit_handler);
  signal(SIGINT, exit_handler);
  int16_t socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0) {
    perror("socket");
    return ERROR;
  }
  struct sockaddr_in addr;
  if (create_addr(options, &addr)) {
    return ERROR;
  }
  if (prepare_socket(socketfd, addr) < 0) return ERROR;
  struct pollfd fds[MAX_CLIENT];
  // The socket file descriptor will always be the first one in the list
  add_client(fds, socketfd, NULL, &g_clients);
  LOG_MSG("listening to %s %u\n", options.address, options.portno);
  while (g_running) {
    // TODO: improve performance by keeping count at this level
    poll_(fds, count_clients(g_clients));
    serve(fds, g_clients);
  }
  close(socketfd);
  return 0;
}