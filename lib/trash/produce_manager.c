#include <stdio.h>
#include <pthread.h>

/**
 * Role division
 *
 * Orchestrator: Single thread
 *   - int jobId = spawn(void *)
 *   - Spawns producer and provides them with a pool to use (owned by Orchestrator)
 * Producer: Multiple threads
 * Consumer: Single thread
 */

/**
 * Thread-safe
 * Indicate that there is a pool ptr to be processed
 *
 * W  by producer `write(*p_idx++)` -> `= DIRTY`
 * RW by consumer `& DIRTY` -> `= 0` -> `read(*p_idx); *idx = 0; }`
 */
const char THREAD_FLAG_DIRTY = 1 << 0;
/**
 * RW Mutex
 */
const char THREAD_FLAG_MUTEX = 1 << 1;
/**
 * Thread-safe
 * W by orchestrator `!DIRTY && !OCCUPIED` -> `= 1`
 * W by producer `= 0` (if producer thread crashes then this is like, forever occupied)
 */
const char THREAD_FLAG_OCCUPIED = 1 << 7;

typedef struct OrchestratorThreadPool
{
  /**
   * Exclusively owned by producers where producerIdx owns thread_flags[producerIdx]
   *
   * (should) live on the stack 🔥🔥🔥🔥🔥
   *
   * Is not owned by this struct
   */
  volatile char *threads_flags;
  /**
   * Pool of pointers
   *
   * Thread-safe
   * RW by producer `== 0` -> `= (!=0)`
   * RW by consumer `==` `= 0`
   */
  volatile size_t *pool;
  /**
   * Maxium number of rows in the pool
   */
  const unsigned int pool_size;
  /**
   * Size of each row in the pool (each thread owns this amount of pointers)
   */
  const unsigned int thread_ptr_count;
  /**
   * Number of rows in the pool (number of threads)
   */
  const unsigned int thread_count;
  /**
   * Pointer to the next row to be consumed
   */
  unsigned int thread_idx;
} OrchestratorThreadPool;

/**
 * Please provide flags from stack
 */
volatile OrchestratorThreadPool OrchestratorThreadPool_new(volatile char *const thread_flags, const unsigned int thread_count, const unsigned int thread_ptr_count)
{
  unsigned int pool_size = thread_count * thread_ptr_count * sizeof(size_t);

  // create pool
  void *s = calloc(pool_size);
  if (!s)
  {
    return NULL;
  }

  char *threads_flags = (char *)(pool + pool_size);

  *s = (OrchestratorThreadPool){
      threads_flags,
      .thread_ptr_count = thread_ptr_count,
      .thread_count = thread_count,
      .pool_size = thread_count * thread_ptr_count,
      .thread_idx = 0,
  };

  return otp;

  pthread_cond_t j;
  ;
}

void OrchestratorThreadPool_free(const OrchestratorThreadPool *restrict const tp)
{
  free(tp->pool);
}

typedef struct ThreadPool
{
  /**
   * (producer) Always use |= to set flags
   * (consumer) Always use &= ~ to clear flags
   */
  volatile char *restrict const flags;
  /**
   * (producer) Only assign non-zero values to zero values
   * (consumer) Only consume non-zero values and set to zero
   */
  volatile size_t *restrict const ptrs;
  /**
   * Size of pointer pool
   */
  const unsigned int ptrs_size;
  /**
   * Can only be changed by producer
   */
  volatile void *p_idx;
  /**
   * ThreadPool args (DO NOT FREE)
   */
  void *restrict const args;
} ThreadPool;

const ThreadPool ThreadPool_get(const OrchestratorThreadPool *restrict const tp, const unsigned int thread_idx)
{
  volatile char *const restrict flags = tp->threads_flags + thread_idx;
  const unsigned int ptrs_size = tp->thread_ptr_count;
  return (ThreadPool){
      .ptrs = tp->pool + thread_idx * tp->thread_ptr_count,
      flags,
      ptrs_size,
  };
}

/**
 * Only assigns non-zero values (producer)
 */
volatile const void *restrict const *ThreadPoolProducer_put(const ThreadPool *restrict const tp, const void *const ptr)
{
  unsigned int o_idx = tp->idx;
  unsigned int find = o_idx;

  for (; find < tp->size; find++)
  {
    if (tp->ptrs[find])
      continue;

    tp->ptrs[find] = (size_t)ptr;

    goto end;
  }

  for (find = 0; find < o_idx; find++)
  {
    if (tp->ptrs[find])
      continue;

    tp->ptrs[find] = (size_t)ptr;
  }

end:
  return tp->ptrs + find;
}

/**
 * Only consumes non-zero values (consumer)
 */
