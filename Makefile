CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fpic

NAME = j1939decode
LIBA = lib$(NAME).a

SOURCES = src/j1939.c src/cJSON.c src/file.c
HEADERS = src/j1939.h src/cJSON.h src/file.h
LIBHEADERS = src/j1939.h
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

LIB_DIR = $(PREFIX)/lib
INCLUDE_DIR = $(PREFIX)/include/$(NAME)

.PHONY: all install uninstall clean
all: $(LIBA)

$(LIBA): $(OBJECTS)
	ar rcs $(LIBA) $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(LIBA)
	install -d $(DESTDIR)$(LIB_DIR)
	install -m 644 $(LIBA) $(DESTDIR)$(LIB_DIR)
	install -d $(DESTDIR)$(INCLUDE_DIR)
	install -m 644 $(LIBHEADERS) $(DESTDIR)$(INCLUDE_DIR)

uninstall:
	rm $(DESTDIR)$(LIB_DIR)/$(LIBA)
	rm -r $(DESTDIR)$(INCLUDE_DIR)

clean:
	rm $(LIBA)
	rm $(OBJECTS)
