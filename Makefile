all: client server
client:bankingClient.c
	gcc -o client bankingClient.h bankingClient.c -lpthread
server:bankingServer.c
	gcc -o server bankingServer.h bankingServer.c -lpthread

clean:
	rm -f client
	rm -f server
