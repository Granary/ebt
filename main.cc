// main program
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

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
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <openssl/md5.h>
}

using namespace std;

#include "util.h"
#include "ir.h"
#include "emit.h"

// TODOXXX separate 'verbose' option (for the build process output)

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

void
system_command(vector<string> &args, string path = "")
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
      // cerr << "CHANGE DIRECTORY: " << path << endl; // TODOXXX
      chdir(path.c_str());
    }

  cerr << endl;
  cerr << "---" << endl;
  cerr << "sj: " << cmd << endl;

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
      int rc = execvp("/bin/sh", argv);
      string errmsg = "exec system command '" + cmd + "'";
      perror(errmsg.c_str());
      exit(1);
    }
  else
    waitpid(pid, &status, 0);

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
      cerr << "system command failed: " + cmd << endl;
      exit(1);
    }

  free(argv[2]);
  free(argv);
}

// --- command line parser and utility ---

static void
usage(const char *prog_name)
{
  fprintf(stderr,
          "Usage: %s [options] FILENAME [[--] target program to analyze]\n"
          "   or: %s [options] -e SCRIPT [[--] target program to analyze]\n"
          "\n"
          "Options and arguments:\n"
          "  -e SCRIPT   : one-liner program\n"
          "  -o          : stop after pass-3 and show the resulting client source\n"
          "  -g FILENAME : output client source to file, instead of stdout\n"
          "  -t PATH     : create build folder in PATH (defaults to /tmp)\n"
          "  -f          : (testing purposes only) output 'fake' client template\n"
          "  -p PASS     : stop after pass (0:lex, 1:parse, 2:resolve, 3:emit, 4:run)\n",
          // TODOXXX options for launching the target program
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
  // XXX want to switch to getopt_long
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
      mkdir(tmp_path.c_str(), STANDARD_PERMISSIONS);
      cerr << "sj: creating temporary directory " << tmp_path << endl;

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

  if (has_outfile) cerr << "sj: generated client " << outfile_path << endl;

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

  // XXX users need to make sure SJ_HOME is set
  char *sj_home_c = getenv("SJ_HOME");
  if (!sj_home_c)
    {
      cerr << "need to set SJ_HOME environment variable" << endl;
      exit(1);
    }
  string sj_home(sj_home_c);

  translator_output co(cmakefile);
  co.line() << "# generated by sj version " << SJ_VERSION_STRING << "\n";

  // XXX name the sj_client after the script??
  co.newline() << "set(CMAKE_C_FLAGS \"-I" + sj_home + "\")";
  co.newline();
  co.newline() << "add_library(sj_client SHARED script_output.c)";
  co.newline() << "find_package(DynamoRIO)";
  co.newline() << "if (NOT DynamoRIO_FOUND)";
  co.newline() << "  message(FATAL_ERROR \"DynamoRIO package required to build\")";
  co.newline() << "endif(NOT DynamoRIO_FOUND)";
  co.newline() << "configure_DynamoRIO_client(sj_client)";

  cmakefile.close();
  cerr << "sj: generated " << cmakefile_path << endl;
  // TODOXXX extra verbosity: show cmakefile contents

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
  dr_command.push_back(build_path + "/libsj_client.so");
  dr_command.push_back("--");
  dr_command.insert(dr_command.end(), target_command.begin(), target_command.end());
  // TODOXXX assemble command line for the target program
  system_command(dr_command, orig_path);

  // TODOXXX clean up: delete temporary directory
  // TODOXXX clean up: restore old working directory (before running DR?)
}
