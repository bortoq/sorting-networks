/* sorter main */
#include "sorter.h"
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
  const char *cmd = argc > 1 ? argv[1] : "help";

  if (strcmp(cmd, "proof") == 0 || strcmp(cmd, "test") == 0)
  {
    size target = 0;
    int have_target = 0;
    if (argc > 2)
    {
      target = (size)strtoul(argv[2], NULL, 10);
      have_target = 1;
    }
    return run_proof_cmd(target, have_target);
  }

  if (strcmp(cmd, "search") == 0 || strcmp(cmd, "bld") == 0)
  {
    size target = 0;
    size extra = 1;
    int have_target = 0;

    if (argc > 2)
    {
      target = (size)strtoul(argv[2], NULL, 10);
      have_target = 1;
    }
    if (argc > 3)
      extra = (size)strtoul(argv[3], NULL, 10);
    return run_search_cmd(target, have_target, extra);
  }

  fputs(HELP, stdout);
  return 0;
}
