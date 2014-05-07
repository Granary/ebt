#include <stdio.h>
#include <stdlib.h>

static void usage(const char *prog_name)
{
  fprintf(stderr,
          "Usage: %s [options]\n"
          "\n"
          "Options and arguments:\n"
          "  -g file : output script to file, instead of stdout\n",
          prog_name);
}

int main (int argc, char * const argv []) {
  usage(argv[0]);
  exit(1);
}
