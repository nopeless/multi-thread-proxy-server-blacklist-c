#include <stddef.h>
#include <pthread.h>

#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

typedef unsigned char safequeue_status_t;

typedef struct SafeQueue
{
  void **const queue;
  const unsigned int capacity;
  unsigned int head_idx;
  unsigned int tail_idx;

  pthread_mutex_t mutex;
  pthread_cond_t trigger_pop;
  pthread_cond_t trigger_push;

  safequeue_status_t status;

  void (*print_item)(void *);
} SafeQueue;

SafeQueue SafeQueue_new(unsigned int capacity);
SafeQueue *SafeQueue_free(SafeQueue *sq);

void *SafeQueue_push(SafeQueue *sq, void *item);

void *SafeQueue_pop(SafeQueue *sq);
void **SafeQueue_exit(SafeQueue *sq, long int timeout_ns);

unsigned int _SafeQueue_length(SafeQueue *sq);

int SafeQueue_lock(SafeQueue *sq);
int SafeQueue_unlock(SafeQueue *sq);

void SafeQueue_Debug(SafeQueue *sq, pthread_mutex_t *stdout_mutex);

safequeue_status_t SafeQueue_exited(SafeQueue *sq);

#endif
