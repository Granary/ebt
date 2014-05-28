// language representation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "ir.h"
#include "parse.h"

using namespace std;

// --- methods for sj_file ---

sj_file::sj_file() {}
sj_file::sj_file(const string& name) : name(name) {}

// --- methods for sj_module ---

void
sj_module::compile()
{
  sj_file *f;
  if (has_contents)
    {
      istringstream input(script_contents);
      script_files.push_back(parse(this, input));
    }
  else
    {
      ifstream input(script_path.c_str());
      if (!input.is_open()) { perror("cannot open script file"); exit(1); } // TODOXXX: exit() doesn't really fit here -- use an rc
      script_files.push_back(parse(this, input));
    }
}

/* XXX no particularly fancy infrastructure for now */
void
sj_module::emit_dr_client(translator_output& o) const
{
  o.newline() << "#include \"dr_api.h\"";
  o.newline();

  o.newline() << "static void event_exit(void);";
  o.newline();

  o.newline() << "DR_EXPORT void";
  o.newline() << "dr_init(client_id_t id)";
  o.newline() << "{";
    o.newline(1) << "/* empty client */";
    o.newline() << "dr_register_exit_event(event_exit);";
  o.newline(-1) << "}";
  o.newline();

  o.newline() << "static void";
  o.newline() << "event_exit(void)";
  o.newline() << "{";
    o.newline(1) << "/* empty client */";
  o.newline(-1) << "}";
  o.newline();
}
