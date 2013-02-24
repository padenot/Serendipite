#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "macros.h"

#define PACKAGE_STRING "Serendipite 0.1"

#define REQUEST_TEXT                                                           \
"GET %s HTTP/1.0\r\n"                                                          \
"User-Agent: " PACKAGE_STRING "\r\n"                                           \
"Host: %s\r\n\r\n"

void http_request(const char* url, size_t url_length, const char* host,
                  size_t host_length, char** request, size_t* request_length)
{
  ASSERT(request && request_length, "NULL request or request_length");
  /* size of the request string + size of the url
   * + size of the host - strlen("%s") * 2 + NULL byte */
  *request_length = url_length + host_length + strlen(REQUEST_TEXT) - 3;
  *request = malloc(*request_length);

  if (!*request) {
    LOG(LOG_FATAL, "OOM");
    exit(1);
  }

  snprintf(*request, *request_length, REQUEST_TEXT, url, host);

  /* remove the null byte */
  (*request_length)--;
}

int http_connect(const char* host)
{
  int socket_fd, rv;
  struct addrinfo h = {0};
  struct addrinfo* r;

  h.ai_family = AF_UNSPEC;
  h.ai_socktype = SOCK_STREAM;
  h.ai_flags = 0;
  h.ai_protocol = 0;

  if ((rv = getaddrinfo(host, "80", &h, &r))) {
    LOG(LOG_FATAL, "getaddrinfo: %s\n", gai_strerror(rv));
    return ERROR;
  }

  if ((socket_fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
    int e = errno;
    LOG(LOG_FATAL, "socket() error: %s", strerror(e));
    return ERROR;
  }

  if ((rv = connect(socket_fd, r->ai_addr, r->ai_addrlen)) == -1) {
    int e = errno;
    LOG(LOG_FATAL, "connect() error: %s", strerror(e));
    return ERROR;
  }

  freeaddrinfo(r);
  return socket_fd;
}

int http_close(int fd)
{
  return close(fd);
}

int http_send_request(int socket_fd, const char* request, size_t length)
{
  int rv;
  size_t offset = 0;

  rv = write(socket_fd, request, length);

  if (rv != length) {
    int e = errno;
    LOG(LOG_CRITICAL, "write() error: %s", strerror(e));
    return -1;
  }
  return offset;
}

int http_read_response(int socket_fd, char** buffer, size_t* length)
{
  int rv;
  int offset = 0;
  int buffer_size = 16 * 1024;
  int read_size = 1024;
  ASSERT(buffer && length, "NULL buffer or length");

  *buffer = malloc(buffer_size);

  while((rv = read(socket_fd, (*buffer) + offset, read_size)) > 0) {
    offset += rv;
    if (offset + read_size > buffer_size) {
      buffer_size *= 2;
      *buffer = realloc(*buffer, buffer_size);
    }
  }

  if (rv == -1) {
    int e = errno;
    LOG(LOG_FATAL, "read() error: %s", strerror(e));
    return ERROR;
  }

  offset += rv;
  *length = offset;
  (*buffer)[offset] = 0;

  return OK;
}

size_t http_body_offset(char* buffer, size_t length)
{
  char* b = buffer;

  while(length-- > 3) {
    if (*buffer == '\r' && *(buffer+1) == '\n' &&
        *(buffer+2) == '\r' && *(buffer+3) == '\n') {
      return buffer - b + 4;
    }
    buffer++;
  }
  LOG(LOG_WARNING, "No body found in http response.");
  return 0;
}
