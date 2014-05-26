// misc utilities
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.
//
// This file directly incorporates code from translator-output.cxx
// from Systemtap. Please be sure to either remove this before
// releasing the project, or to license the project under GPLv2 or
// later.

#include <iostream>

#include "util.h"

using namespace std;

translator_output::translator_output (ostream &file) : o(file), tablevel(0) {}

ostream&
translator_output::newline (int indent)
{
  tablevel += indent;
  o << "\n";
  for (unsigned i = 0; i < tablevel; i++)
    o << "  ";
  return o;
}

ostream&
translator_output::line ()
{
  return o;
}
