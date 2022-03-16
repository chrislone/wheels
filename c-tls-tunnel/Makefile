CC = gcc
MAC = "Darwin"
Linux = "Linux"
PLATFORM :="$(shell uname -s)"
FLAGS ?=

## ifeq 不需要缩进
ifeq ($(PLATFORM),$(MAC))
	FLAGS += -L/usr/local/lib
else
	FLAGS += -Wl,-rpath=/usr/local/lib
endif

all: main

main: main.c
	$(CC) -o main \
	deps/base64.c \
	deps/cJSON.c \
	httpheaderlist.c \
	utils.c \
	config.c \
	main.c \
 	-iquote ./llhttp/build ./llhttp/build/libllhttp.a -lm $(FLAGS) -lcrypto -ltls -lssl

platform:
	@echo "FLAGS: $(PLATFORM)"
