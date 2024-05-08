#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include "safe_queue.h"

#define QUEUE_CAPACITY 10
#define NUM_ITEMS 100
#define NUM_PRODUCERS 5
#define NUM_CONSUMERS 3
#define PRODUCER_SLEEP 1000000
#define CONSUMER_SLEEP 1000000

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void mutex_printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  pthread_mutex_lock(&print_mutex);
  vprintf(format, args);
  pthread_mutex_unlock(&print_mutex);

  va_end(args);
}

struct Arg
{
  int id;
  SafeQueue *sq;
};

void *producer(struct Arg *producer_arg)
{
  SafeQueue *sq = producer_arg->sq;
  int id = producer_arg->id;

  for (int i = 0; i < NUM_ITEMS; i++)
  {
    int *item = malloc(sizeof(int));
    int number = id * NUM_ITEMS + i;
    *item = number;

    // mutex_printf("  producer[%d]: Producing item %d\n", id, number);

    usleep(rand() % PRODUCER_SLEEP);
    item = SafeQueue_push(sq, item);

    if (item)
    {
      mutex_printf("  producer[%d]: Recieved for item %d\n", id, number);
      free(item);
      break;
    }

    // mutex_printf("  producer[%d]: produced %d\n", id, number);
    SafeQueue_Debug(sq, &print_mutex);
  }

  mutex_printf("producer[%d]: Exited\n", id);

  pthread_exit(NULL);
}

void *consumer(struct Arg *consumer_arg)
{
  SafeQueue *sq = consumer_arg->sq;
  int id = consumer_arg->id;

  for (;;)
  {
    int *item = SafeQueue_pop(sq);

    if (!item)
    {
      mutex_printf("  consumer[%d]: Recieved cancellation\n", id);
      break;
    }

    // mutex_printf("  consumer[%d]: Consuming item %d\n", id, *item);

    usleep(rand() % CONSUMER_SLEEP);

    // mutex_printf("  consumer[%d]: consumed item %d\n", id, *item);
    SafeQueue_Debug(sq, &print_mutex);

    free(item);
  }

  mutex_printf("consumer[%d]: Exited\n", id);

  pthread_exit(NULL);
}

void print_item(void *item)
{
  printf("%d", *(int *)item);
}

int main()
{
  for (int timeout = 1; timeout < 2; timeout++)
  {
    SafeQueue sq = SafeQueue_new(QUEUE_CAPACITY);

    sq.print_item = print_item;

    // single thread test first
    int *item = malloc(sizeof(int));
    *item = 42;
    SafeQueue_push(&sq, item);

    item = SafeQueue_pop(&sq);
    if (*item != 42)
    {
      perror("single thread test failed");
      return 1;
    }
    free(item);

    pthread_t producer_threads[NUM_PRODUCERS];
    pthread_t consumer_threads[NUM_CONSUMERS];

    struct Arg producer_args[NUM_PRODUCERS];
    struct Arg consumer_args[NUM_CONSUMERS];

    // create producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
      producer_args[i] = (struct Arg){i, &sq};
      pthread_create(&producer_threads[i], NULL, (void *(*)(void *))producer, &producer_args[i]);
    }

    // create consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++)
    {
      consumer_args[i] = (struct Arg){i, &sq};
      pthread_create(&consumer_threads[i], NULL, (void *(*)(void *))consumer, &consumer_args[i]);
    }

    usleep(3000000);

    void **items = SafeQueue_exit(&sq, timeout * 3000000);

    // join producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++)
      if (pthread_join(producer_threads[i], NULL))
        goto err;

    // join consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++)
      if (pthread_join(consumer_threads[i], NULL))
        goto err;
    if (items)
    {
      if (timeout)
      {
        printf("Timed out\n");
      }

      printf("Unprocessed items in the queue:\n");
      for (int i = 0; items[i]; i++)
      {
        printf(" %4d: ", i);
        print_item(items[i]);
        printf("\n");
        free(items[i]);
      }
      free(items);
    }

    SafeQueue_free(&sq);
  }

  return 0;

err:
  perror("pthread_create");
  return 1;
}
