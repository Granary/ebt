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
translator_output::newline (int indent_amount)
{
  indent(indent_amount);
  o << "\n";
  for (unsigned i = 0; i < tablevel; i++)
    o << "  ";
  return o;
}

void
translator_output::indent (int amount)
{
  tablevel += amount;
}

ostream&
translator_output::line ()
{
  return o;
}

/* escape questionable stuff in a C string literal
   quick and dirty (meant for printf statements that emit original script code) */
string
c_stringify(const std::string &unescaped)
{
  string result("");
  for (unsigned i = 0; i < unescaped.size(); i++)
    {
      char c = unescaped[i];

      /* escape questionable stuff: */
      if (c == '\"' || c == '\\') result.push_back('\\');

      result.push_back(c);
    }
  return result;
}

/* output a string into a multiline C comment
   (does not support // C++-style comments) */
string
c_comment(const std::string &unescaped)
{
  string result("");
  for (unsigned i = 0; i < unescaped.size(); i++)
    {
      char c = unescaped[i];
      char c2 = i < unescaped.size() - 1 ? unescaped[i+1] : '\0';

      result.push_back(c);

      /* avoid inserting closing comment: */
      if (c == '*' && c2 == '/')
        result.push_back(' ');
    }
  return result;
}
