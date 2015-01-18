SERVER1_PORT = 12425
SERVER2_PORT = 12525

all:
	clear
	make clear
	gcc client.c -o client
	make run_client

client:
	gcc client.c -o client

server:
	gcc server.c -o server -lpthread

both:
	make client
	make server

clear:
	rm client
	rm server

run_s1:
	./server $(SERVER1_PORT) $(SERVER2_PORT) localhost

run_s2:
	./server $(SERVER2_PORT) $(SERVER1_PORT) localhost

run_c1:
	./client $(SERVER1_PORT) localhost $(SERVER2_PORT) localhost

run_c2:
	./client $(SERVER2_PORT) localhost $(SERVER1_PORT) localhost

build:
	clear
	rm client
	make client
	make run_client

test:
	clear
	make clear
	gcc client.c -o client
	gcc server.c -o server
	make run

