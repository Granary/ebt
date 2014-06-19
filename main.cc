// main program
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#define SJ_VERSION_STRING "0.0"

#include <iostream>
#include <fstream>
#include <string>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
}

using namespace std;

#include "util.h"
#include "ir.h"

static void
usage(const char *prog_name)
{
  fprintf(stderr,
          "Usage: %s [options] FILENAME\n"
          "   or: %s [options] -e SCRIPT\n"
          "\n"
          "Options and arguments:\n"
          "  -e SCRIPT   : one-liner program\n"
          "  -g FILENAME : output script to file, instead of stdout\n"
          "  -p PASS     : stop after pass (0:lex, 1:parse, 2:resolve, 3:emit)\n",
          prog_name, prog_name);
  exit(1);
}

int
main (int argc, char * const argv [])
{
  sj_module script;
  script.has_contents = false;

  bool has_outfile = false;
  char *outfile_path = NULL;

  /* parse options */
  char c;
  while ((c = getopt(argc, argv, "g:e:p:")) != -1)
    {
      switch (c)
        {
        case 'e':
          script.has_contents = true;
          script.script_contents = string(optarg);
          script.script_name = "<command line>";
          break;
        case 'g':
          has_outfile = true;
          outfile_path = optarg;
          break;
        case 'p':
          char *num_endptr;
          script.last_pass = (int)strtoul(optarg, &num_endptr, 10);
          if (*num_endptr != '\0' || script.last_pass < 0 || script.last_pass > 3)
            {
              cerr << "Invalid pass number (should be 0-3)." << endl;
              // XXX print usage?
              exit(1);
            }
          break;
        default:
          usage(argv[0]);
        }
    }

  if (script.has_contents && optind < argc)
    {
      /* spurious non-option arguments exist */
      usage(argv[0]);
    }
  else if (!script.has_contents && optind != argc-1)
    {
      usage(argv[0]);
    }
  else if (!script.has_contents)
    {
      script.script_path = string(argv[optind]);
      script.script_name = string(basename(argv[optind]));
      // script.script_name = script.script_path;
    }

  // perform ast translation -- passes 0-2
  script.compile();

  // emit final module -- pass 3
  if (script.last_pass < 3)
    exit(0);

  ofstream outfile;
  if (has_outfile)
    {
      outfile.open(outfile_path);
      if (!outfile.is_open()) { perror("cannot open output file"); exit(1); }
    }

  translator_output o(has_outfile ? outfile : cout);
  o.line() << "/* generated by sj version " << SJ_VERSION_STRING << " */\n";
  script.emit_dr_client(o);
  if (has_outfile) outfile.close();
}
