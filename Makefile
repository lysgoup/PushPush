all:
	gcc -o client/p_client client/p_client.c `pkg-config --libs --cflags gtk+-3.0` -lpthread
	gcc -o server/serv_gr server/server_gr.c -pthread -I ./server/jansson-2.13/src/ -L ./server/jansson-2.13/src/.libs -ljansson

clean:
	rm server/serv_gr
	rm client/p_client