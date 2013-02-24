#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "macros.h"

#define LFM_HOST "ws.audioscrobbler.com"
#define PACKAGE_STRING "Serendipite 0.1"
#define API_KEY "d4ff4069de5ae7929d4037dc01313301"

#define REQUEST_TEXT                                                           \
"GET %s HTTP/1.1\r\n"                                                          \
"Host: %s\r\n"                                                                 \
"User-Agent: "                                                                 \
PACKAGE_STRING                                                                 \
"\r\n"                                                                         \
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"  \
"Accept-Language: en-US,en;q=0.5\r\n\r\n"

#define NEW_RELEASE_URL                                                        \
"/2.0/?method=user.getnewreleases"                                             \
"&user=evok3r&api_key="                                                        \
API_KEY                                                                        \
"&format=json"

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

  sprintf(*request, REQUEST_TEXT, url, host);
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

  if ((rv = getaddrinfo(LFM_HOST, "80", &h, &r))) {
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
  int rv = write(socket_fd, request, length);
  if (rv == -1) {
    int e = errno;
    LOG(LOG_CRITICAL, "write() error: %s", strerror(e));
  }
  return rv;
}

int http_read_response(int socket_fd, char** buffer, size_t* length)
{
  int rv;
  int offset = 0;
  int buffer_size = 16 * 1024;
  int read_size = 1024;
  ASSERT(buffer && length, "NULL buffer or length");

  *buffer = malloc(buffer_size);

  while((rv = read(socket_fd, *buffer + offset, read_size))) {
    offset += rv;
    if (rv == -1) {
      int e = errno;
      LOG(LOG_FATAL, "read() error: %s", strerror(e));
      return ERROR;
    }
    if (offset + read_size > buffer_size) {
      buffer_size *= 2;
      *buffer = realloc(*buffer, buffer_size);
    }
  }

  offset += rv;
  (*buffer)[offset] = 0;

  return offset;
}

int main(int argc, char* argv[])
{
  int socket_fd = http_connect(LFM_HOST);
  size_t length, request_text_length;
  char* buffer;

  http_request(NEW_RELEASE_URL, strlen(NEW_RELEASE_URL),
          LFM_HOST, strlen(LFM_HOST), &buffer, &request_text_length);

  http_send_request(socket_fd, buffer, strlen(buffer));

  free(buffer);

  http_read_response(socket_fd, &buffer, &length);

  puts(buffer);

  free(buffer);

  http_close(socket_fd);

  return OK;
}
