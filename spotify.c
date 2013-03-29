#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include "spotify.h"
#include "macros.h"
#include "config.h"

extern const char g_appkey[];
extern const size_t g_appkey_size;

sp_session *g_session;

static int notify_events;
static pthread_mutex_t notify_mutex;
static pthread_cond_t notify_cond;
void (*g_callback)(sp_session*) = 0;


static void connection_error(sp_session * session, sp_error error)
{
  LOG(LOG_DEBUG, "Connection error: %s", sp_error_message(error));
}

static void logged_in(sp_session *session, sp_error error)
{
  if (error != SP_ERROR_OK) {
    LOG(LOG_CRITICAL, "Login error: %s", sp_error_message(error));
    exit(ERROR);
  }
  g_callback(session);
}

static void logged_out(sp_session *session)
{
  LOG(LOG_OK, "Logged out");
  exit(0);
}

void notify_main_thread(sp_session *session)
{
  pthread_mutex_lock(&notify_mutex);
  notify_events = 1;
  pthread_cond_signal(&notify_cond);
  pthread_mutex_unlock(&notify_mutex);
}

void log_message(sp_session * session, const char* msg)
{
  LOG(LOG_DEBUG, "[Spotify] %s", msg);
}

int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {

}

void metadata_updated(sp_session *session)
{ }

void play_token_lost(sp_session *session)
{ }

void end_of_track(sp_session *session)
{ }

static sp_session_callbacks session_callbacks = {
  .logged_in = &logged_in,
  .notify_main_thread = &notify_main_thread,
  .music_delivery = &music_delivery,
  .metadata_updated = &metadata_updated,
  .play_token_lost = &play_token_lost,
  .log_message = &log_message,
  .end_of_track = &end_of_track,
};

static sp_session_config spconfig = {
  .api_version = SPOTIFY_API_VERSION,
  .cache_location = "tmp",
  .settings_location = "tmp",
  .application_key = g_appkey,
  .application_key_size = 0,
  .user_agent = PACKAGE_STRING,
  .callbacks = &session_callbacks,
  NULL,
};

int spotify_init(const char * username,const char * password)
{
  sp_error error;
  sp_session *session;
  spconfig.application_key_size = g_appkey_size;

  // Register the callbacks.
  spconfig.callbacks = &session_callbacks;

  error = sp_session_create(&spconfig, &session);
  if (SP_ERROR_OK != error) {
    fprintf(stderr, "failed to create session: %s\n",
                    sp_error_message(error));
    return 2;
  }

  // Login using the credentials given on the command line.
  error = sp_session_login(session, username, password, 0, NULL);

  if (SP_ERROR_OK != error) {
    fprintf(stderr, "failed to login: %s\n",
                    sp_error_message(error));
    return 3;
  }

  g_session = session;
  return 0;
}

int spotify_main_loop(char * username, char * password, void (*callback)(sp_session*))
{
  int next_timeout = 0;
  g_callback = callback;
  pthread_mutex_init(&notify_mutex, NULL);
  pthread_cond_init(&notify_cond, NULL);

  if (spotify_init(username, password) != 0) {
    fprintf(stderr,"Spotify failed to initialize\n");
    exit(ERROR);
  }
  pthread_mutex_lock(&notify_mutex);
  for (;;) {
    if (next_timeout == 0) {
      while(!notify_events) {
        pthread_cond_wait(&notify_cond, &notify_mutex);
      }
    } else {
      struct timespec ts;

      clock_gettime(CLOCK_REALTIME, &ts);

      ts.tv_sec += next_timeout / 1000;
      ts.tv_nsec += (next_timeout % 1000) * 1000000;

      while(!notify_events) {
        if(pthread_cond_timedwait(&notify_cond, &notify_mutex, &ts)) {
          break;
        }
      }
    }

    notify_events = 0;
    pthread_mutex_unlock(&notify_mutex);

    do {
      sp_session_process_events(g_session, &next_timeout);
    } while (next_timeout == 0);

    pthread_mutex_lock(&notify_mutex);
  }
  return 0;
}
