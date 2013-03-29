#ifndef HTTP_H
#define HTTP_H

/**
 * Connect to `host` on port 80.
 * returns a socket fd.
 */
int http_connect(const char* host);
/**
 *  Close the fd `fd`.
 */
int http_close(int fd);
/**
 * Crafts an http request for the url `url`, the host `host`, of respective size
 * `url_length` and `host_length`. `request` will be allocated, and its size
 * will be `request_length`.
 */
int http_request(const char* url, size_t url_length,
                 const char* host, size_t host_length,
                 char** request, size_t* request_length);
/**
 * Send the http request `request` of length `length` to the socket `fd`.
 */
int http_send_request(int fd, const char* request, size_t length);
/**
 * Read the response on the socket `fd`. `buffer` will be allocated, and shall
 * be freed by the user. `length` is the number of bytes read.
 */
int http_read_response(int fd, char** buffer, size_t* length);

/**
 * Return the position of the first character of the request body.
 */
size_t http_body_offset(char* buffer, size_t length);

#endif /* HTTP_H */
