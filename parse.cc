// language parser
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#include <string>
#include <set>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdexcept>

#include <assert.h>

using namespace std;

#include "ir.h"
#include "parse.h"

// --- pretty printing functions ---

std::ostream&
operator << (std::ostream& o, const source_loc& loc)
{
  return o << loc.file->name << ":" << loc.line << ":" << loc.col;
}

std::ostream&
operator << (std::ostream& o, const token& tok)
{
  string ttype;
  switch (tok.type) {
  case tok_op: ttype = "tok_op"; break;
  case tok_ident: ttype = "tok_ident"; break;
  case tok_str: ttype = "tok_str"; break;
  case tok_num: ttype = "tok_num"; break;
  }

  o << " \"";
  for (unsigned i = 0; i < tok.content.length(); i++) {
    char c = tok.content[i]; o << (isprint (c) ? c : '?');
  }
  o << "\" at " << tok.location;

  return o;
}

// --- error handling ---

struct parse_error: public runtime_error
{
  const token* tok;
  parse_error (const string& msg): runtime_error (msg), tok(0) {}
  parse_error (const string& msg, const token* t): runtime_error (msg), tok(t) {}
};

// --- lexing functions ---

class lexer
{
private:
  sj_module *m;
  sj_file *f;

  string input_contents;
  unsigned cursor;
  unsigned curr_line;
  unsigned curr_col;

  int input_get();
  int input_peek(unsigned offset = 0);

  token *scan();

  // use to store lookahead:
  token *last_tok;
  token *next_tok;

public:
  lexer(sj_module* m, sj_file *f, istream& i);

  static set<string> keywords;
  static set<string> operators;

  // TODOXXX make scan() public and move these below ones to parser
  token *peek();
  token *next();

  // Clear unused last_tok and next_tok; skips any lookahead tokens:
  void swallow();
  // XXX check and re-check memory management for our tokens!
};

set<string> lexer::keywords;
set<string> lexer::operators;


lexer::lexer(sj_module* m, sj_file *f, istream& input)
  : m(m), f(f), cursor(0), curr_line(1), curr_col(1),
    last_tok(NULL), next_tok(NULL)
{
  getline(input, input_contents, '\0'); // TODOXXX really works?
  // TODOXXX record the end of input_contents

  if (keywords.empty())
    {
      keywords.insert("probe");

      keywords.insert("not");
      keywords.insert("and");
      keywords.insert("or");
    }

  if (operators.empty())
    {
      // XXX: all operators currently at most 2 characters
      operators.insert("{");
      operators.insert("}");
      operators.insert("(");
      operators.insert(")");

      operators.insert(".");

      operators.insert("$");
      // operators.insert("@");

      operators.insert("*");
      operators.insert("/");
      operators.insert("%");
      operators.insert("+");
      operators.insert("-");
      operators.insert("!");
      operators.insert("~");
      // operators.insert(">>");
      // operators.insert("<<");
      operators.insert("<");
      operators.insert(">");
      operators.insert("<=");
      operators.insert(">=");
      operators.insert("==");
      operators.insert("!=");
      // operators.insert("&");
      // operators.insert("^");
      // operators.insert("|");
      operators.insert("&&");
      operators.insert("||");
      
      operators.insert("?");
      operators.insert(":");
    }
}


int
lexer::input_peek(unsigned offset)
{
  // TODOXXX replace this nonsense
  // if (cursor + offset >= input_contents.size()) {
  //   // fetch another line of data, or return EOF
  //   // XXX this whole arrangement is kind of strange
  //   char c;
  //   while (cursor + offset >= input_contents.size()) {
  //     char c;
  //     while (input.get(c), !input.eof()) {
  //       input_contents.push_back(c);
  //       if (c == '\n') break;
  //     }
  //     if (input.eof() || input.fail()) break;
  //   }

  if (cursor + offset >= input_contents.size())
    return -1; // EOF or failed read
    // }
  return input_contents[cursor + offset];
}

int
lexer::input_get()
{
  char c = input_peek();
  if (c < 0) return c; // EOF

  // advance cursor
  cursor++;
  if (c == '\n') {
    curr_line++; curr_col = 1;
  } else curr_col++;

  return c;
}

token *
lexer::scan()
{
  token *t = new token;
  t->location.file = f;

 skip:
  t->location.line = curr_line;
  t->location.col = curr_col;

  int c = input_get();
  if (c < 0) { // end of file
    delete t;
    return NULL;
  }

  if (isspace (c))
    goto skip;

  int c2 = input_peek();

  if (c == '/' && c2 == '*') { // found C style comment
    (void) input_get(); // consume '*'
    c = input_get(); c2 = input_get();
    while (c2 >= 0) {
      if (c == '*' && c2 == '/') break;
      c = c2; c2 = input_get();
    }
    if (c2 < 0) throw parse_error("unclosed comment"); // XXX
    goto skip;
  } else if (c == '#' || (c == '/' && c2 == '/')) { // found C++ or shell style comment
    unsigned initial_line = curr_line;
    do { c = input_get(); } while (c >= 0 && curr_line == initial_line);
    goto skip;
  }

  if (isalpha(c) || c == '_') { // found identifier
    t->type = tok_ident;
    t->content.push_back(c);
    while (isalnum (c2) || c2 == '_')
      {
        input_get ();
        t->content.push_back(c2);
        c2 = input_peek ();
      }
    if (keywords.count(t->content))
      t->type = tok_op;
    return t;
  } else if (isdigit(c)) { // found integer literal
    t->type = tok_num;
    t->content.push_back(c);
    // XXX slurp alphanumeric chars first, figure out if it's a valid number later
    while (isalnum (c2))
      {
        input_get ();
        t->content.push_back(c2);
        c2 = input_peek ();
      }
    return t;
  } else if (c == '\"') { // found string literal
    t->type = tok_str;
    for (;;) {
      c = input_get();

      if (c < 0 || c == '\n')
        {
          throw parse_error("could not find matching closing quote", t);
        }
      if (c == '\"')
        break;
      else if (c == '\\')
        {
          c = input_get();
          switch (c)
            {
            case 'a':
            case 'b':
            case 't':
            case 'n':
            case 'v':
            case 'f':
            case 'r':
            case 'x':
            case '0' ... '7': // an octal code
            case '\\':
              // XXX Maintain these escapes in the string as-is, in
              // case we want to emit the string as a C literal.
              t->content.push_back('\\');

            default:
              t->content.push_back(c);
              break;
            }
        }
      else
        t->content.push_back(c);
    }

    return t;
  } else if (ispunct (c)) { // found at least a one-character operator
    t->type = tok_op;

    string s, s2;
    s.push_back(c); s2.push_back(c);
    s2.push_back(c2);

    if (operators.count(s2)) { // valid two-character operator
      input_get(); // consume c2
      t->content = s2;
    } else if (operators.count(s)) { // valid single-character operator
      t->content = s;
    } else {
      t->content = ispunct(c2) ? s2 : s;
      throw parse_error("unrecognized operator", t);
    }

    return t;
  } else { // found an unrecognized symbol
    t->type = tok_op;
    ostringstream s;
    s << "\\x" << hex << setw(2) << setfill('0') << c;
    t->content = s.str();
    throw parse_error("unexpected junk symbol", t);
  }
}

token *
lexer::next()
{
  if (! next_tok) next_tok = scan();
  if (! next_tok) throw parse_error("unexpected end of file");

  last_tok = next_tok;
  next_tok = NULL;
  return last_tok;
}

token *
lexer::peek()
{
  if (! next_tok) next_tok = scan();

  last_tok = next_tok;
  return last_tok;
}

void
lexer::swallow()
{
  assert (last_tok != NULL);
  delete last_tok;
  last_tok = next_tok = NULL;
}

// --- parsing functions ---

class parser
{
private:
  sj_module *m;
  sj_file *f;
  lexer input;

public:
  parser(sj_module* m, const string& name, istream& i)
    : m(m), f(new sj_file(name)), input(lexer(m, f, i)) {}

  sj_file *parse();
};

sj_file *
parse (sj_module* m, istream& i, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  parser p (m, name, i);
  return p.parse();
}

sj_file *
parse (sj_module* m, const string& n, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  istringstream i (n);
  parser p (m, name, i);
  return p.parse();
}

sj_file *
parser::parse()
{
  // TODOXXX replace this placeholder
  try {
    token *t;
    while (t = input.peek()) {
      cerr << *t << endl; input.next(); input.swallow();
    }
  } catch (const parse_error& pe) {
    // TODOXXX use input.last_tok if no token in error
    cerr << "parse error:" << ' ' << pe.what() << endl;

    if (pe.tok) {
      cerr << "\tat: " << *pe.tok << endl;
      // TODOXXX print original source line
    }
  }

  return f;
}
