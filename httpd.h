#ifndef __HTTPD_H__
#define __HTTPD_H__

#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "defines.h"

uint8_t iseol(char *s);
ssize_t end_of_header(char *s, ssize_t size);
char *get_extension(char *path, ssize_t len);
int16_t next_token(char *s, char **next);
int8_t prepare_socket(int16_t socketfd, struct sockaddr_in addr);
int8_t create_addr(option_t options, struct sockaddr_in *addr);
int8_t parse_request_line(char *request_line, request_t *request);
int8_t request_complete(request_t *request);
int8_t parse_request_line(char *request_line, request_t *request);
void free_request(request_t request);
int8_t parse_request_line(char *request_line, request_t *request);
int8_t parse_headers(char *header_lines, request_t *request);
int32_t parse_request(int8_t clientfd, request_t *request);
int8_t answer(int8_t clientfd, request_t *request, status_code_e status_code);
int16_t serve(int16_t socketfd, struct sockaddr *client_addr);
int8_t preprocess_path(char *path, ssize_t pathsize, request_t *request);
int8_t handle(int8_t clientfd);
int8_t sendfile_(int16_t clientfd, request_t *request);

#endif // __HTTPD_H__