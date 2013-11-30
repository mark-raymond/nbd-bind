# Makefile for nbd-bind
# Copyright (c) 2013 Mark Raymond
# Released under the MIT license

QEMUNBD := /usr/bin/qemu-nbd
PREFIX  := /usr/local/bin
CFLAGS  := -Wall -DQEMUNBD="$(QEMUNBD)"

.PHONY: all clean install

all: nbd-bind nbd-unbind

clean:
	rm -f nbd-bind nbd-unbind nbd-bind.o nbd-unbind.o nbd-common.o

install: all
	install -m 4755 nbd-bind $(PREFIX)/nbd-bind
	install -m 4755 nbd-unbind $(PREFIX)/nbd-unbind

uninstall:
	rm -f $(PREFIX)/nbd-bind $(PREFIX)/nbd-unbind

nbd-bind: nbd-bind.o nbd-common.o
nbd-unbind: nbd-unbind.o nbd-common.o
