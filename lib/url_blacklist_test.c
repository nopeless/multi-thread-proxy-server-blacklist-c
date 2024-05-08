#include <stdio.h>
#include <stdlib.h>
#include "url_blacklist.h"

int main(int argc, char const *argv[])
{
  UrlBlacklist bl;
  UrlBlacklist_new(&bl, "blacklist.txt", '\n', 8);

  UrlBlacklist_print_table(&bl);

  char *test_strings[] = {
      "google.com",
      "www.google.com",
      "aws.www.google.com",
      "sex.com",
      "sex.c",
      "sexy.com",
      "sexy.c",
      "asex.c",
      "asex.com",
      "test.com",
      "testing.com",
      "something.com",
      "anything.com",
      "porn.xxx",
      "porn.net",
      "prn.xxx",
      "p.xxx",
      "x.xxx",
      "p.d.com",
      "po.com",
      "xcom",
  };

  for (int p = 0; p < sizeof(test_strings) / sizeof(*test_strings); p++)
  {
    char *r = UrlBlacklist_exists(&bl, test_strings[p]);
    printf("-> %-20s %s\n\n", test_strings[p], "false\0true" + 6 * !!r);
  }

  UrlBlacklist_free(&bl);

  return 0;
}
