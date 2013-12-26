#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>

#include "external/utf8.h"
#include "http.h"
#include "json.h"
#include "macros.h"

#define LFM_HOST "ws.audioscrobbler.com"
#define NEW_RELEASE_URL                                                        \
"/2.0/?method=user.getnewreleases"                                             \
"&user=evok3r&api_key="                                                        \
API_KEY                                                                        \
"&format=json"

#define API_KEY "d4ff4069de5ae7929d4037dc01313301"

struct artist_album_state {
  int lastfm_api_state;
  char buffer[512];
  size_t len_album;
  char* result[30];
  int result_index;
  int got_name;
};

/* state to know what we want */
typedef enum {
  NOTHING,
  ARTIST,
  ALBUM
} lastfm_api_state;

int httpreq(char** buffer, size_t* length)
{
  int socket_fd;
  size_t request_text_length;

  if ((socket_fd = http_connect(LFM_HOST)) < 0) {
    LOG(LOG_CRITICAL, "Could not connect to %s.", LFM_HOST);
    exit(ERROR);
  }

  http_request(NEW_RELEASE_URL, strlen(NEW_RELEASE_URL),
               LFM_HOST, strlen(LFM_HOST), buffer, &request_text_length);

  if (http_send_request(socket_fd, *buffer, request_text_length) == -1) {
    LOG(LOG_CRITICAL, "Could not send request.");
    exit(ERROR);
  }

  free(*buffer);

  if (http_read_response(socket_fd, buffer, length)) {
    LOG(LOG_CRITICAL, "Could not read request");
    exit(ERROR);
  }

  http_close(socket_fd);

  return OK;
}

void artist_album_cb(json_value* v, void* user_ptr)
{
  struct artist_album_state * s = (struct artist_album_state *) user_ptr;
  if (v->kv == JSON_KEY && v->length && strncmp(v->buffer + v->offset, "name", v->length) == 0) {
    s->got_name = 1;
    switch(s->lastfm_api_state) {
      case NOTHING:
        s->lastfm_api_state = ALBUM;
        break;
      case ALBUM:
        s->lastfm_api_state = ARTIST;
        break;
      case ARTIST:
        s->lastfm_api_state = NOTHING;
    }
  }

  if (v->kv == JSON_VALUE && s->got_name) {
    s->got_name = 0;
    if (s->lastfm_api_state == ALBUM) {
      ASSERT((v->length + 5) < 512, "Not enough room in the buffer to put the album name");
      s->len_album = v->length;
      strncpy(s->buffer, v->buffer + v->offset, v->length);
      strncpy(s->buffer + s->len_album, " ", strlen(" "));
      s->len_album += strlen(" ");
    } else if (s->lastfm_api_state == ARTIST) {
      ASSERT((s->len_album + v->length) < 512, "Not enough room in the buffer to put the artist name");
      ASSERT(s->result_index < 30, "implement realloc for callback");

      strncpy(s->buffer + s->len_album, v->buffer + v->offset, v->length);
      s->buffer[s->len_album + v->length] = 0;

      u8_unescape(s->buffer, 512, s->buffer);

      s->result[s->result_index] = malloc(strlen(s->buffer) + 1);
      strcpy(s->result[s->result_index], s->buffer);
      s->result_index++;

      s->lastfm_api_state = NOTHING;
    }
  }
}

int main(int argc, char ** argv)
{
  char* buffer;
  size_t length, body_pos;
  struct artist_album_state state = {0};
  int i;

  setlocale(LC_ALL, "");

  httpreq(&buffer, &length);

  body_pos = http_body_offset(buffer, length);

  LOG(LOG_OK, "New relases:");
  parse_json(buffer + body_pos, length - body_pos, (void*)&state, artist_album_cb);

  for (i = 0; i < state.result_index; i++) {
    printf("%s\n", state.result[i]);
  }

  free(buffer);

  for (i = 0; i < state.result_index; i++) {
   free(state.result[i]);
  }
  return 0;
}
