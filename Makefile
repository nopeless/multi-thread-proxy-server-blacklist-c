# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC=gcc
CFLAGS=-fdiagnostics-color -O2 -Wall -std=gnu2x -fms-extensions -D_GNU_SOURCE -Wno-unused-result
LDFLAGS=-lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy: proxy.c csapp.h
	$(CC) $(CFLAGS) -Ilib csapp.o lib/bigboi.c lib/safe_queue.c lib/url_blacklist.c proxy.c
	cp a.out proxy

proxy-debug: CFLAGS += -DDEBUG -g -O0
proxy-debug: proxy;

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

