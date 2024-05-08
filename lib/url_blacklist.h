/**
 * Extremely rudimentary set implementation for strings that are delimted by a character
 */
#include <stdlib.h>

#ifndef _CHR_DELIM_SET_H
#define _CHR_DELIM_SET_H

typedef struct UrlBlacklist
{
  int fd;
  char *file;
  unsigned int file_size;
  char **table;
  char delim;
  unsigned int mask;
} UrlBlacklist;

/**
 * Create a new UrlBlacklist
 *
 * @param filename The name of the file to read from
 * @param delim The delimiter character
 * @param table_size_2 The size of the table to use (2^table_size_2)
 *
 * 20 -> 1MB
 */
UrlBlacklist *UrlBlacklist_new(UrlBlacklist *cds, char *filename, const char delim, u_int8_t table_size_2);
void UrlBlacklist_free(UrlBlacklist *cds);

char *UrlBlacklist_exists(UrlBlacklist *cds, char *url);
char *UrlBlacklist_get_rule(UrlBlacklist *cds, char *url);
void UrlBlacklist_print_table(UrlBlacklist *bl);

#endif
