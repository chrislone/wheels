CC = gcc

all: main

main: main.c
	$(CC) -o main deps/base64.c deps/cJSON.c httpheaderlist.c utils.c main.c \
 	-iquote ./llhttp/build ./llhttp/build/libllhttp.a -lcrypto -lm -L/usr/local/lib -ltls -lssl
