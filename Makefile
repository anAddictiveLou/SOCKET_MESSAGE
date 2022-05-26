all:
	gcc -o server stream_server.c
	gcc -o client stream_client.c
clean:
	rm -rf client server