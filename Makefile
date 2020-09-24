all: build

build:
	gcc -Wall -pthread s-talk.c receiver.c list.c keyboard_send.c shutdown.c -o s-talk

clean: 
	rm s-talk

run: build
	./s-talk 8989 localhost 8989

valgrind:
	gcc -Wall -g -pthread s-talk.c receiver.c list.c keyboard_send.c shutdown.c -o s-talk
	valgrind --leak-check=full --show-leak-kinds=all ./s-talk 8989 localhost 8989