#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <time.h>

#include "httpd.h"
#include "defines.h"
#include "mime.h"

#define POSIX_SPACES " \f\n\r\t\v";

inline uint8_t iseol(char *s) {
  if (s[0] == '\n') return 1;
  if (s[0] == '\r') return 2;
  return 0;
}

/**
 * Check if the string contains two consecusive end of line.
 * Manage Unix-style and Windows-style end of line.
 */
inline ssize_t end_of_header(char *s, ssize_t size) {
  while (size >= 2) {
    if (size >= 4)
      if (s[size - 4] == '\r' && s[size - 3] == '\n' &&
          s[size - 2] == '\r' && s[size - 1] == '\n')
        return size - 4;
    if (size >= 3)
      if ((s[size - 3] == '\r' && s[size - 2] == '\n' && s[size - 1] == '\n') ||
        (s[size - 3] == '\n' && s[size - 2] == '\r' && s[size - 1] == '\n'))
        return size - 3;
    if (s[size - 1] == '\n' && s[size - 2] == '\n')
      return size - 2;
    --size;
  }
  return -1;
}

char *get_extension(char *path, ssize_t len) {
  if (len <= 0) return NULL;
  for (ssize_t i = len - 1; i > 0; --i)
    if (path[i] == '.' && path[i + 1] != 0) return &path[i + 1];
  return NULL;
}

/**
 * Returns the next token in a string. A token is a list of characters that
 * are not a spaces, a line feeds, a carriage returns or the character 0
 * returns the length of the token in bytes and *next points to the beginning of
 * the token
 * if stop_at_eol is different from 0, next token will return 0 if eol is
 * encountered
 * returns 0 if a line feed or a carriage return is found. *next will be
 * pointing to the next token
 * returns -1 if end of line is reached
 */
int16_t next_token(char *s, char **next) {
  char *head = s;
  uint8_t eol = 0;
  while (*head && (isspace(*head)) && !iseol(head)) ++head;
  while (*head && iseol(head)) {
    eol |= iseol(head);
    ++head;
  }
  *next = head;
  if (eol) return 0;
  if (*head == 0) return -1;
  while (*head && !isspace(*head) && !iseol(head)) ++head;
  return head - *next;
}

int8_t create_addr(option_t options, struct sockaddr_in *addr) {
  // If localhost is provided, convert it to 127.0.0.1,
  // otherwise just use the address provided
  if (!strcmp(options.address, "localhost")) {
    memcpy(options.address, "127.0.0.1", 9);
  }
  // Retrieve address
  struct in_addr inp;
  if (!inet_aton(options.address, &inp)) {
    LOG_ERROR("Invalid address: %s\n", options.address);
    return ERROR;
  }
  // Assign a name to the socket
  memset((void *) addr, 0, sizeof (addr));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inp.s_addr;
  addr->sin_port = htons(options.portno);

  return 0;
}

int8_t prepare_socket(int16_t socketfd, struct sockaddr_in addr) {
  // Set the address/port associated to that socket reusable
  uint8_t t = 1;
  setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof (uint8_t));
  // Bind the address to the socket
  if (bind(socketfd, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) != 0) {
    perror("bind");
    return ERROR;
  }
  // Listen only to one client
  if (listen(socketfd, 1)) {
    perror("listen");
    return ERROR;
  }
  return 0;
}

int8_t request_complete(request_t *request) {
  return request->path != NULL;
}

void free_request(request_t request) {
  if (request.path != NULL) free(request.path);
  if (request.body != NULL) free(request.body);
  for (uint8_t i = 0; i < NB_HEADERS; ++i) {
    if (request.headers[i] != NULL) free(request.headers[i]);
  }
  // TODO: clear extra headers
}

int8_t parse_request_line(char *request_line, request_t *request) {
  uint8_t i;
  char *token;
  int16_t tokensize;
  tokensize = next_token(request_line, &token);
  if (tokensize <= 0) {
    LOG_ERROR("Wrongly formed request line: %s\n", request_line);
    return ERROR;
  }
  for (i = 0; i < NB_METHODS; ++i) {
    if (!strncmp(g_methods[i], token, tokensize)) break;
  }
  if (i >= NB_METHODS) {
    LOG_ERROR("Unknown method: %s\n", request_line);
    return ERR_UNKNOWN_METHOD;
  } else request->method = i;
  // Parse the path
  tokensize = next_token(&token[tokensize], &token);
  if (tokensize <= 0) {
    LOG_ERROR("Wrongly formed request line: %s\n", request_line);
    return ERROR;
  }
  if (preprocess_path(token, tokensize, request) != 0) return ERROR;
  // Parse the HTTP version
  tokensize = next_token(&token[tokensize], &token);
  if (tokensize < 0) {
    LOG_ERROR("Wrongly formed request line: %s\n", request_line);
    return ERROR;
  }
  if (tokensize == 0) {
    fprintf(stderr, "Warning: version not provided\n");
    return token - request_line;
  }
  for (i = 0; i < NB_VERSION; ++i)
    if (!strncmp(g_version[i], token, tokensize)) break;
  if (i >= NB_VERSION) {
    LOG_WARNING("Unknown version: %s\n", request_line);
  }
  else request->http_version = i;
  tokensize = next_token(&token[tokensize], &token);
  return token - request_line;
}

int8_t parse_headers(char *header_lines, request_t *request) {
  ssize_t eoh = end_of_header(header_lines, strnlen(header_lines, BUFFER_SIZE));
  if (eoh < 0) return ERROR;
  char *token = header_lines;
  char *type;
  char *value;
  int16_t typesize = 0;
  int16_t valuesize = 0;
  while (token - header_lines < eoh) {
    typesize = next_token(&token[0], &token);
    if (typesize < 0) return ERROR;
    type = token;
    valuesize = next_token(&token[typesize], &token);
    if (valuesize < 0) return ERROR;
    value = strndup(token, valuesize);
    uint8_t i;
    for (i = 0; i < NB_HEADERS; ++i)
      if (!strncasecmp(g_headers[i], type, typesize - 1)) break;
    if (i >= NB_HEADERS) {
      // TODO: handle extra headers
      free(value);
    } else {
      request->headers[i] = value;
    }
    // Pop the end of line
    // TODO: Manage multi-line headers
    next_token(&token[valuesize], &token);
  }
  return token - header_lines;
}

int32_t parse_request(int8_t clientfd, request_t *request) {
  if (request == NULL) return ERROR;
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE * sizeof (char));
  ssize_t totallen = 0;
  while (end_of_header(buffer, totallen) < 0) {
    ssize_t len = read(clientfd, &buffer[totallen], BUFFER_SIZE - totallen);
    if (len < 0) {
      perror("read");
      return ERROR;
    }
    if (len == 0) return ERROR;
    totallen += len;
  }
  ssize_t bytes_parsed = 0;
  ssize_t total_bytes_parsed = 0;
  if ((bytes_parsed = parse_request_line(buffer, request)) <= 0) return bytes_parsed;
  total_bytes_parsed += bytes_parsed;
  if ((bytes_parsed = parse_headers(&buffer[bytes_parsed], request)) <= 0) return ERROR;
  total_bytes_parsed += bytes_parsed;
  return total_bytes_parsed;
}

int8_t answer(int8_t clientfd, request_t *request, status_code_e status_code) {
  LOG_DEBUG("sending back code %i %s\n", g_status_code[status_code].code,
    g_status_code[status_code].message);
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE * sizeof (char));
  ssize_t len = snprintf(buffer, BUFFER_SIZE, "%s %i %s\n",
    g_version[request->http_version], g_status_code[status_code].code,
    g_status_code[status_code].message);
  if (write(clientfd, buffer, len) < 0) {
    perror("write");
    return ERROR;
  }
  return 0;
}

int16_t serve(int16_t socketfd, struct sockaddr *client_addr) {
  socklen_t socklen = sizeof (struct sockaddr);
  // Wait for a client to connect
  int16_t clientfd = accept(socketfd, client_addr, &socklen);
  if (clientfd < 0) {
    perror("accept");
    return ERROR;
  }
  return clientfd;
}

int8_t preprocess_path(char *path, ssize_t pathsize, request_t *request) {
  if (!strncmp(path, "/", pathsize)) {
    request->path = strndup("index.html", 10);
  } else if (path[0] == '/') {
    request->path = strndup(&path[1], pathsize - 1);
  }
  if (request->path == NULL) {
    perror("strndup");
    return ERROR;
  }
  return 0;
}

// TODO: refactor that beast of a function!
int8_t sendfile_(int16_t clientfd, request_t *request) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE * sizeof (char));
  int16_t filefd = open(request->path, O_RDONLY);
  if (filefd < 0) {
    if (errno == EACCES) {
      answer(clientfd, request, _403);
      return ERROR;
    } else if (errno == ENOENT) {
      answer(clientfd, request, _404);
      return ERROR;
    }
    answer(clientfd, request, _500);
    return ERROR;
  }
  ssize_t filesize = lseek(filefd, 0, SEEK_END);
  if (filesize < 0) {
    perror("lseek");
    return ERROR;
  }
  // Be kind, rewind
  lseek(filefd, 0, SEEK_SET);
  // Some headers
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  ssize_t position = 0;
  position = snprintf(buffer, BUFFER_SIZE, "HTTP/1.0 200 OK\n");
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "Server: shttpd/%i.%i.%i\n",
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "Date: %s, %i %s %i %i:%i:%i GMT\n",
    dow[tm.tm_wday], tm.tm_mday, moy[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "Content-type: %s\n", get_mime_type(get_extension(request->path, strlen(request->path))));
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "Content-length: %lu\n", filesize);
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "Last-Modified: %s, %i %s %i %i:%i:%i GMT\n",
    dow[tm.tm_wday], tm.tm_mday, moy[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
  position += snprintf(buffer + position,
    BUFFER_SIZE - position,
    "\n");
  size_t tlen = 0;
  while (tlen < strnlen(buffer, BUFFER_SIZE)) {
    ssize_t len = write(clientfd, buffer, position);
    if (len < 0) {
      perror("write");
      return ERROR;
    }
    tlen += len;
  }
  if (request->method == GET) {
    // Sending file
    LOG_DEBUG("Sending %s\n", request->path);
    ssize_t bytesent = sendfile(clientfd, filefd, 0, filesize);
    if (bytesent < 0) {
      perror("sendfile");
      return ERROR;
    }
    LOG_DEBUG("%li bytes sent\n", bytesent);
  }
  return 0;
}

int8_t handle(int8_t clientfd) {
  request_t request;
  memset(&request, 0, sizeof (request));
  int32_t ret = 0;
  if ((ret = parse_request(clientfd, &request)) > 0) {
    LOG_DEBUG("%s %s\n", g_methods[request.method], request.path);
    for (uint8_t i = 0; i < NB_HEADERS; ++i)
      if (request.headers[i])
        LOG_DEBUG("%s: %s\n", g_headers[i], request.headers[i]);
    sendfile_(clientfd, &request);
  } else {
    switch (ret) {
    case ERR_UNKNOWN_METHOD:
      answer(clientfd, &request, _501);
      break;
    default:
      answer(clientfd, &request, _500);
    }
  }
  // TODO: manage keep-alive
  close(clientfd);
  free_request(request);
  return 0;
}
