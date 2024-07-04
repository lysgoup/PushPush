# PushPush: Multi-user Game Similar to Sokoban

## How To Build
```
$ gcc -o client p_client.c `pkg-config --libs --cflags gtk+-2.0`
```
Use this command in the client directory to build a client source code with gtk library.
```
$ gcc -o server p_server.c -pthread -I ./jansson-2.13/src/ -L ./jansson-2.13/src/.libs -ljansson
```
Use this command in the server directory to build a server source code with jasson library.
<br>
You can also use make command in the main directory to build both of source codes.
```
$ make
```