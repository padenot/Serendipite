#include <stdlib.h>
#include <signal.h>
#include "macros.h"
#include "spotify.h"

char * g_playlist_name;

int usage(char * name) {
  LOG(LOG_FATAL, "Usage:\n\t%s spotify_user_name spotify_password playlist_name", name);
  return EXIT_FAILURE;
}

void cleanup_spotify_session(int signal)
{
  if (spotify_shutdown()) {
    LOG(LOG_WARNING, "Could not exist spotify session properly.");
  }
  exit(0);
}

void spotify_callback(sp_session* session)
{
  char url[255];
  int rv;
  sp_playlist * playlist;
  rv = spotify_add_playlist(session, g_playlist_name, &playlist);
  if (rv) {
    LOG(LOG_CRITICAL, "Error while adding the playlist");
    return;
  }

  rv = spotify_playlist_url(playlist, url, array_size(url));

  if (rv) {
    LOG(LOG_CRITICAL, "Cannot get url for playlist.");
    return;
  }

  printf("%s\n", url);
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

int main(int argc, char ** argv) {
  if (argc != 4) {
    return usage(argv[0]);
  }

  install_sighandler();

  g_playlist_name = argv[3];

  spotify_main_loop_init(argv[1], argv[2], spotify_callback);
  spotify_main_loop();

  return EXIT_SUCCESS;
}
