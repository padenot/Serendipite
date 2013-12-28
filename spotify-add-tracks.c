#include <stdlib.h>
#include <signal.h>
#include "macros.h"
#include "spotify.h"

char * g_playlist_url;

int usage(char * name)
{
  LOG(LOG_FATAL, "Usage:\n\t%s spotify_user_name spotify_password playlist_url", name);
  return EXIT_FAILURE;
}

void cleanup_spotify_session(int signal)
{
  if (spotify_shutdown()) {
    LOG(LOG_WARNING, "Could not exist spotify session properly.");
  }
}

void spotify_callback(sp_session* session)
{
  char linebuffer[256] = { 0 };
  sp_playlist * playlist;
  playlist = spotify_load_playlist_from_url(g_playlist_url);

  while(fgets(linebuffer, array_size(linebuffer), stdin)) {
    int i;
    if (linebuffer[0] == '\n') {
      break;
    }
    for (i = 0; i < array_size(linebuffer); i++) {
      if (linebuffer[i] == '\n') {
        linebuffer[i] = '\0';
      }
    }
    printf("###%s###\n", linebuffer);

    if (spotify_add_url_to_playlist(linebuffer, playlist) != OK) {
      LOG(LOG_CRITICAL, "Could not add track %s to playlist %s.", linebuffer, g_playlist_url);
    } else {
      LOG(LOG_OK, "%s added to %s.", linebuffer, g_playlist_url);
    }
  }

  sp_playlist_release(playlist);

  cleanup_spotify_session(0);
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

  g_playlist_url = argv[3];

  spotify_main_loop_init(argv[1], argv[2], spotify_callback);
  spotify_main_loop();

  return EXIT_SUCCESS;
}
