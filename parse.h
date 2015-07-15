// language parser
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef EBT_PARSE_H
#define EBT_PARSE_H

#include <string>
#include <iostream>

/* from ir.h */
struct ebt_module;
struct ebt_file;

struct source_loc
{
  ebt_file *file;
  unsigned line;
  unsigned col;
};

std::ostream& operator << (std::ostream& o, const source_loc& loc);

enum tok_type { tok_op, tok_ident, tok_str, tok_num };

struct token
{
  source_loc location;
  tok_type type;
  std::string content;
};

std::ostream& operator << (std::ostream& o, const token& tok);

void test_lexer (ebt_module* m, std::istream& i,
                 const std::string source_name = "");
void test_lexer (ebt_module* m, const std::string& n,
                 const std::string source_name = "");

ebt_file *parse (ebt_module* m, std::istream& i,
                const std::string source_name = "");
ebt_file *parse (ebt_module* m, const std::string& n,
                const std::string source_name = "");

#endif // EBT_PARSE_H
