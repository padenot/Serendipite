AUTOMAKE_OPTIONS = foreign

CFLAGS = -Wall
include_HEADERS = json.h http.h

AM_CFLAGS = -I$(top_srcdir)/external/spotify/ -L$(top_srcdir)/external/spotify/

bin_PROGRAMS = lastfmreleases spotifycreateplaylist
lastfmreleases_SOURCES = json.c http.c lastfm-releases.c external/utf8.c
spotifycreateplaylist_SOURCES = spotify-create-playlist.c spotify.c appkey.c
spotifycreateplaylist_LDADD = -lspotify

if DEBUG
AM_CFLAGS += -g3 -O0 -DDEBUG
AM_CXXFLAGS = -g3 -O0 -DDEBUG
else
AM_CFLAGS += -O2
AM_CXXFLAGS = -O2
endif

if COLORS
AM_CFLAGS += -DCOLORS
endif
