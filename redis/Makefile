CC=g++
CFLAGS+=-Wall -std=c++11
INCLUDES=-I../ -I./homa
HEADERS=$(wildcard *.h)
LDFLAGS=-lhiredis -lhiredis_ssl

all: hiredis sample

sample:	sample.cc ../db/redis_db.o $(HEADERS)
	$(MAKE) -C ./hiredis USE_SSL=1
	sudo $(MAKE) install -C ./hiredis USE_SSL=1
	$(MAKE) -C ../db
	$(CC) $(CFLAGS) $(INCLUDES) $< ../db/redis_db.o $(LDFLAGS) -o $@

clean:
	$(RM) sample
	$(MAKE) -C ./hiredis clean
