/**
 * Custom implementation of big string
 */
#include <stddef.h>

#ifndef BIGBOI_H
#define BIGBOI_H

// intrusive linked list-ish?
#define _BIGBOI_NODE_FIELDS \
  char *restrict data;      \
  size_t length;            \
  struct _BigBoiNode *next; \
  size_t capacity;

typedef struct _BigBoiNode
{
  _BIGBOI_NODE_FIELDS
} _BigBoiNode;

typedef struct BigBoi
{
  _BIGBOI_NODE_FIELDS

  /**
   * BigBoi's Last node in the list
   */
  struct _BigBoiNode *last;
  /**
   * BigBoi's Total length of the string
   */
  size_t total_length;
} BigBoi;

/**
 * Creates a new BigBoi with no string
 */
BigBoi *BigBoi_new(const size_t initial_capacity);

size_t BigBoi_append_str(BigBoi *restrict const bigboi, const char *restrict str);
size_t BigBoi_append_strn(BigBoi *restrict const bigboi, const char *restrict str, size_t str_len);

char *BigBoi_to_str(BigBoi *restrict const bigboi);

void BigBoi_free(BigBoi *restrict const bigboi);

void BigBoi_reset(BigBoi *restrict const bigboi);

void BigBoi_debug_print(BigBoi *restrict const bigboi);

#endif
