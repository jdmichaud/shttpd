#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "httpd.h"

#define FAIL() { \
  ++totalres; \
  fprintf(stderr, "%s failed at %s(%i)\n", __func__, __FILE__, __LINE__); \
}

int8_t test_next_token() {
  int8_t totalres = 0;
  char *s = " TEST";
  char *resc;
  int16_t res = next_token(s, &resc);
  if (res != 4 || *resc != s[1]) FAIL();

  s = " TEST   ";
  res = next_token(s, &resc);
  if (res != 4 || *resc != s[1]) FAIL();

  s = " TEST\n";
  res = next_token(s, &resc);
  if (res != 4 || *resc != s[1]) FAIL();

  s = " TEST\r\n";
  res = next_token(s, &resc);
  if (res != 4 || *resc != s[1]) FAIL();

  s = "";
  res = next_token(s, &resc);
  if (res != -1) FAIL();

  s = "\n";
  res = next_token(s, &resc);
  if (res != 0) FAIL();

  s = "\r\n";
  res = next_token(s, &resc);
  if (res != 0) FAIL();

  s = "\r\nwqeqwe";
  res = next_token(s, &resc);
  if (res != 0 || *resc != s[2]) FAIL();

  s = "\r\n wqeqwe";
  res = next_token(s, &resc);
  if (res != 0 || *resc != s[2]) FAIL();

  s = "   \r\n wqeqwe";
  res = next_token(s, &resc);
  if (res != 0 || *resc != s[5]) FAIL();

  return totalres;
}

int8_t test_end_of_header() {
  int8_t totalres = 0;
  char *s;

  s = "\n\n";
  if (end_of_header(s, 2) != 0) FAIL();

  s = " \n\n";
  if (end_of_header(s, 3) != 1) FAIL();

  s = " \r\n\r\n";
  if (end_of_header(s, 5) != 1) FAIL();

  s = "X\n\r\n";
  if (end_of_header(s, 4) != 1) FAIL();

  s = "X\r\n\n";
  if (end_of_header(s, 4) != 1) FAIL();

  s = "\n \n";
  if (end_of_header(s, 3) != -1) FAIL();

  s = "wqqwd \r\n qwdwq";
  if (end_of_header(s, 14) != -1) FAIL();

  s = "";
  if (end_of_header(s, 1) != -1) FAIL();

  s = "Hyphotesis non figo";
  if (end_of_header(s, 19) != -1) FAIL();

  return totalres;
}

int8_t test_get_extension() {
  int8_t totalres = 0;
  char *s;

  s = "hello.txt";
  if (strcmp(get_extension(s, 8), "txt")) FAIL();

  s = "hello.tx";
  if (strcmp(get_extension(s, 7), "tx")) FAIL();

  s = "hello";
  if (get_extension(s, 5) != NULL) FAIL();

  s = "hello.";
  if (get_extension(s, 6) != NULL) FAIL();

  return totalres;
}

int main() {
  return test_next_token() +
    test_end_of_header() +
    test_get_extension();
}