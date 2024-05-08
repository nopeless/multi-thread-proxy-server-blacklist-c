#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bigboi.h"

int main(void)
{
  // // 8
  // char str[] = "test hii";

  // // 4
  // char dest[8] = {0};

  // size_t nd = snstrcpy(dest, 8, str);

  // printf("%s\n", dest + nd);

  // // print locations of str dest and nd
  // printf("str: %p\n", str);
  // printf("dest: %p\n", dest);
  // printf("nd: %lx\n", nd);

  // printf("HI\n");

  char *res = "ttetesttest1234testtest12345678";

  BigBoi *bb;
  char *s;

  bb = BigBoi_new(1);

  BigBoi_append_str(bb, "t");
  BigBoi_append_str(bb, "te");
  BigBoi_append_str(bb, "test");
  BigBoi_append_str(bb, "test123");
  BigBoi_append_str(bb, "4testtest12345678");

  BigBoi_debug_print(bb);

  s = BigBoi_to_str(bb);
  BigBoi_free(bb);
  printf("%s\n", s);

  if (strcmp(s, res))
    goto failed;

  free(s);

  bb = BigBoi_new(1);

  BigBoi_append_strn(bb, "t", 1);
  BigBoi_append_strn(bb, "te", 2);
  BigBoi_append_strn(bb, "test", 4);
  BigBoi_append_strn(bb, "test123", 7);
  BigBoi_append_strn(bb, "4testtest12345678", 17);

  BigBoi_debug_print(bb);

  s = BigBoi_to_str(bb);
  printf("%s\n", s);

  if (strcmp(s, res))
    goto failed;

  free(s);
  BigBoi_reset(bb);

  BigBoi_append_str(bb, "t");
  BigBoi_append_str(bb, "te");
  BigBoi_append_str(bb, "test");
  BigBoi_append_str(bb, "test123");
  BigBoi_append_str(bb, "4testtest12345678");

  BigBoi_debug_print(bb);

  s = BigBoi_to_str(bb);
  printf("%s\n", s);

  if (strcmp(s, res))
    goto failed;

  free(s);
  BigBoi_reset(bb);

  BigBoi_append_strn(bb, "t", 1);
  BigBoi_append_strn(bb, "te", 2);
  BigBoi_append_strn(bb, "test", 4);
  BigBoi_append_strn(bb, "test123", 7);
  BigBoi_append_strn(bb, "4testtest12345678", 17);

  BigBoi_debug_print(bb);

  s = BigBoi_to_str(bb);
  BigBoi_free(bb);
  printf("%s\n", s);

  if (strcmp(s, res))
    goto failed;
  free(s);

  return 0;

failed:
  free(s);
  printf("FAILED\n");
}
