#ifndef SPOTIFY_H
#define SPOTIFY_H
#include "external/spotify/api.h"

int spotify_add_playlist(sp_session* session, char * name, sp_playlist** playlist);
int spotify_add_track_to_playlist(const char * track_url, sp_playlist * playlist);
int spotify_do_search(char* search, void ** result);
int spotify_main_loop();
int spotify_main_loop_init(const char * username, const char * password, void (*callback)(sp_session*));
int spotify_playlist_url(sp_playlist * playlist, char * buffer, size_t len);
int spotify_select_playlist(sp_session * session, char* name, sp_playlist** playlist);
int spotify_shutdown();
sp_playlist * spotify_load_playlist_from_url(char * url);

#endif
