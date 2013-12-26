# Sérendipité

## What?
A last.fm / Spotify mashup software written in C, that intends to create dynamic
Spotify playlists based on you last.fm recommendations.

## How?

Get an `appkey.c` file, by registering as a developer at
<https://developer.spotify.com/>, and save the content of your key in an
`appkey.c` file, at the root of the repo. Then, do the following:

```sh
./autogen.sh
./configure --enable-debug --enable-colors
make -f Makefile_spotify
make
```

## Why C?
Why not?

## What license?
MIT
