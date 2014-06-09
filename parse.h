// language parser
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

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
