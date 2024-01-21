all:
	gcc -o client/p_client client/p_client.c `pkg-config --libs --cflags gtk+-3.0` -lpthread
	gcc -o serv_gr server_gr.c -pthread -I jansson-2.13/src/ -L jansson-2.13/src/.libs -ljansson

clean:
	rm server/serv_gr
	rm client/p_client