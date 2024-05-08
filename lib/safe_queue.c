#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include "safe_queue.h"

/**
 * Rejects push operations
 */
safequeue_status_t SAFE_QUEUE_REJECT_PUSH = 1 << 0;
/**
 * Rejects pop operations
 */
safequeue_status_t SAFE_QUEUE_REJECT_POP = 1 << 1;

/**
 * Queue is invalid. Further operations will be ignored
 */
safequeue_status_t SAFE_QUEUE_EXITED = 1 << 6;

/**
 * If memory has been collected
 */
safequeue_status_t SAFE_QUEUE_COLLECTED = 1 << 7;

static __always_inline int next_idx(unsigned int idx, unsigned int capacity)
{
  return ++idx >= capacity ? 0 : idx;
}

/**
 * Initialize a new SafeQueue
 */
SafeQueue SafeQueue_new(unsigned int capacity)
{
  void **queue = calloc(capacity, sizeof(void *));

  return (SafeQueue){
      queue,
      capacity,
      .head_idx = 0,
      .tail_idx = 0,

      .mutex = PTHREAD_MUTEX_INITIALIZER,
      .trigger_pop = PTHREAD_COND_INITIALIZER,
      .trigger_push = PTHREAD_COND_INITIALIZER,

      .status = 0,

      .print_item = NULL,
  };
}

/**
 * NULL if freed. Free the SafeQueue. FREE THE POINTER YOURSELF
 */
SafeQueue *SafeQueue_free(SafeQueue *sq)
{
  pthread_mutex_lock(&sq->mutex);

  if (sq->status & SAFE_QUEUE_COLLECTED)
  {
    pthread_mutex_unlock(&sq->mutex);
    return NULL;
  }

  if (!(sq->status & SAFE_QUEUE_EXITED))
  {
    pthread_mutex_unlock(&sq->mutex);
    return sq;
  }

  free(sq->queue);

  pthread_mutex_unlock(&sq->mutex);

  pthread_mutex_destroy(&sq->mutex);
  pthread_cond_destroy(&sq->trigger_pop);
  pthread_cond_destroy(&sq->trigger_push);

  sq->status |= SAFE_QUEUE_COLLECTED;

  return NULL;
}

/**
 * Push an item to the SafeQueue. If NULL, pushed. If not null, free the item
 */
void *SafeQueue_push(SafeQueue *sq, void *item)
{
  pthread_mutex_lock(&sq->mutex);

  if (sq->status & (SAFE_QUEUE_EXITED | SAFE_QUEUE_REJECT_PUSH) || !item)
    goto unlock_item;

  while (sq->queue[sq->head_idx]) // not empty
  {
    pthread_cond_wait(&sq->trigger_push, &sq->mutex);

    if (sq->status & SAFE_QUEUE_REJECT_PUSH)
      goto unlock_item;
  }

  sq->queue[sq->head_idx] = item;
  sq->head_idx = next_idx(sq->head_idx, sq->capacity);

  pthread_cond_signal(&sq->trigger_pop);
  pthread_mutex_unlock(&sq->mutex);

  return NULL;

unlock_item:
  pthread_mutex_unlock(&sq->mutex);
  return item;
}

/**
 * Blocking call until next item
 *
 * If NULL, the queue is no longer valid and should terminate
 */
void *SafeQueue_pop(SafeQueue *sq)
{
  pthread_mutex_lock(&sq->mutex);

  if (sq->status & (SAFE_QUEUE_EXITED | SAFE_QUEUE_REJECT_POP))
    goto unlock_null;

  void *item = sq->queue[sq->tail_idx];

  while (!item)
  {
    pthread_cond_wait(&sq->trigger_pop, &sq->mutex);

    if (sq->status & SAFE_QUEUE_REJECT_POP)
      goto unlock_null;

    item = sq->queue[sq->tail_idx];
  }

  sq->queue[sq->tail_idx] = NULL;
  sq->tail_idx = next_idx(sq->tail_idx, sq->capacity);

  pthread_cond_signal(&sq->trigger_push);
  pthread_mutex_unlock(&sq->mutex);

  return item;

unlock_null:
  pthread_mutex_unlock(&sq->mutex);
  return NULL;
}

static inline void timespec_add_ns(struct timespec *ts, unsigned long int duration_ns)
{
  ts->tv_nsec += duration_ns;
  ts->tv_sec += ts->tv_nsec / 1000000000;
  ts->tv_nsec %= 1000000000;
}

/**
 * if mode is 0, immediately exit
 *
 * Returns remaining items in the queue, null terminated
 */
void **SafeQueue_exit(SafeQueue *sq, long int timeout_ns)
{
  pthread_mutex_lock(&sq->mutex);

  void **items = NULL;

  if (timeout_ns == 0)
  {
    // kill every pusher
    sq->status |= SAFE_QUEUE_REJECT_PUSH;
    pthread_cond_broadcast(&sq->trigger_push);

    // kill every popper
    sq->status |= SAFE_QUEUE_REJECT_POP;
    pthread_cond_broadcast(&sq->trigger_pop);
  }
  else
  {
    if (timeout_ns > 0)
    {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);

      timespec_add_ns(&ts, timeout_ns);

      while (sq->queue[sq->tail_idx])
      {
        int result = pthread_cond_timedwait(&sq->trigger_push, &sq->mutex, &ts);
        if (result == ETIMEDOUT)
          break;
      }
    }
    else
    {
      while (sq->queue[sq->tail_idx])
      {
        pthread_cond_wait(&sq->trigger_push, &sq->mutex);
      }
    }
    // there is nothing in the queue

    // kill every pusher
    sq->status |= SAFE_QUEUE_REJECT_PUSH;
    pthread_cond_broadcast(&sq->trigger_push);

    // kill every popper
    sq->status |= SAFE_QUEUE_REJECT_POP;
    pthread_cond_broadcast(&sq->trigger_pop);
  }

  // has items
  if (sq->queue[sq->tail_idx])
  {
    if (sq->tail_idx < sq->head_idx)
    {
      // TAIL...HEAD
      unsigned int len = sq->head_idx - sq->tail_idx;
      items = malloc((len + 1) * sizeof(void *));

      for (unsigned int i = 0; i < len; i++)
      {
        items[i] = sq->queue[sq->tail_idx + i];
      }

      items[len] = NULL;
    }
    else
    {
      // ...HEAD TAIL...
      unsigned int tail_count = sq->capacity - sq->tail_idx;
      unsigned int len = tail_count + sq->head_idx;

      items = malloc((len + 1) * sizeof(void *));

      unsigned int i = 0;

      for (; i < tail_count; i++)
      {
        items[i] = sq->queue[sq->tail_idx + i];
      }

      for (; i < len; i++)
      {
        items[i] = sq->queue[i - tail_count];
      }

      items[len] = NULL;
    }
  }

  sq->status |= SAFE_QUEUE_EXITED;

  pthread_mutex_unlock(&sq->mutex);

  return items;
}

/**
 * NOT THREADSAFE: Return queue length
 */
inline unsigned int _SafeQueue_length(SafeQueue *sq)
{
  unsigned int length =
      sq->queue[sq->head_idx]
          ? sq->capacity
          : (sq->head_idx < sq->tail_idx
                 ? sq->head_idx + sq->capacity - sq->tail_idx
                 : sq->head_idx - sq->tail_idx);

  return length;
}

/**
 * pthread_mutex_lock(&sq->mutex);
 */
inline int SafeQueue_lock(SafeQueue *sq)
{
  return pthread_mutex_lock(&sq->mutex);
}

/**
 * pthread_mutex_unlock(&sq->mutex);
 */
inline int SafeQueue_unlock(SafeQueue *sq)
{
  return pthread_mutex_unlock(&sq->mutex);
}

void SafeQueue_Debug(SafeQueue *sq, pthread_mutex_t *stdout_mutex)
{
  pthread_mutex_lock(&sq->mutex);

  if (sq->status & SAFE_QUEUE_COLLECTED)
  {
    pthread_mutex_unlock(&sq->mutex);

    pthread_mutex_lock(stdout_mutex);
    printf("[SafeQueue]: <invalid>\n");
    pthread_mutex_unlock(stdout_mutex);

    return;
  }

  if (stdout_mutex)
    pthread_mutex_lock(stdout_mutex);

  printf("[SafeQueue]: %p\n", (void *)sq);
  printf("  capacity: %d\n", sq->capacity);
  printf("  head_idx: %d\n", sq->head_idx);
  printf("  tail_idx: %d\n", sq->tail_idx);

  printf("  queue[%d]: [\n", _SafeQueue_length(sq));
  for (int i = 0; i < sq->capacity; i++)
  {
    printf(
        "  %c %p",
        i == sq->head_idx && i == sq->tail_idx
            ? '*'
        : i == sq->head_idx
            ? '>'
        : i == sq->tail_idx
            ? '<'
            : ' ',
        sq->queue[i]);
    if (sq->queue[i] && sq->print_item)
    {
      printf(" -> ");
      sq->print_item(sq->queue[i]);
    }
    printf("\n");
  }

  printf("  ]\n");

  if (stdout_mutex)
    pthread_mutex_unlock(stdout_mutex);

  pthread_mutex_unlock(&sq->mutex);

  return;
}

safequeue_status_t SafeQueue_exited(SafeQueue *sq)
{
  return sq->status & SAFE_QUEUE_EXITED;
}
