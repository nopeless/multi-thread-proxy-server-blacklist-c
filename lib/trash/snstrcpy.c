#include <string.h>

/*
 * Copy from source to dst, not including nul, but ending on src's null
 *
 * @return number of bytes copied (including NUL)
 */
size_t
snstrcpy(char *restrict dst, size_t dlen, const char *restrict src)
{
  int count = 0;

  for (; count < dlen; count++)
  {
    if (!(*dst++ = *src++))
      return count;
  }

  return dlen;
}
