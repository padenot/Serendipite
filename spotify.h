#ifndef SPOTIFY_H
#define SPOTIFY_H
#include "external/spotify/api.h"

int spotify_main_loop(char * username, char * password, void (*callback)(sp_session*));

#endif
