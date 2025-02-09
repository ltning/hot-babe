# where to install this program
PREFIX = /usr/local
DESTDIR = ${PREFIX}

# optimization cflags
CFLAGS = -O2 -Wall -g `gtk-config --cflags` `gdk-pixbuf-config --cflags`

OBJS = hot-babe.o 
CC = gcc
LIBS = `gtk-config --libs | sed "s/-lgtk//g"` `gdk-pixbuf-config --libs`
INSTALL = -m 755

all: hot-babe

hot-babe: $(OBJS)
	$(CC) -o hot-babe $(OBJS) $(LIBS)

clean:
	rm -f hot-babe *.o

install:
	install -d $(DESTDIR)/bin
	install $(INSTALL) hot-babe $(DESTDIR)/bin
