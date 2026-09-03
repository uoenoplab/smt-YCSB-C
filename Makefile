CC=g++
CFLAGS=-std=c++11 -g -Wall -pthread -I./ -I./redis/hiredis -I./redis/homa
LDFLAGS=-lpthread -ltbb
HIREDIS=redis/hiredis/libhiredis.a
SUBDIRS=core db
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: hiredis $(SUBDIRS) $(EXEC)

# Vendored hiredis with the Homa transport patch; built static, no system install.
hiredis:
	$(MAKE) -C redis/hiredis static HIREDIS_CFLAGS="-I../homa -D_DEFAULT_SOURCE"

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS) $(HIREDIS)
	$(CC) $(CFLAGS) $(wildcard *.cc) $(OBJECTS) $(HIREDIS) $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	$(MAKE) -C redis/hiredis clean
	$(RM) $(EXEC)

.PHONY: hiredis $(SUBDIRS) $(EXEC)
