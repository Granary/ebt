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

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <openssl/md5.h>
}

using namespace std;

#include "util.h"
#include "ir.h"
#include "emit.h"

unsigned char hash_result[MD5_DIGEST_LENGTH];

string
md5hash(const string& path, bool oneliner)
{
  if (!oneliner)
    {
      int fd = open(path.c_str(), O_RDONLY);
      if (fd < 0) { perror("md5hash"); exit(-1); }

      struct stat statbuf;
      if (fstat(fd, &statbuf) < 0) { perror("md5hash"); exit(-1); }

      char *file_buffer = (char *)mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

      MD5((unsigned char*) file_buffer, statbuf.st_size, hash_result);

      munmap(file_buffer, statbuf.st_size);
    }
  else
    {
      MD5((unsigned char*) path.c_str(), path.length(), hash_result);
    }

  string result("");
  // use six hex digits as our hash
  for (unsigned i = 0; i < 3 && i < MD5_DIGEST_LENGTH; i++)
    {
      char hex[3]; sprintf(hex, "%02x", hash_result[i]);
      result.push_back(hex[0]); result.push_back(hex[1]);
    }
  return result;
}

static void
usage(const char *prog_name)
{
  fprintf(stderr,
          "Usage: %s [options] FILENAME\n"
          "   or: %s [options] -e SCRIPT\n"
          "\n"
          "Options and arguments:\n"
          "  -e SCRIPT   : one-liner program\n"
          "  -o          : stop after pass-3 and show the resulting client source\n"
          "  -g FILENAME : output client source to file, instead of stdout\n"
          "  -t PATH     : create build folder in PATH (defaults to /tmp)\n"
          "  -f          : (testing purposes only) output 'fake' client template\n"
          "  -p PASS     : stop after pass (0:lex, 1:parse, 2:resolve, 3:emit, 4:run)\n",
          // TODOXXX options for the target program
          prog_name, prog_name);
  exit(1);
}

int
main (int argc, char * const argv [])
{
  sj_module script;
  script.has_contents = false;

  // whether to compile+run the client, or just generate source:
  bool run_client = true;

  // where to create a build folder:
  string tmp_prefix = "/tmp"; // TODOXXX customizable

  bool has_outfile = false;
  string outfile_path;

  bool emit_fake_client = false;

  /* parse options */
  char c;
  while ((c = getopt(argc, argv, "g:e:p:fot:")) != -1)
    {
      switch (c)
        {
        case 'f':
          emit_fake_client = true;
          run_client = false;
          script.last_pass = 3;
          break;
        case 'e':
          script.has_contents = true;
          script.script_contents = string(optarg);
          script.script_name = "<command line>";
          break;
        case 't':
          tmp_prefix = string(optarg);
          break;
        case 'g':
          has_outfile = true;
          outfile_path = string(optarg);
        case 'o':
          // TODOXXX should be mutually exclusive with -p
          run_client = false;
          script.last_pass = 3;
          break;
        case 'p':
          char *num_endptr;
          script.last_pass = (int)strtoul(optarg, &num_endptr, 10);
          if (*num_endptr != '\0' || script.last_pass < 0 || script.last_pass > 4)
            {
              cerr << "Invalid pass number (should be 0-4)." << endl;
              // XXX print usage?
              exit(1);
            }
          break;
        default:
          usage(argv[0]);
        }
    }

  // TODOXXX UGLY harmonize run_client and -p option
  if (!run_client && script.last_pass == 4)
    {
      script.last_pass = 3;
      cerr << "Will not run the resulting client.";
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

  // determine where to emit the client source
  string tmp_path;
  if (!has_outfile && run_client)
    {
      if (script.has_contents)
        tmp_path = tmp_prefix + "/sj_" + md5hash(script.script_contents, true);
      else
        tmp_path = tmp_prefix + "/sj_" + md5hash(script.script_path, false);
      mkdir(tmp_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      cerr << "created temporary directory " << tmp_path << endl;

      outfile_path = tmp_path + "/script_output.c";
      has_outfile = true;
    }

  ofstream outfile;
  if (has_outfile)
    {
      outfile.open(outfile_path.c_str());
      if (!outfile.is_open()) { perror("cannot open output file"); exit(1); }
    }

  translator_output o(has_outfile ? outfile : cout);

  o.line() << "/* generated by sj version " << SJ_VERSION_STRING << " */\n";
  if (emit_fake_client)
    script.emit_fake_client(o);
  else
    script.emit_dr_client(o);

  if (has_outfile) outfile.close();

  // run the final module -- pass 4
  if (script.last_pass < 4)
    exit(0);

  cerr << "RUNNING THE CLIENT IS NOT YET SUPPORTED" << endl;

  // TODOXXX emit cmakefile for client
  // TODOXXX run cmake command
  // TODOXXX run make command
  // TODOXXX run dynamorio on the target program
  // TODOXXX clean up: delete temporary directory
}
