#ifndef SPOTIFY_H
#define SPOTIFY_H
#include "external/spotify/api.h"

int spotify_main_loop_init(const char * username, const char * password, void (*callback)(sp_session*));
int spotify_main_loop();
int spotify_shutdown();
int spotify_add_playlist(sp_session* session, char * name, sp_playlist** playlist);
int spotify_playlist_url(sp_playlist * playlist, char * buffer, size_t len);
int spotify_select_playlist(sp_session * session, char* name, sp_playlist** playlist);
int spotify_do_search(char* search, void ** result);

#endif
