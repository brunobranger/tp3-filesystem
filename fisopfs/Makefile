CC = gcc
CFLAGS := -ggdb3 -O2 -Wall -std=c11
CFLAGS += -Wno-unused-function -Wvla
FUSE_CFLAGS = $(shell pkg-config fuse --cflags)
FUSE_LIBS = $(shell pkg-config fuse --libs)

FS_NAME := fisopfs
OBJS = fisopfs.o fs.o persistence.o

# === Docker ===
build:
	./dock build

run:
	./dock run

exec:
	./dock exec

# === Modo local (para debug fuera de Docker) ===
local: $(FS_NAME)

$(FS_NAME): $(OBJS)
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) $^ $(FUSE_LIBS) -o $@

fisopfs.o: fisopfs.c fs.h persistence.h
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) -c fisopfs.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

persistence.o: persistence.c persistence.h fs.h
	$(CC) $(CFLAGS) -c persistence.c

clean:
	rm -f $(FS_NAME) $(OBJS) .clang-files
	rm -rf prueba/

format: .clang-files .clang-format
	xargs -r clang-format -i < .clang-files

.clang-files:
	find . -type f \( -name '*.c' -o -name '*.h' \) > .clang-files

.PHONY: build run exec local clean format