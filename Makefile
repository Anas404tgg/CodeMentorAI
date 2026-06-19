# Makefile for CodeMentor AI Backend (Phase 1)
export TMP := /c/Users/HP/Desktop/tidada/tmp
export TEMP := /c/Users/HP/Desktop/tidada/tmp
export TMPDIR := /c/Users/HP/Desktop/tidada/tmp
CC = /c/msys64/mingw64/bin/gcc
CFLAGS = -std=c99 -Wall -Wextra -I src/backend
LDFLAGS = -lmicrohttpd -lcurl -lsqlite3 -lcjson -lws2_32

SRCS = src/backend/main.c \
       src/backend/server.c \
       src/backend/parser.c \
       src/backend/sandbox.c \
       src/backend/database.c \
       src/backend/ai_client.c \
       src/backend/json_utils.c \
       src/backend/migrations.c

OBJS = $(SRCS:.c=.o)

TARGET = codementor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean