#include <stdlib.h>
#include "url_blacklist.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

char is_whitespace(char *start, char *end)
{
  for (; start < end; start++)
  {
    if (*start != ' ' && *start != '\t')
      return 0;
  }
  return 1;
}

/**
 * Returns the "next" hash value
 */
unsigned int mutate(unsigned int hash)
{
  // linear congruential generator
  const unsigned int a = 1664525;
  const unsigned int c = 1013904223;

  return a * hash + c;
}

unsigned int digest(const char *str, unsigned const int len)
{
  unsigned int hash = 0;

  for (unsigned int i = 0; i < len; i++)
  {
    hash += (hash << 5) + *str++;
  }

  return mutate(hash);
}

/**
 * turns *sex*.com
 * into *.com
 */
unsigned int _get_glob_group(char *const dest, char *str, unsigned const int len)
{
  char *tail = dest;
  char *head = str;
  char *const end = str + len;

  char should_glob = 0;

  for (;;)
  {
    for (; !(*head == '.' || head == end);)
    {
      if (*head == '*')
        should_glob = 1;
      head++;
    }

    if (should_glob)
      *tail++ = '*';
    else
    {
      for (; str < head;)
        *tail++ = *str++;
    }

    if (head == end)
      break;

    *tail++ = *head++;
    str = head;
    should_glob = 0;
  }

  *tail = '\0';

  return tail - dest;
}

/**
 * str is null terminated
 *
 * the final character of glob should not be NULL or any character inside str
 */
char _glob_match(char *str, char *glob, unsigned const int glob_len)
{
  char *glob_end = glob + glob_len;

  char expanding = 0;

  for (;;)
  {
  start:
    char *head_str = str;
    char *head_glob = glob;

    while (*head_str == *head_glob)
    {
      if (*head_str == '.')
      {
        expanding = 0;
        str = head_str + 1;
        glob = head_glob + 1;
        goto start;
      }

      head_str++;
      head_glob++;
    }

    // yippie
    if (*head_glob == '*')
    {
      while (*head_glob == '*')
      {
        head_glob++;
      }

      expanding = 1;
      glob = head_glob;
      str = head_str;
      continue;
    }

    if (!*head_str || !expanding)
      return head_glob >= glob_end;

    str++;
  }
}

static inline unsigned int _item_length(UrlBlacklist *bl, char *item)
{
  char *end = memchr(item, bl->delim, bl->file_size);
  return end - item;
}

UrlBlacklist *UrlBlacklist_new(
    UrlBlacklist *bl,
    char *filename,
    const char delim,
    u_int8_t table_size_2)
{
  bl->table = calloc(1 << table_size_2, sizeof(char *));

  if (!bl->table)
    return NULL;

  // create memory mapped file
  int fd = open(filename, O_RDONLY, 0);
  bl->fd = fd;
  struct stat stat;
  fstat(fd, &stat);
  bl->file_size = stat.st_size;
  bl->file = mmap(NULL, bl->file_size, PROT_READ, MAP_SHARED, fd, 0);

  if (bl->file == MAP_FAILED)
  {
    free(bl->table);
    return NULL;
  }

  bl->delim = delim;

  // create mask
  bl->mask = (1 << table_size_2) - 1;

  char *start = bl->file;
  char *delim_pos = strchr(bl->file, delim);
  for (; delim_pos; start = delim_pos + 1, delim_pos = strchr(start, delim))
  {
    // skip empty lines
    if (delim_pos - start == 0)
      continue;

    // ignore lines that are only whitespace
    if (is_whitespace(start, delim_pos))
      continue;

    // ignore comment lines starting with #
    if (*start == '#')
      continue;

    // ignore starting host e.g. "0.0.0.0 " by skipping to next space
    // maximum length of IP address is 15 characters
    char *space_pos = memchr(start, ' ', 16);
    if (space_pos && space_pos < delim_pos)
      start = space_pos + 1;

    char *generic_rule = NULL;
    int generic_rule_len = 0;
    char should_free_generic_rule = 0;
    if (memchr(start, '*', delim_pos - start))
    {
      should_free_generic_rule = 1;
      generic_rule = malloc((delim_pos - start) * sizeof(char) + 1);
      generic_rule_len = _get_glob_group(generic_rule, start, delim_pos - start);
    }
    else
    {
      generic_rule = start;
      generic_rule_len = delim_pos - start;
    }

    // whitelist rule consume character
    char whitelist = 0;
    if (*start == '!')
    {
      whitelist = 1;
      start++;
      generic_rule++;
      generic_rule_len--;
    }

    unsigned int hash = digest(generic_rule, generic_rule_len);
    unsigned int index = hash & bl->mask;

    // #ifdef DEBUG
    //     printf("%-20.*s %-20.*s: %08x -> %04x %6d\n", (int)(delim_pos - start), start, generic_rule_len, generic_rule, hash, index, index);
    // #endif

    // got the hash
    if (should_free_generic_rule)
      free(generic_rule);

    unsigned int oi = index;

    for (; bl->table[index];)
    {
      // #ifdef DEBUG
      //       char *rule = UrlBlacklist_get_rule(bl, bl->table[index]);
      //       printf("Collision with: %s\n", rule);
      //       free(rule);
      // #endif
      // check if same item
      if (strncmp(bl->table[index], start, delim_pos - start) == 0)
        break;

      hash = mutate(hash);
      index = hash & bl->mask;

      if (index == oi)
        goto too_many_coll;
    }

    // #ifdef DEBUG
    //     printf("Adding rule: %5x %.*s\n", index, (int)(delim_pos - start), start);
    // #endif

    bl->table[index] = start - whitelist;
  }

  return bl;

too_many_coll:
  fprintf(stderr, "Too many collisions. Increase table size\n");
  free(bl->table);
  munmap(bl->file, bl->file_size);
  return NULL;
}

void UrlBlacklist_free(UrlBlacklist *bl)
{
  munmap(bl->file, bl->file_size);
  close(bl->fd);
  free(bl->table);
}

char *UrlBlacklist_exists(UrlBlacklist *bl, char *_url)
{

  // count number of dots
  int num_dots = 0;

  char *end = _url;
  for (; *end; end++)
  {
    if (*end == '.')
      num_dots++;
  }

  char *url = malloc(sizeof(*url) * (end - _url) + 1);
  memcpy(url, _url, ((end - _url) + 1) * sizeof(*_url));

  end += url - _url;

  // get segment locations
  //  www.google.com
  // ^   ^      ^   ^
  char **segment_locations = malloc((num_dots + 2) * sizeof(char *));
  segment_locations[0] = url - 1;
  segment_locations[num_dots + 1] = end;

  char *url_left, *url_right;

  end = url;
  for (int i = 0; *end; end++)
  {
    if (*end == '.')
      segment_locations[++i] = end;
  }

  // attempt to find exact match
  {
    unsigned int hash = digest(url, end - url);
    unsigned int index = hash & bl->mask;

    while (bl->table[index])
    {
      char *rule = bl->table[index];

      char whitelist = 0;
      if (*rule == '!')
      {
        whitelist = 1;
        rule++;
      }

      // do pattern matching
      if (!strncmp(url, rule, end - url))
      {
        free(url);
        free(segment_locations);

        return whitelist ? NULL : bl->table[index];
      }

      hash = mutate(hash);
      index = hash & bl->mask;
    }

    // exact match wasn't block_reason_rule
  }

  char *block_reason_rule = NULL;
  char found = 0;

  // <end> <.> <\0> plus user error ex: x..com
  char *permutation = malloc((end - url) + 2 + num_dots);
  char *perm_mem = permutation;
  char *permutation_end;

  char **segments = malloc(sizeof(char *) * (num_dots + 1));

  char *start = url;
  for (int i = 0; i < num_dots; i++)
  {
    char *dot = strchr(start, '.');
    segments[i] = malloc(dot - start + 1);
    strncpy(segments[i], start, dot - start);
    segments[i][dot - start] = '\0';
    start = dot + 1;
  }

  segments[num_dots] = malloc((end - start) + 1);
  strcpy(segments[num_dots], start);

  // print all segments
  // for (int i = 0; i < num_dots + 1; i++)
  // {
  //   printf("%s\n", segments[i]);
  // }

  for (int width = 0; width <= num_dots; width++)
  {
    int offset = num_dots - width;
    url_left = segment_locations[offset] + 1;
    url_right = segment_locations[offset + width + 1];

    int limit = 1 << width;
    for (int i = 0; i < limit; i++)
    {
      unsigned int bits = i << (sizeof(int) - 1 - width);
      unsigned int HIGH = 1 << (sizeof(int) - 1);
      permutation_end = permutation;

      for (int j = 0; j <= width; j++)
      {
        // check for high bit
        if (bits & HIGH)
          permutation_end += sprintf(permutation_end, "%s.", segments[offset + j]);
        else
          permutation_end += sprintf(permutation_end, "*.");

        bits <<= 1;
      }

      *--permutation_end = '\0';

      // printf("%s\n", permutation);

      goto permutation_yield;
    permutation_continue:
    }
  }

  // first run
  if (permutation != url && !found)
  {
    permutation = url;
    permutation_end = end;
    url_left = url;
    url_right = end;
    goto permutation_yield;
  }

prepare_return:
  // free all segments
  for (unsigned int i = 0; i < num_dots + 1; i++)
  {
    free(segments[i]);
  }

  free(segments);
  free(perm_mem);
  free(segment_locations);
  free(url);

  return block_reason_rule;

permutation_yield:

  // #ifdef DEBUG
  //   printf("Checking for subset match: %s\n", permutation);
  // #endif

  unsigned int hash = digest(permutation, permutation_end - permutation);
  unsigned int index = hash & bl->mask;

  while (bl->table[index])
  {
    char *rule = bl->table[index];
    char whitelist = 0;
    if (*rule == '!')
    {
      whitelist = 1;
      rule++;
    }

    char tmp = *url_right;
    *url_right = '\0';
#ifdef DEBUG
    char *r = UrlBlacklist_get_rule(bl, bl->table[index]);
    printf("Checking %s against %s\n", url_left, r);
    free(r);
#endif
    char match = _glob_match(url_left, rule, _item_length(bl, rule));
    *url_right = tmp;

    if (match)
    {
      found = 1;
      block_reason_rule = whitelist ? NULL : rule;
      goto prepare_return;
    }

    hash = mutate(hash);
    index = hash & bl->mask;
  }

  // #ifdef DEBUG
  //   printf("Subset match not block_reason_rule: %s\n", permutation);
  // #endif

  // fail
  goto permutation_continue;
}

/**
 * Returns an allocated string that is the copy of the rule but null terminated
 */
char *UrlBlacklist_get_rule(UrlBlacklist *bl, char *rule)
{
  unsigned int len = strchr(rule, bl->delim) - rule;
  char *copy = malloc(len + 1);
  strncpy(copy, rule, len);
  copy[len] = '\0';
  return copy;
}

void UrlBlacklist_print_table(UrlBlacklist *bl)
{
  const unsigned int MAX_ENTRIES = 0xf;

  printf("UrlBlacklist table [%d]: %p\n", bl->mask + 1, (void *)bl);

  unsigned int entry_count = 0;

  for (unsigned int i = 0; i < bl->mask + 1; i++)
  {
    if (!bl->table[i])
      continue;

    entry_count++;

    if (entry_count > MAX_ENTRIES)
      continue;

    char *rule = UrlBlacklist_get_rule(bl, bl->table[i]);
    printf("%5x %p %s", i, (void *)rule, rule);

    unsigned int hash = mutate(digest(rule, strlen(rule)));
    unsigned int index = hash & bl->mask;

    free(rule);

    while (bl->table[index])
    {
      rule = UrlBlacklist_get_rule(bl, bl->table[index]);
      printf(" -> %s", rule);
      free(rule);

      hash = mutate(hash);
      index = hash & bl->mask;
    }

    printf("\n");
  }

  if (entry_count > MAX_ENTRIES)
    printf("... %d more entries\n", entry_count - MAX_ENTRIES);

  printf("- end of table -\n");
}
