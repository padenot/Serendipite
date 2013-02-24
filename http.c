#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "macros.h"

const char* LFM_HOST = "ws.audioscrobbler.com";

#define PACKAGE_STRING "Serendipite 0.1"

#define API_KEY "d4ff4069de5ae7929d4037dc01313301"

#define request_text "GET %s HTTP/1.1\r\n" \
                     "Host: ws.audioscrobbler.com\r\n" \
                     "User-Agent: " \
                     PACKAGE_STRING \
                     "\r\n" \
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" \
                     "Accept-Language: en-US,en;q=0.5\r\n\r\n"

#define new_release_url "/2.0/?method=user.getnewreleases&user=evok3r&api_key=" API_KEY "&format=json"

int request(const char* url, size_t length, char** request, size_t* request_length)
{
  ASSERT(request && request_length, "NULL request or request_length");
  /* size of the request boilerplate + size of the url - strlen("%s") + NULL byte */
  *request_length = length + strlen(request_text) - 1;
  *request = malloc(*request_length);

  if (!*request) {
    LOG(LOG_FATAL, "OOM");
    exit(1);
  }

  LOG(LOG_DEBUG, "%s", request_text);

  sprintf(*request, request_text, url);
  LOG(LOG_DEBUG, "%s", *request);

  return 0;
}

int socket_connect()
{
  int socket_fd, s;
  struct addrinfo h = {0};
  struct addrinfo* r;

  h.ai_family = AF_UNSPEC;
  h.ai_socktype = SOCK_STREAM;
  h.ai_flags = 0;
  h.ai_protocol = 0;

  if ((s = getaddrinfo(LFM_HOST, "80", &h, &r))) {
    LOG(LOG_FATAL, "getaddrinfo: %s\n", gai_strerror(s));
    return ERROR;
  }

  if ((socket_fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
    int e = errno;
    LOG(LOG_FATAL, "socket() error: %s", strerror(e));
    return ERROR;
  }

  if ((s = connect(socket_fd, r->ai_addr, r->ai_addrlen)) == -1) {
    int e = errno;
    LOG(LOG_FATAL, "connect() error: %s", strerror(e));
    return ERROR;
  }

  freeaddrinfo(r);
  return socket_fd;
}

int socket_close(int fd)
{
  return close(fd);
}

int socket_send_request(int socket_fd, const char* request, size_t length)
{
  int rv = write(socket_fd, request, length);
  if (rv == -1) {
    int e = errno;
    LOG(LOG_CRITICAL, "write() error: %s", strerror(e));
  }
  return rv;
}

int socket_read_response(int socket_fd, char** buffer, size_t* length)
{
  int rv;
  int offset = 0;
  int buffer_size = 16 * 1024;
  int read_size = 1024;
  ASSERT(buffer && length, "NULL buffer or length");

  *buffer = malloc(16 * 1024);

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

  if (rv == -1) {
    int e = errno;
    LOG(LOG_CRITICAL, "read() error: %s", strerror(e));
  }

  offset += rv;
  (*buffer)[offset] = 0;

  return offset;
}

int main(int argc, char* argv[])
{
  int socket_fd = socket_connect();
  size_t length, request_text_length;
  char* buffer;

  request(new_release_url, strlen(new_release_url), &buffer, &request_text_length);

  socket_send_request(socket_fd, buffer, strlen(buffer));

  free(buffer);

  socket_read_response(socket_fd, &buffer, &length);

  puts(buffer);

  free(buffer);

  socket_close(socket_fd);

  return OK;
}
