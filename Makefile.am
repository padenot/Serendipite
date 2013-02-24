AUTOMAKE_OPTIONS = foreign

CFLAGS = -Wall
include_HEADERS = json.h http.h

bin_PROGRAMS = serendipite
serendipite_SOURCES = json.c http.c main.c

if DEBUG
AM_CFLAGS = -g3 -O0 -DDEBUG
AM_CXXFLAGS = -g3 -O0 -DDEBUG
else
AM_CFLAGS = -O2
AM_CXXFLAGS = -O2
endif
