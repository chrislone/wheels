CC = gcc

all: main

main: main.c
	$(CC) -o main httpheaderlist.c main.c -iquote ./llhttp/build ./llhttp/build/libllhttp.a -L/usr/local/lib -ltls -lssl
