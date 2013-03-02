#ifndef __MAIN_H__
#define __MAIN_H__
#include <libspotify/api.h>
int spotify_main_loop(char * username, char * password, void (*callback)(sp_session*));
#endif
