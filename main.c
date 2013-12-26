#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <signal.h>
#include "macros.h"
#include "json.h"
#include "http.h"
#include "external/utf8.h"
#include "spotify.h"

#define LFM_HOST "ws.audioscrobbler.com"
#define NEW_RELEASE_URL                                                        \
"/2.0/?method=user.getnewreleases"                                             \
"&user=evok3r&api_key="                                                        \
API_KEY                                                                        \
"&format=json"

#define API_KEY "d4ff4069de5ae7929d4037dc01313301"

#define safe_free(x) \
  if (x) {           \
    free(x);         \
  }

typedef enum _spotify_tasks {
  SEARCH
} spotify_task;

typedef enum _spotify_job_status {
  ALLOCATED,
  INITIALIZED,
  RUNNING,
  DONE
} spotify_job_status;

typedef struct _spotify_job {
  spotify_task task;
  spotify_job_status status;
  char* input;
  void* result;
} spotify_job;

spotify_job* spotify_job_create(spotify_task task)
{
  spotify_job* job = calloc(1, sizeof(spotify_job));
  job->task = task;
  job->status = INITIALIZED;
  return job;
}

void spotify_job_set_input(spotify_job* job, char* str)
{
  if (job->task != SEARCH) {
    LOG(LOG_WARNING, "Attempt to set input on a job that does not take an input.");
    return;
  }
  job->input = str;
}

void spotify_job_get_result(spotify_job* job, void ** result)
{
  ASSERT(job && *result);
  if (job->status != DONE) {
    LOG(LOG_WARNING, "Job not done, try again.");
    return;
  }
  *result = job->result;
  job->result = NULL;
}

void spotify_job_destroy(spotify_job* job) {
  if (job->status != DONE) {
    LOG(LOG_WARNING, "destroying in flight job.");
  }
  safe_free(job->input);
  free(job);
}

void spotify_job_run(spotify_job* job) {
  if (job->status != INITIALIZED) {
    LOG(LOG_WARNING, "spotify_job_run: bad state.");
    return;
  }
  switch (job->task) {
    case SEARCH:
      ASSERT(job->input);
      spotify_do_search(job->input, &job->result);
      break;
    default:
      LOG(LOG_FATAL, "invalid job");
      exit(1);
  }
}

void json_callback(json_value* v, void* user_ptr) {
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

struct artist_album_state {
  int lastfm_api_state;
  char buffer[512];
  size_t len_album;
  char* result[30];
  int result_index;
  int got_name;
};

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
      strncpy(s->buffer + s->len_album, ", by ", strlen(", by "));
      s->len_album += strlen(", by ");
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

// move this.
void spotify_callback(sp_session* session)
{
  printf("allo callback %d\n", __LINE__);
}

void cleanup_spotify_session(int signal)
{
  if (spotify_shutdown()) {
    LOG(LOG_WARNING, "Could not exist spotify session properly.");
  }
  exit(0);
}

void install_sighandler()
{
  struct sigaction sigactn;

  sigactn.sa_handler = cleanup_spotify_session;

  if (sigaction(SIGINT, &sigactn, NULL)) {
    LOG(LOG_FATAL, "Could not install signal handler.");
    exit(1);
  }
}

int main(int argc, char* argv[])
{
  char* buffer;
  size_t length, body_pos;
  struct artist_album_state state = {0};
  int i;

  setlocale(LC_ALL, "");

  install_sighandler();

  httpreq(&buffer, &length);

  body_pos = http_body_offset(buffer, length);

  LOG(LOG_OK, "New relases:");
  parse_json(buffer + body_pos, length - body_pos, (void*)&state, artist_album_cb);

  for (i = 0; i < state.result_index; i++) {
    printf("%d. %s\n", i, state.result[i]);
  }

  spotify_main_loop(argv[1], argv[2], spotify_callback);

  free(buffer);

  for (i = 0; i < state.result_index; i++) {
   free(state.result[i]);
  }

  return 0;
}
