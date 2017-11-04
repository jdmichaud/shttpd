#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdint.h>
#include <string.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#define ERROR -1
#define ERR_NO_SUCH_FILE -2
#define ERR_ACCESS -3
#define ERR_UNKNOWN_METHOD -4
#define FD_CLOSED -5
#define MAX_PORT_NO 0xFFFF
#define BUFFER_SIZE 4096
#define SOCKET_INDEX 0
#define MAX_CLIENT 1024

/** Some useful macro */

#define LOG_MSG(format, ...) { fprintf(stdout, format, __VA_ARGS__); }
#define LOG_ERROR(format, ...) { fprintf(stderr, "Error:"format, __VA_ARGS__); }
#define LOG_WARNING(format, ...) { fprintf(stderr, "Warning:"format, __VA_ARGS__); }
#define LOG_DEBUG(format, ...) \
  { fprintf(stderr, "%s(%d) - %s: "format, __FILE__, __LINE__, __func__, __VA_ARGS__); }

//#define LOG_DEBUG(format, ...) {;}

// Days of the week 3-letters abbreviations
static const char dow[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
// Months of the year 3-letters abbreviations
static const char moy[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/** HTTP related */

#define GET 0
#define HEAD 1
#define NB_METHODS 2

static const char g_methods[][5] = { "GET", "HEAD" };

#define ACCEPT 0
#define ACCEPT_CHARSET 1
#define ACCEPT_ENCODING 2
#define ACCEPT_LANGUAGE 3
#define ACCEPT_DATETIME 4
#define ACCESS_CONTROL_REQUEST_METHOD 5
#define ACCESS_CONTROL_REQUEST_HEADERS 6
#define AUTHORIZATION 7
#define CACHE_CONTROL 8
#define CONNECTION 9
#define COOKIE 10
#define CONTENT_LENGTH 11
#define CONTENT_MD5 12
#define CONTENT_TYPE 13
#define DATE 14
#define EXPECT 15
#define FORWARDED 16
#define FROM 17
#define HOST 18
#define IF_MATCH 19
#define IF_MODIFIED_SINCE 20
#define IF_NONE_MATCH 21
#define IF_RANGE 22
#define IF_UNMODIFIED_SINCE 23
#define MAX_FORWARDS 24
#define PRAGMA 25
#define PROXY_AUTHORIZATION 26
#define RANGE 27
#define REFERER 28
#define TE 29
#define USER_AGENT 30
#define UPGRADE 31
#define X_REQUESTED_WITH 32
#define DNT 33
#define X_FORWARDED_FOR 34
#define X_FORWARDED_HOST 35
#define X_FORWARDED_PROTO 36
#define FRONT_END_HTTPS 37
#define X_HTTP_METHOD_OVERRIDE 38
#define X_ATT_DEVICEID 39
#define X_WAP_PROFILE 40
#define PROXY_CONNECTION 41
#define X_UIDH 42
#define X_CSRF_TOKEN 43
#define X_REQUEST_ID 44
#define X_CORRELATION_ID 45

#define NB_HEADERS 47

static const char g_headers[][31] = {
  "Accept",
  "Accept-Charset",
  "Accept-Encoding",
  "Accept-Language",
  "Accept-Datetime",
  "Access-Control-Request-Method",
  "Access-Control-Request-Headers",
  "Authorization Authentication",
  "Cache-Control",
  "Connection",
  "Cookie",
  "Content-Length",
  "Content-MD5",
  "Content-Type",
  "Date",
  "Expect",
  "Forwarded",
  "From",
  "Host",
  "If-Match",
  "If-Modified-Since",
  "If-None-Match",
  "If-Range",
  "If-Unmodified-Since",
  "Max-Forwards",
  "Pragma",
  "Proxy-Authorization",
  "Range",
  "Referer",
  "TE",
  "User-Agent",
  "Upgrade",
  "X-Requested-With",
  "DNT",
  "X-Forwarded-For",
  "X-Forwarded-Host",
  "X-Forwarded-Proto",
  "Front-End-Https",
  "X-Http-Method-Override",
  "X-ATT-DeviceId",
  "X-Wap-Profile",
  "Proxy-Connection",
  "Proxy-Connection",
  "X-UIDH",
  "X-Csrf-Token",
  "X-Request-ID",
  "X-Correlation-ID",
};

#define HTTP_1_0 0
#define HTTP_1_1 1
#define NB_VERSION 2

static const char g_version[][9] = { "HTTP/1.0", "HTTP/1.1" };

typedef enum {
  _200 = 0,
  _400,
  _403,
  _404,
  _500,
  _501,
} status_code_e;

typedef struct {
  uint16_t code;
  char *message;
} status_code_t;

#define NB_STATUS_CODE 5

static const status_code_t g_status_code[] = {
  { 200, "OK" },
  { 400, "Bad Request" },
  { 403, "Forbidden" },
  { 404, "Not Found" },
  { 500, "Internal Server Error" },
  { 501, "Not Implemented" },
};

typedef enum {
  E_GET = 0,
  E_POST
} method_e;

typedef struct {
  char* type;
  char *value;
} extra_header_t;

typedef enum {
  E_HTTP_1_0 = 0,
  E_HTTP_1_1
} http_version_e;

typedef struct {
  method_e method;
  char *path;
  http_version_e http_version;
  char *headers[NB_HEADERS];
  extra_header_t *extra_headers;
  uint32_t nb_extra_headers;
  char *body;
} request_t;

/** End of HTTP related */

typedef struct {
  char *address;
  uint32_t portno;
} option_t;

typedef struct client_s {
  struct sockaddr *client_addr;
  int16_t clientfd;
  struct client_s *next;
} client_t;

extern client_t *g_clients;

#endif // __DEFINES_H__
