// main program
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#define EBT_VERSION_STRING "0.0"

#include <iostream>
#include <fstream>
#include <string>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <openssl/md5.h>
}

using namespace std;

#include "util.h"
#include "ir.h"
#include "emit.h"

// --- paraphernalia for dealing with the system ---

#define STANDARD_PERMISSIONS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

unsigned char hash_result[MD5_DIGEST_LENGTH];

// TODOXXX should also bake a timestamp into the hash, or use a caching mode
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

// Global settings used by system_command():
bool system_verbose;
ofstream *cnull;
ostream& mesg() { if (system_verbose) return cerr; else return *cnull; }

void
system_command(vector<string> &args, string path = "", bool show_output = false)
{
  /* Set up command line: */

  string cmd(args[0]);
  for (unsigned i = 1; i < args.size(); i++) cmd += " " + args[i];

  char ** argv = (char **) malloc(sizeof(char *) * 4);
  argv[0] = (char *) "/bin/sh";
  argv[1] = (char *) "-c";
  argv[2] = (char *) malloc(sizeof(char) * (cmd.size() + 1));
  strncpy(argv[2], cmd.c_str(), cmd.size() + 1);
  argv[3] = NULL;

  /* Set current working directory: */
  if (path != "")
    {
      chdir(path.c_str()); // XXX is this really the cleanest way?
    }

  mesg() << endl;
  mesg() << "---" << endl;
  mesg() << "ebt: " << cmd << endl;

  /* We need to wait for the command to complete. */
  int status;
  pid_t pid = fork();
  if (pid < 0)
    {
      string errmsg = "fork system command '" + cmd + "'";
      perror(errmsg.c_str());
      exit(1);
    }
  else if (pid == 0)
    {
      // XXX should probably configure the environment
      if (!system_verbose && !show_output) { close(1); close(2); open("/dev/null", O_WRONLY); open("/dev/null", O_WRONLY); } // XXX
      int rc = execvp("/bin/sh", argv);
      string errmsg = "exec system command '" + cmd + "'";
      perror(errmsg.c_str());
      exit(1);
    }
  else
    waitpid(pid, &status, 0);

  // XXX: drrun tends to like to exit with nonzero codes even if things are OK?
  if (!WIFEXITED(status) /*|| WEXITSTATUS(status) != 0*/)
    {
      cerr << "system command failed: " + cmd << endl;
      exit(1);
    }

  free(argv[2]);
  free(argv);
}

// --- command line parser and utility ---

static struct option long_options[] = {
  {"fake", no_argument, 0, 'f' },
  {"show-source", no_argument, 0, 'o' },
  {"verbose", no_argument, 0, 'v' },
  {0, 0, 0, 0}
};

static void
usage(const char *prog_name)
{
  fprintf(stderr,
          "Usage: %s [options] FILENAME [[--] target program to analyze]\n"
          "   or: %s [options] -e SCRIPT [[--] target program to analyze]\n"
          "\n"
          "Options and arguments:\n"
          "  -e SCRIPT        : one-liner program\n"
          "  -o --show-source : stop after pass-3 and show resulting client source\n"
          "  -g FILENAME      : output client source to file, instead of stdout\n"
          "  -t PATH          : create build folder in PATH (defaults to /tmp)\n"
          "  -f --fake        : (testing purposes only) output 'fake' client template\n"
          "  -v --verbose     : show output of the compilation process\n"
          "  -p PASS          : stop after pass (0:lex, 1:parse, 2:resolve, 3:emit, 4:run)\n",
          // XXX may want additional options for launching the target program
          prog_name, prog_name);
  exit(1);
}

int
main (int argc, char * const argv [])
{
  ebt_module script;
  script.has_contents = false;

  // whether to compile+run the client, or just generate source:
  bool run_client = true;

  // where to create a build folder:
  string tmp_prefix = "/tmp";

  bool has_outfile = false;
  string outfile_path;

  bool emit_fake_client = false;

  system_verbose = false;

  /* parse options */
  char c;
  while ((c = getopt_long(argc, argv, "g:e:p:fvot:", long_options, NULL)) != -1)
    {
      switch (c)
        {
        case 'f':
          emit_fake_client = true;
          run_client = false;
          script.last_pass = 3;
          break;
        case 'v':
          system_verbose = true;
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
          // XXX Logically, this should be mutually exclusive with -p:
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

  // Harmonize run_client and -p options:
  if (!run_client && script.last_pass == 4)
    {
      script.last_pass = 3;
      cerr << "Will not run the resulting client.";
    }
  if (script.last_pass < 4)
    run_client = false;

  // The first non-option filename is generally the script path.
  if (!script.has_contents)
    {
      if (optind >= argc) /* no script to run */
        usage(argv[0]);
      script.script_path = string(argv[optind]);
      script.script_name = string(basename(argv[optind]));
      // script.script_name = script.script_path;
      optind++;
    }
  if (script.has_contents)
    script.script_name = script.script_path = "<command line>";

  // The remaining arguments give the command line of the target program:
  if (!run_client && optind < argc) /* spurious non-option arguments exist */
    usage(argv[0]);

  vector<string> target_command;
  while (optind < argc)
    {
      target_command.push_back(string(argv[optind]));
      optind++;
    }

  if (run_client && target_command.empty()) /* no target program to run */
    {
      cerr << "no target program given" << endl; // XXX: should be optional?
      usage(argv[0]);
    }

  /////////////////////////////////////////
  // perform ast translation -- passes 0-2
  if (script.compile() != 0)
    exit (1);

  ///////////////////////////////
  // emit final module -- pass 3
  if (script.last_pass < 3)
    exit(0);

  // XXX Now comes the compilation step. We want to make sure output
  // is only emitted for error messages, and not any of the ordinary
  // compilation output, unless the verbose flag is set:
  ofstream cn("/dev/null"); cnull = &cn;

  // XXX users need to make sure EBT_HOME is set
  char *ebt_home_c = getenv("EBT_HOME");
  if (!ebt_home_c)
    {
      cerr << "need to set EBT_HOME environment variable" << endl;
      exit(1);
    }
  string ebt_home(ebt_home_c);

  // determine where to emit the client source
  string tmp_path;
  if (!has_outfile && run_client)
    {
      if (script.has_contents)
        tmp_path = tmp_prefix + "/ebt_" + md5hash(script.script_contents, true);
      else
        tmp_path = tmp_prefix + "/ebt_" + md5hash(script.script_path, false);
      // TODOXXX deal with the case when the target dir already exists
      mkdir(tmp_path.c_str(), STANDARD_PERMISSIONS);
      mesg() << "ebt: creating temporary directory " << tmp_path << endl;

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

  o.line() << "/* generated by ebt version " << EBT_VERSION_STRING << " */\n";
  if (emit_fake_client)
  {
    fake_client_template fake_template(&script);
    fake_template.emit(o);
  }
  else
  {
    dr_client_template dr_template(&script);
    dr_template.emit(o);
  }

  if (has_outfile) outfile.close();

  if (has_outfile) mesg() << "ebt: generated client " << outfile_path << endl;

  //////////////////////////////////
  // run the final module -- pass 4
  if (script.last_pass < 4)
    exit(0);

  // First save the current working directory:
  char *cwd = get_current_dir_name();
  string orig_path(cwd);
  free(cwd);

  // How to Compile and Run the DynamoRIO Client
  //
  // (1) Create script_output.c (done above).
  // (2) Emit CMakeLists.txt
  string cmakefile_path = tmp_path + "/CMakeLists.txt";
  ofstream cmakefile;
  cmakefile.open(cmakefile_path.c_str());
  if (!cmakefile.is_open())
    {
      perror("cannot open CmakeLists.txt for output");
      exit(1);
    }

  translator_output co(cmakefile);
  co.line() << "# generated by ebt version " << EBT_VERSION_STRING
    << "from " << script.script_path << "\n";

  // XXX name the ebt_client after the script??
  co.newline() << "set(CMAKE_C_FLAGS \"-I" + ebt_home + "\")";
  co.newline();
  co.newline() << "add_library(ebt_client SHARED script_output.c)";
  co.newline() << "find_package(DynamoRIO)";
  co.newline() << "if (NOT DynamoRIO_FOUND)";
  co.newline() << "  message(FATAL_ERROR \"DynamoRIO package required to build\")";
  co.newline() << "endif(NOT DynamoRIO_FOUND)";
  co.newline() << "configure_DynamoRIO_client(ebt_client)";
  co.newline();

  cmakefile.close();
  mesg() << "ebt: generated " << cmakefile_path << endl;
  // XXX Optional extra verbosity: show cmakefile contents

  // (3) Create directory build/
  string build_path = tmp_path + "/build";
  mkdir(build_path.c_str(), STANDARD_PERMISSIONS);

  // (4) Run CMake command
  vector<string> cmake_command;
  cmake_command.push_back("cmake");
  // XXX users need to make sure DYNAMORIO_HOME is set
  char *dr_home_c = getenv("DYNAMORIO_HOME"); string dr_home(dr_home_c);
  cmake_command.push_back("-DDynamoRIO_DIR=" + dr_home + "/cmake");
  cmake_command.push_back("..");
  system_command(cmake_command, build_path);

  // (5) Run Make command
  vector<string> make_command;
  make_command.push_back("make");
  system_command(make_command, build_path);

  // (6) Run DynamoRIO on the target program
  vector<string> dr_command;
  dr_command.push_back("drrun");
  dr_command.push_back("-root");
  dr_command.push_back(dr_home);
  dr_command.push_back("-c");
  dr_command.push_back(build_path + "/libebt_client.so");
  dr_command.push_back("--");
  dr_command.insert(dr_command.end(), target_command.begin(), target_command.end());
  // XXX assemble command line for the target program

  system_command(dr_command, orig_path, true);

  // XXX we may want to delete the temporary directory after running
}
