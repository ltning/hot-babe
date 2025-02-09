# where to install this program
PREFIX = /usr/local
DESTDIR = ${PREFIX}

# optimization cflags
CFLAGS = -O2 -Wall -g `gtk-config --cflags` `gdk-pixbuf-config --cflags` -DDESTDIR=\"$(DESTDIR)\"

OBJS = hot-babe.o loader.o
CC = gcc
LIBS = `gtk-config --libs` `gdk-pixbuf-config --libs`
INSTALL = -m 755

all: hot-babe

hot-babe: $(OBJS)
	$(CC) -o hot-babe $(OBJS) $(LIBS)

clean:
	rm -f hot-babe *.o

install:
	install -d $(DESTDIR)/bin
	install $(INSTALL) hot-babe $(DESTDIR)/bin
	install -d $(DESTDIR)/share/hot-babe/hb01
	install $(INSTALL) hb01/* $(DESTDIR)/share/hot-babe/hb01
