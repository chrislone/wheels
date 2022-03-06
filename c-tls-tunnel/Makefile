CC = gcc

all: main

main: main.c
	$(CC) -o main deps/cJSON.c httpheaderlist.c utils.c main.c -iquote ./llhttp/build ./llhttp/build/libllhttp.a -L/usr/local/lib -ltls -lssl
