// language parser
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#include <string>
#include <iostream>

using namespace std;

#include "ir.h"
#include "parse.h"

// --- pretty printing functions ---

std::ostream&
operator << (std::ostream& o, const source_loc& loc)
{
  return o << loc.file->name << ":" << loc.line << ":" << loc.column;
}

std::ostream&
operator << (std::ostream& o, const token& tok)
{
  // TODOXXX
}

// --- parsing functions ---

sj_file *
parse (sj_module* m, std::istream& i, const std::string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  sj_file *result = new sj_file(name);
  // TODOXXX
  return result;
}

sj_file *
parse (sj_module* m, const std::string& n, const std::string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  sj_file *result = new sj_file(name);
  // TODOXXX
  return result;
}
