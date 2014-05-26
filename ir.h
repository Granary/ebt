// language representation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#ifndef SJ_IR_H
#define SJ_IR_H

#include <string>
#include <vector>

/* from util.h */
struct translator_output;

struct sj_file {
  sj_file ();
  sj_file (const std::string& name);

  std::string name;

  // TODOXXX
};

struct sj_module {
private:
  std::vector<sj_file *> script_files;

public:
  bool has_contents;
  std::string script_name; // short path or "<command line>"
  std::string script_path; // FILENAME
  std::string script_contents; // -e PROGRAM

  void compile();
  void emit_dr_client(translator_output &o) const;

  // TODOXXX
};

#endif // SJ_IR_H
