FLAGS=-fdiagnostics-color -O2 -pedantic -Wall -std=gnu2x -fms-extensions

all: _lib bigboi safe_queue

bigboi: _lib bigboi.h bigboi.c
	gcc $(FLAGS) bigboi.h bigboi.c -c

bigboi-test: FLAGS += -DDEBUG -g -O0
bigboi-test: bigboi bigboi_test.c
	gcc $(FLAGS) snstrcpy.o bigboi.o bigboi_test.c

bigboi-debug: bigboi-test;

safe_queue: safe_queue.h safe_queue.c
	gcc $(FLAGS) safe_queue.h safe_queue.c -c

safe_queue-test: FLAGS += -DDEBUG -g -O0
safe_queue-test: safe_queue safe_queue_test.c
	gcc $(FLAGS) safe_queue.o safe_queue_test.c

safe_queue-debug: safe_queue-test;

url_blacklist: url_blacklist.h url_blacklist.c
	gcc $(FLAGS) url_blacklist.h url_blacklist.c -c

url_blacklist-test: FLAGS += -DDEBUG -g -O0
url_blacklist-test: url_blacklist url_blacklist_test.c
	gcc $(FLAGS) url_blacklist.o url_blacklist_test.c

url_blacklist-debug: url_blacklist-test;

clean:
	rm -f *.o *.gch *.log a.out
