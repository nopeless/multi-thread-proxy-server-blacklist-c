#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bigboi.h"

#undef _BIGBOI_NODE_FIELDS

static inline void *_BigBoiNode_init_routine(const size_t size, const size_t capacity)
{
  _BigBoiNode *bb = malloc(size);

  if (!bb)
    return NULL;

  bb->data = malloc(capacity * sizeof(*bb->data));

  if (!bb->data)
  {
    free(bb);
    return NULL;
  }

  bb->length = 0;
  bb->capacity = capacity;
  bb->next = NULL;

  return bb;
}

BigBoi *BigBoi_new(const size_t initial_capacity)
{

  BigBoi *bb = _BigBoiNode_init_routine(sizeof(BigBoi), initial_capacity);

  if (!bb)
    return NULL;

  bb->total_length = 0;
  bb->last = (_BigBoiNode *)bb;

  return bb;
}

_BigBoiNode *_BigBoiNode_new(const size_t capacity)
{
  return _BigBoiNode_init_routine(sizeof(_BigBoiNode), capacity);
}

size_t BigBoi_append_str(BigBoi *restrict const head, const char *restrict str)
{
  _BigBoiNode *bb = head->last;
  size_t sesh_written = 0;

  for (;;)
  {
    // nothing to write
    if (!*str)
      break;

    // if can write data
    if (bb->length < bb->capacity)
    {
      char *dend = bb->data + bb->capacity;
      char *dst = bb->data + bb->length;

      for (; dst < dend; dst++, str++)
      {
        if (!(*dst = *str))
        {
          break;
        }
      }

      size_t written = bb->capacity - bb->length - (dend - dst);

      // Update lengths
      bb->length += written;
      sesh_written += written;
    }

    // if no more data to write
    if (!*str)
      break;

    if (bb->next)
    {
      bb = bb->next;
      continue;
    }

    // allocate new node
    _BigBoiNode *new_node = _BigBoiNode_new(bb->capacity << 1);

    if (!new_node)
      break;

    bb->next = new_node;
    bb = new_node;
  }

  head->last = bb;
  head->total_length += sesh_written;

  return sesh_written;
}

size_t BigBoi_append_strn(BigBoi *restrict const head, const char *restrict str, size_t str_len)
{
  _BigBoiNode *bb = head->last;
  size_t sesh_written = 0;

  for (;;)
  {
    if (str_len == 0)
      break;

    size_t free_space = bb->capacity - bb->length;

    if (str_len <= free_space)
    {
      memcpy(bb->data + bb->length, str, str_len);

      bb->length += str_len;
      sesh_written += str_len;

      break;
    }

    if (free_space > 0)
    {
      memcpy(bb->data + bb->length, str, free_space);

      bb->length += free_space;
      sesh_written += free_space;

      str += free_space;
      str_len -= free_space;
    }

    if (bb->next)
    {
      bb = bb->next;
      continue;
    }

    // allocate new node
    _BigBoiNode *new_node = _BigBoiNode_new(bb->capacity << 1);

    if (!new_node)
      break;

    bb->next = new_node;
    bb = new_node;
  }

  head->last = bb;
  head->total_length += sesh_written;

  return sesh_written;
}

char *BigBoi_to_str(BigBoi *restrict head)
{
  char *str = malloc(sizeof(*str) * head->total_length + 1);

  if (!str)
    return NULL;

  char *p = str;

  _BigBoiNode *bb = (_BigBoiNode *)head;
  for (; bb; bb = bb->next)
  {
    if (bb->length == 0)
      break;

    memcpy(p, bb->data, bb->length);

    p += bb->length;
  }

  *p = '\0';

  return str;
}

void BigBoi_free(BigBoi *restrict const head)
{
  _BigBoiNode *bb = (_BigBoiNode *)head;
  for (; bb;)
  {
    free(bb->data);
    _BigBoiNode *tmp = bb->next;
    free(bb);
    bb = tmp;
  }
}

/**
 * Resets without freeing the memory
 */
void BigBoi_reset(BigBoi *restrict const head)
{
  _BigBoiNode *bb = (_BigBoiNode *)head;
  for (; bb;)
  {
    bb->length = 0;
    bb = bb->next;
  }

  head->total_length = 0;
  head->last = (_BigBoiNode *)head;
}

void BigBoi_debug_print(BigBoi *restrict const head)
{
  printf("[BigBoi] %p\n", (void *)head);
  printf("    Total Length: %ld\n", head->total_length);
  printf("    Last: %p\n", (void *)head->last);

  _BigBoiNode *bb = (_BigBoiNode *)head;

  // incredible
  for (int i = 0; bb; i++)
  {
    printf("[BigBoi %d] %p\n", i, (void *)bb);
    printf("    Node: %p\n", (void *)bb);
    printf("    Data: %p\n", (void *)bb->data);
    printf("      = \"%.*s\"\n", (int)bb->length, bb->data);
    printf("    Length: %ld\n", bb->length);
    printf("    Capacity: %ld\n", bb->capacity);
    printf("    Next: %p\n", (void *)bb->next);

    bb = bb->next;
  }
}
