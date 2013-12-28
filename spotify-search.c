#include <stdlib.h>
#include <signal.h>
#include "macros.h"
#include "spotify.h"

char * g_query;

int usage(char * name)
{
  LOG(LOG_FATAL, "Usage:\n\t%s spotify_user_name spotify_password search", name);
  return EXIT_FAILURE;
}

void cleanup_spotify_session(int signal)
{
  if (spotify_shutdown()) {
    LOG(LOG_WARNING, "Could not exist spotify session properly.");
  }
}

void spotify_search_complete(sp_album* album)
{
  char link_text[256];
  sp_link* link;

  link = sp_link_create_from_album(album);
  if (!link) {
    LOG(LOG_WARNING, "Could not create link from album.");
  }

  if (sp_link_as_string(link, link_text, array_size(link_text)) > array_size(link_text)) {
    LOG(LOG_WARNING, "Link truncated.");
  }

  printf("%s\n", link_text);
  LOG(LOG_OK, "Exiting...");

  cleanup_spotify_session(0);
}

void spotify_callback(sp_session* session)
{
  spotify_search_album(g_query, spotify_search_complete);
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

  g_query = argv[3];

  spotify_main_loop_init(argv[1], argv[2], spotify_callback);
  spotify_main_loop();

  return EXIT_SUCCESS;
}
