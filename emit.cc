// backend generation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#include <iostream>

using namespace std;

#include "emit.h"
#include "ir.h"
#include "util.h"

// --- fake backend, for testing purposes ---

void
fake_client_template::register_probe(basic_probe *bp)
{
  basic_probes[bp->mechanism].push_back(bp);
}

void
fake_client_template::emit(translator_output& o) const
{
  cerr << "RESULTING BASIC PROBES (by mechanism)" << endl;
  for (map<basic_probe_type, vector<basic_probe *> >::const_iterator it = basic_probes.begin();
       it != basic_probes.end(); it++)
    {
      if (it != basic_probes.begin())
        cerr << "---" << endl;

      for (vector<basic_probe *>::const_iterator jt = it->second.begin();
           jt != it->second.end(); jt++)
        {
          (*jt)->print(cerr);
          cerr << endl;
        }
    }
}

// --- DynamoRIO backend ---

void
dr_client_template::register_probe(basic_probe *bp)
{
  // TODOXXX
}

// TODOXXX add in the actual probes
void
dr_client_template::emit(translator_output& o) const
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
