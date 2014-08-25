// misc utilities
// Copyright (C) 2014 Serguei Makarov
// Copyright (C) 2005-2013 Red Hat Inc.
// Copyright (C) 2005-2008 Intel Corporation.
// Copyright (C) 2010 Novell Corporation.
//
// This file is part of SJ, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

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
