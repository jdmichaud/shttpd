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

#include "defines.h"
#include "httpd.h"

int16_t g_socketfd;

void usage(char **argv) {
  fprintf(stderr, "usage: %s ip port\n", argv[0]);
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
  g_socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (g_socketfd < 0) {
    perror("socket");
    return ERROR;
  }
  struct sockaddr_in addr;
  if (create_addr(options, &addr)) {
    return ERROR;
  }
  if (prepare_socket(g_socketfd, addr) < 0) return ERROR;
  LOG_MSG("listening to %s %u\n", options.address, options.portno);
  while (1) {
    struct sockaddr client_addr;
    int16_t clientfd = serve(g_socketfd, &client_addr);
    if (clientfd < 0) return ERROR;
    handle(clientfd);
  }
  close(g_socketfd);
  return 0;
}