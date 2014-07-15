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

#ifndef SJ_UTIL_H
#define SJ_UTIL_H

#include <iostream>

/* XXX this is a primitive version of translator-output.cxx from systemtap */
class translator_output {
  std::ostream& o;
  unsigned tablevel;

public:
  translator_output(std::ostream &file);

  std::ostream& newline (int indent_amount = 0);
  void indent (int amount);

  std::ostream& line();
};

std::string c_stringify(const std::string &unescaped);
std::string c_comment(const std::string &unescaped);

#endif // SJ_UTIL_H
