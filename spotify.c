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

static void logged_in(sp_session *session, sp_error error)
{
  LOG(LOG_OK, "Logged in");
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

static void connection_error(sp_session *session, sp_error error)
{
  LOG(LOG_WARNING, "Connection error: %s.", sp_error_message(error));
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
  ASSERT(1, "Not implemented.");
  return 0;
}

void metadata_updated(sp_session *session)
{
  ASSERT(1, "Not implemented.");
}

void play_token_lost(sp_session *session)
{
  ASSERT(1, "Not implemented.");
}

void end_of_track(sp_session *session)
{
  ASSERT(1, "Not implemented.");
}

static sp_session_callbacks session_callbacks = {
  .logged_in = &logged_in,
  .logged_out = &logged_out,
  .connection_error = &connection_error,
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

int spotify_init(const char * username, const char * password)
{
  sp_error error;
  sp_session *session;
  spconfig.application_key_size = g_appkey_size;

  // Register the callbacks.
  spconfig.callbacks = &session_callbacks;

  error = sp_session_create(&spconfig, &session);
  if (SP_ERROR_OK != error) {
    LOG(LOG_CRITICAL, "failed to create session: %s", sp_error_message(error));
    return 2;
  }

  error = sp_session_login(session, username, password, 0, NULL);

  if (SP_ERROR_OK != error) {
    LOG(LOG_CRITICAL, "failed to login: %s", sp_error_message(error));
    return 3;
  }

  g_session = session;
  return 0;
}

int spotify_shutdown()
{
  LOG(LOG_OK, "Shutting down spotify session.");
  if (!g_session) {
    LOG(LOG_WARNING, "Not logged in, cannot shutdown session.");
    return 0;
  }
  if (sp_session_logout(g_session) != SP_ERROR_OK) {
    LOG(LOG_WARNING, "Could not logout.");
  }
  if (sp_session_release(g_session) != SP_ERROR_OK) {
    LOG(LOG_WARNING, "Could not release spotify session.");
  }
  return 0;
}

void tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata) {
  LOG(LOG_OK, "%d tracks added.", num_tracks);
}

void playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata)
{
  LOG(LOG_OK, "playlist update in progress (finished: %s).", done ? "true" : "false");
}

void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
  LOG(LOG_OK, "Playlist container loaded.");
}

int search_complete() {
  ASSERT(1, "Not implemented.");
  return OK;
}

int spotify_do_search(char* search, void ** result)
{
  ASSERT(1, "Not implemented.");
  ASSERT(g_session, "Should have a session if we do a search.");
 //  sp_search* sp_search_create   (g_session,
 //    search,
 //    0   track_offset,
 //    0   track_count,
 //    0   album_offset,
 //    10   album_count,
 //    0   artist_offset,
 //    10   artist_count,
 //    0   playlist_offset,
 //    0   playlist_count,
 //    SP_SEARCH_STANDARD,
 //    search_complete_cb *    callback,
 //    void *    userdata   
 //  );
  return OK;
}

int spotify_add_playlist(sp_session* session, char * name, sp_playlist** playlist)
{
  LOG(LOG_DEBUG, "Getting container…");
  sp_playlistcontainer_callbacks container_callbacks = {
    .container_loaded = container_loaded,
  };
  sp_playlistcontainer * container = sp_session_playlistcontainer(session);
  sp_playlistcontainer_add_callbacks(container, &container_callbacks, NULL);
  *playlist = sp_playlistcontainer_add_new_playlist(container, name);
  if (*playlist == NULL) {
    return ERROR;
  }

  sp_playlist_callbacks playlist_callbacks = {
    .tracks_added = tracks_added,
    .playlist_update_in_progress = playlist_update_in_progress,
  };
  sp_playlist_add_callbacks(*playlist, &playlist_callbacks, NULL);
  return OK;
}

int spotify_playlist_url(sp_playlist * playlist, char * buffer, size_t len)
{
  sp_link *playlist_link = sp_link_create_from_playlist(playlist);
  if (playlist_link == NULL) {
    LOG(LOG_WARNING, "Failed to create link.");
    return ERROR;
  }
  int size = sp_link_as_string(playlist_link, buffer, len);
  if (size > len) {
    LOG(LOG_WARNING, "Link was truncated.");
    return ERROR;
  }

  return OK;
}

int spotify_main_loop_init(char * username, char * password, void (*callback)(sp_session*))
{
  g_callback = callback;
  pthread_mutex_init(&notify_mutex, NULL);
  pthread_cond_init(&notify_cond, NULL);

  if (spotify_init(username, password) != 0) {
    LOG(LOG_CRITICAL, "Spotify failed to initialize");
    exit(ERROR);
  }
  return 0;
}

int spotify_main_loop()
{
  int next_timeout = 0;

  if (!g_session) {
    LOG(LOG_CRITICAL, "Need a session before rolling.");
    spotify_shutdown();
    exit(1);
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
