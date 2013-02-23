#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"
#include "json.h"

const int BUFFER_SIZE = 64 * 1024;

void json_callback(json_value* v) {
  switch(v->type) {
    case JSON_STR: {
      char * str = malloc(v->length + 1);
      strncpy(str, v->buffer + v->offset, v->length);
      str[v->length] = 0;

      LOG(LOG_DEBUG, "%s: offset(%zu) length(%zu) depth(%zu): \"%s\" (%s)", type2str[v->type], v->offset, v->length, v->depth, str, v->kv == JSON_KEY ? "key" : "value");
      free(str);
      break;
    };
    case JSON_OBJECT: {
      LOG(LOG_DEBUG, "%s: offset(%zu) depth(%zu)", type2str[v->type], v->offset, v->depth);
      break;
    };
    case JSON_ARRAY: {
      //LOG(LOG_DEBUG, "%s: offset(%d), depth(%d), index(%d)");
      break;
    };
  }
}

/* state to know what we want */
typedef enum {
  NOTHING,
  ARTIST,
  ALBUM
} lastfm_api_state;

void print(char* str, size_t len)
{
  putc('"', stdout);
  while(len--) {
    putc(*str++, stdout);
  }
  putc('"', stdout);
  putc('\n', stdout);
}

void artist_album_cb(json_value* v)
{
  static int lastfm_api_state = NOTHING;
  static char buffer[512];
  static size_t len_album = 0;
  static int got_name = 0;
  if (v->kv == JSON_KEY && v->length && strncmp(v->buffer + v->offset, "name", v->length) == 0) {
    got_name = 1;
    switch(lastfm_api_state) {
      case NOTHING:
        lastfm_api_state = ALBUM;
        break;
      case ALBUM:
        lastfm_api_state = ARTIST;
        break;
      case ARTIST:
        lastfm_api_state = NOTHING;
    }
  }

  if (v->kv == JSON_VALUE && got_name) {
    got_name = 0;
    if (lastfm_api_state == ALBUM) {
      ASSERT(v->length + 5 < 512, "Not enough room in the buffer to put the album name");
      len_album = v->length;
      strncpy(buffer, v->buffer + v->offset, v->length);
      strncpy(buffer + len_album, ", by ", strlen(", by "));
      len_album += strlen(", by ");
    } else if (lastfm_api_state == ARTIST) {
      ASSERT(len_album + v->length < 512, "Not enough room in the buffer to put the artist name");
      strncpy(buffer + len_album, v->buffer + v->offset, v->length);
      buffer[len_album + v->length] = 0;
      LOG(LOG_DEBUG, "%s", buffer);
      lastfm_api_state = NOTHING;
    }
  }
}

int main(int argc, char* argv[])
{
  char buffer[BUFFER_SIZE];
  int fd, total = 0, len = 0;

  fd = open("test/lastfm.json", O_RDONLY);

  SYSCALL(fd);

  while((len = read(fd, buffer + total, BUFFER_SIZE))) {
    total += len;
  }

  parse_json(buffer, total, artist_album_cb);

  return 0;
}
