// misc utilities
// Copyright (C) 2014 Serguei Makarov
// Copyright (C) 2005-2013 Red Hat Inc.
// Copyright (C) 2005-2008 Intel Corporation.
// Copyright (C) 2010 Novell Corporation.
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

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
      if (c == '\n') { result.push_back('\\'); result.push_back('n'); continue; } // TODOXXX may want to omit newline altogether
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
