#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include "spotify.h"
#include "macros.h"
#include "config.h"

const char * CREDENTIAL_BLOB_PATH = PACKAGE_STRING"/credentials";

extern const char g_appkey[];
extern const size_t g_appkey_size;

sp_session *g_session = 0;

static int notify_events;
static int g_can_exit = 0;
static int g_operations_pending = 0;
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
  LOG(LOG_OK, "Logged out.");
  pthread_mutex_lock(&notify_mutex);
  g_can_exit = 1;
  pthread_cond_signal(&notify_cond);
  pthread_mutex_unlock(&notify_mutex);
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

void credentials_blob_updated(sp_session * session, const char * blob)
{
  FILE * f = fopen(CREDENTIAL_BLOB_PATH, "w");

  if (!f) {
    LOG(LOG_WARNING, "Could not open user credentials file (%s).", strerror(errno));
    return;
  }

  if (fputs(blob, f) < 0) {
    LOG(LOG_WARNING, "Could not write user credentials file (%s).", strerror(errno));
    return;
  }

  if (fclose(f)) {
    LOG(LOG_WARNING, "Could not close user credentials file (%s).", strerror(errno));
    return;
  }

  LOG(LOG_OK, "Credentials stored: %s", blob);
}

void retrieve_credential_blob(char * blob, size_t len)
{
  FILE * f = fopen(CREDENTIAL_BLOB_PATH, "r");
  if (!f) {
    LOG(LOG_WARNING, "Could not open user credentials file (%s).", strerror(errno));
    return;
  }

  if (fgets(blob, len, f) == NULL) {
    LOG(LOG_WARNING, "Could not read user credentials file (%s).", strerror(errno));
    return;
  }

  if (fclose(f)) {
    LOG(LOG_WARNING, "Could not close user credentials file (%s).", strerror(errno));
    return;
  }
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
  .credentials_blob_updated = credentials_blob_updated
};

static sp_session_config spconfig = {
  .api_version = SPOTIFY_API_VERSION,
  .cache_location = PACKAGE_STRING,
  .settings_location = PACKAGE_STRING,
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
  char blob[256] = { 0 };

  // Register the callbacks.
  spconfig.callbacks = &session_callbacks;

  error = sp_session_create(&spconfig, &session);
  if (SP_ERROR_OK != error) {
    LOG(LOG_CRITICAL, "failed to create session: %s", sp_error_message(error));
    return 2;
  }

  if (strlen(password) == 0) {
    retrieve_credential_blob(blob, array_size(blob));
    error = sp_session_login(session, username, NULL, 0, blob);
  } else {
    error = sp_session_login(session, username, password, 0, NULL);
  }


  if (SP_ERROR_OK != error) {
    LOG(LOG_CRITICAL, "failed to login: %s", sp_error_message(error));
    return 3;
  }

  g_session = session;
  return 0;
}

int spotify_shutdown()
{
  pthread_mutex_lock(&notify_mutex);
  LOG(LOG_OK, "Logging out.");
  if (sp_session_logout(g_session) != SP_ERROR_OK) {
    LOG(LOG_WARNING, "Could not logout.");
  }
  while (g_operations_pending) {
    pthread_cond_signal(&notify_cond);
    pthread_mutex_unlock(&notify_mutex);
    return 0;
  }
  pthread_cond_signal(&notify_cond);
  pthread_mutex_unlock(&notify_mutex);

  LOG(LOG_OK, "Shutting down spotify session.");
  if (!g_session) {
    LOG(LOG_WARNING, "Not logged in, cannot shutdown session.");
    return 0;
  }

  return 0;
}

void tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata) {
  LOG(LOG_OK, "%d tracks added.", num_tracks);
}

void playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata)
{
  LOG(LOG_OK, "playlist update in progress (finished: %s).", done ? "true" : "false");
  pthread_mutex_lock(&notify_mutex);
  g_operations_pending = done ? 0 : 1;
  pthread_mutex_unlock(&notify_mutex);
  if (done) {
    notify_main_thread(g_session);
  }
}

void container_loaded(sp_playlistcontainer *pc, void *userdata)
{
  LOG(LOG_OK, "Playlist container loaded.");
}

void search_complete(sp_search * result, void * userdata) {
  ASSERT(sp_search_is_loaded(result), "Search should be loaded.");

  void (*callback)(sp_album*) = userdata;

  sp_error rv = sp_search_error(result);
  if(rv != SP_ERROR_OK) {
    LOG(LOG_WARNING, "Search error: %s", sp_error_message(rv));
    goto cleanup;
  }

  if (sp_search_num_albums(result) == 0) {
    LOG(LOG_WARNING, "No result for query %s", sp_search_query(result));
    goto cleanup;
  }

  sp_album* album = sp_search_album(result, 0);
  callback(album);

cleanup:
  sp_search_release(result);
}

int spotify_search_album(char* search, void (*callback)(sp_album*))
{
  ASSERT(g_session, "Should have a session if we do a search.");
  sp_search * s;

  s = sp_search_create(g_session,
                       search,
                       0, 0, // tracks
                       0, 1, // artist
                       0, 0, // album
                       0, 0, // playlists
                       SP_SEARCH_STANDARD,
                       search_complete,
                       (void*)callback);
  sp_search_query(s);
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

  sp_link_release(playlist_link);

  return OK;
}

void albumbrowse_callback(sp_albumbrowse *result, void *userdata)
{
  int album_track_count,
      playlist_track_count,
      rv,
      i;
  sp_playlist * playlist = userdata;
  sp_track * track;

  playlist_track_count = sp_playlist_num_tracks(playlist);
  album_track_count = sp_albumbrowse_num_tracks(result);

  LOG(LOG_OK, "Adding %d tracks to a playlist of %d tracks.", album_track_count, playlist_track_count);

  for (i = 0; i < album_track_count; i++) {
    track = sp_albumbrowse_track(result, i);
    rv = sp_playlist_add_tracks(playlist, &track, 1, playlist_track_count + i, g_session);
    if (rv != SP_ERROR_OK) {
      LOG(LOG_CRITICAL, "Could not add track: %s", sp_error_message(rv));
    } else {
      LOG(LOG_OK, "One track added.");
    }
    sp_track_release(track);
  }
  sp_albumbrowse_release(result);

  pthread_mutex_lock(&notify_mutex);
  g_operations_pending = 0;
  pthread_cond_signal(&notify_cond);
  pthread_mutex_unlock(&notify_mutex);
}

int spotify_add_url_to_playlist(const char * link_text, sp_playlist * playlist)
{
  sp_link * link;
  sp_track * track;
  sp_error rv;
  int track_count;

  link = sp_link_create_from_string(link_text);

  if (link == NULL) {
    LOG(LOG_WARNING, "Failed to create link for string %s.", link_text);
    return ERROR;
  }

  switch (sp_link_type(link)) {
    case SP_LINKTYPE_ALBUM: {
        sp_album * album = sp_link_as_album(link);

        if (!album) {
          sp_link_release(link);
          LOG(LOG_WARNING, "Could not get the album from the link.");
          return ERROR;
        }

        pthread_mutex_lock(&notify_mutex);
        g_operations_pending = 1;
        pthread_mutex_unlock(&notify_mutex);

        sp_albumbrowse_create(g_session, album, albumbrowse_callback, (void*)playlist);
        sp_link_release(link);
      }
      break;
    case SP_LINKTYPE_TRACK: {
        sp_track * track = sp_link_as_track(link);
        if (!track) {
          sp_link_release(link);
          LOG(LOG_WARNING, "Could not get the track from the link.");
          return ERROR;
        }

        /* append */
        track_count = sp_playlist_num_tracks(playlist);
        rv = sp_playlist_add_tracks(playlist, &track, 1, track_count, g_session);
        if (rv != SP_ERROR_OK) {
          LOG(LOG_CRITICAL, "Could not add track: %s", sp_error_message(rv));
        }
        sp_link_release(link);
        sp_track_release(track);
      }
      break;
    case SP_LINKTYPE_ARTIST:
      break;
    case SP_LINKTYPE_SEARCH:
      break;
    default:
      LOG(LOG_WARNING, "Don't know what to do with %s.", link_text);
  }

  return OK;
}

sp_playlist * spotify_load_playlist_from_url(char * url)
{
  sp_playlist* playlist;
  sp_link * link;

  ASSERT(g_session, "Should have a session.");
  link = sp_link_create_from_string(url);

  if (sp_link_type(link) != SP_LINKTYPE_PLAYLIST) {
    LOG(LOG_WARNING, "%s is not a valid playlist link.", url);
    return NULL;
  }

  playlist = sp_playlist_create(g_session, link);

  sp_playlist_callbacks playlist_callbacks = {
    .tracks_added = tracks_added,
    .playlist_update_in_progress = playlist_update_in_progress,
  };
  sp_playlist_add_callbacks(playlist, &playlist_callbacks, NULL);

  sp_link_release(link);

  return playlist;
}

int spotify_main_loop_init(const char * username, const char * password, void (*callback)(sp_session*))
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

    if (g_can_exit) {
      break;
    }
  }

  return 0;
}
