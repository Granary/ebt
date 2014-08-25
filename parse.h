// language parser
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef SJ_PARSE_H
#define SJ_PARSE_H

#include <string>
#include <iostream>

/* from ir.h */
struct sj_module;
struct sj_file;

struct source_loc
{
  sj_file *file;
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

void test_lexer (sj_module* m, std::istream& i,
                 const std::string source_name = "");
void test_lexer (sj_module* m, const std::string& n,
                 const std::string source_name = "");

sj_file *parse (sj_module* m, std::istream& i,
                const std::string source_name = "");
sj_file *parse (sj_module* m, const std::string& n,
                const std::string source_name = "");

#endif // SJ_PARSE_H
