// language parser
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

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

////////////////////////////////////
// TODOXXX Language Features to Add
// - basic statements
// - functions
// - global variables
// - control flow constructs
// - basic event resolution language
// - dynamic probe instantiation (e.g. for watchpoints)
// - a more complex access syntax for static/dynamic data
// - associative arrays, statistical aggregates
// - command line arguments
// - basic try/catch error handling as in SystemTap

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
  switch (tok.type)
    {
    case tok_op: ttype = "tok_op"; break;
    case tok_ident: ttype = "tok_ident"; break;
    case tok_str: ttype = "tok_str"; break;
    case tok_num: ttype = "tok_num"; break;
    }

  o << " \"";
  for (unsigned i = 0; i < tok.content.length(); i++)
    {
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
  unsigned input_size;

  unsigned cursor;
  unsigned curr_line;
  unsigned curr_col;

  int input_get();
  int input_peek(unsigned offset = 0);

public:
  lexer(sj_module* m, sj_file *f, istream& i);

  static set<string> keywords;
  static set<string> operators;

  // Used to extract a subrange of the source code (e.g. for diagnostics):
  unsigned get_pos();
  string source(unsigned start, unsigned end);

  token *scan();

  // Clear unused last_tok and next_tok; skips any lookahead tokens:
  void swallow();
  // XXX check and re-check memory management for our tokens!
};

set<string> lexer::keywords;
set<string> lexer::operators;


lexer::lexer(sj_module* m, sj_file *f, istream& input)
  : m(m), f(f), cursor(0), curr_line(1), curr_col(1)
{
  getline(input, input_contents, '\0');
  input_size = input_contents.size();

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

      operators.insert(","); // XXX temporarily used for condition lists

      operators.insert(".");
      operators.insert("::");

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


unsigned
lexer::get_pos()
{
  return cursor;
}

string
lexer::source(unsigned start, unsigned end)
{
  return input_contents.substr(start, end);
}

int
lexer::input_peek(unsigned offset)
{
  if (cursor + offset >= input_size)
    return -1; // EOF or failed read
  return input_contents[cursor + offset];
}

int
lexer::input_get()
{
  char c = input_peek();
  if (c < 0) return c; // EOF

  // advance cursor
  cursor++;
  if (c == '\n')
    {
      curr_line++; curr_col = 1;
    }
  else
    curr_col++;

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
  if (c < 0)
    { // end of file
      delete t;
      return NULL;
    }

  if (isspace (c))
    goto skip;

  int c2 = input_peek();

  if (c == '/' && c2 == '*') // found C style comment
    {
      (void) input_get(); // consume '*'
      c = input_get(); c2 = input_get();
      while (c2 >= 0)
        {
          if (c == '*' && c2 == '/') break;
          c = c2; c2 = input_get();
        }
      if (c2 < 0) throw parse_error("unclosed comment"); // XXX
      goto skip;
    }
  else if (c == '#' || (c == '/' && c2 == '/')) // found C++ or shell style comment
    {
      unsigned initial_line = curr_line;
      do { c = input_get(); } while (c >= 0 && curr_line == initial_line);
      goto skip;
    }

  if (isalpha(c) || c == '_') // found identifier
    {
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
    }
  else if (isdigit(c)) // found integer literal
    {
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
    }
  else if (c == '\"') // found string literal
    {
      t->type = tok_str;
      for (;;)
        {
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
    }
  else if (ispunct (c)) // found at least a one-character operator
    {
      t->type = tok_op;

      string s, s2;
      s.push_back(c); s2.push_back(c);
      s2.push_back(c2);

      if (operators.count(s2)) // valid two-character operator
        {
          input_get(); // consume c2
          t->content = s2;
        }
      else if (operators.count(s)) // valid single-character operator
        {
          t->content = s;
        }
      else
        {
          t->content = ispunct(c2) ? s2 : s;
          throw parse_error("unrecognized operator", t);
        }

      return t;
    }
  else // found an unrecognized symbol
    {
      t->type = tok_op;
      ostringstream s;
      s << "\\x" << hex << setw(2) << setfill('0') << c;
      t->content = s.str();
      throw parse_error("unexpected junk symbol", t);
    }
}

// --- parsing functions ---

// Grammar Overview for the Language
//
// script ::= declaration script
// script ::=
//
// declaration ::= "probe" event_expr "{" "}"
// 
// event_expr ::= [event_expr "."] IDENTIFIER
// event_expr ::= event_expr "(" expr ")"
// event_expr ::= "not" event_expr
// event_expr ::= event_expr "and" event_expr
// event_expr ::= event_expr "or" event_expr
// event_expr ::= event_expr "::" event_expr
//
// designator ::= IDENTIFIER
// TODOXXX designator ::= designator "." IDENTIFIER
// TODOXXX designator ::= designator "." NUMBER
// TODOXXX designator ::= designator "[" expr "]"
//
// expr ::= ["$" | "@"] IDENTIFIER
// expr ::= NUMBER
// expr ::= STRING
// expr ::= "(" expr ")"
// expr ::= UNARY expr
// expr ::= expr BINARY expr
// expr ::= expr "?" expr ":" expr

class parser
{
private:
  sj_module *m;
  sj_file *f;
  lexer input;

  void print_error(const parse_error& pe);
  void throw_expect_error(const string& expected_what, token *t);

  // use to store lookahead:
  token *last_tok;
  token *next_tok;

  token *peek();
  token *next();
  void swallow();

  bool finished();

  bool peek_op(const string& op, token* &t);
  void swallow_op(const string& op);

public:
  parser(sj_module* m, const string& name, istream& i)
    : m(m), f(new sj_file(name)), input(lexer(m, f, i)),
      last_tok(NULL), next_tok(NULL) {}

  void test_lexer();

  sj_file *parse();

  // Handy-dandy precedence table, for expressions:
  // 0        - IDENTIFIER, LITERAL
  // 1 left   - $ @
  // 2 right  - unary + - ! ~
  // 3 left   - * / %
  // 4 left   - + -
  // 5 left   - << >>
  // 6 left   - < <= > >=
  // 7 left   - == !=
  // 8 left   - &
  // 9 left   - ^
  // 10 left  - |
  // 11 left  - &&
  // 12 left  - ||
  // 13 right - ?: // TODOXXX correct associativity?

  expr *parse_basic_expr();
  // XXX expr *parse_sigil();
  expr *parse_unary();
  expr *parse_multiplicative();
  expr *parse_additive();
  expr *parse_bitshift();
  expr *parse_comparison();
  expr *parse_equality();
  expr *parse_bitwise_and();
  expr *parse_bitwise_xor();
  expr *parse_bitwise_or();
  expr *parse_logical_and();
  expr *parse_logical_or();
  expr *parse_ternary();
  expr *parse_expr();

  // Handy-dandy precedence table, for events:
  // 0       - IDENTIFIER
  // 1 left  - EVENT . IDENTIFIER
  // 2 right - EVENT ( EXPR )
  // 3 left  - not EVENT
  // 4 left  - EVENT and EVENT
  // 5 left  - EVENT or EVENT
  // 6 left  - EVENT :: EVENT // TODOXXX correct associativity??

  // TODOXXX swap precedence levels 1 and 2, perhaps???

  event *parse_basic_event ();
  event *parse_subevent ();
  event *parse_conditional_event ();
  event *parse_event_not ();
  event *parse_event_and ();
  event *parse_event_or ();
  event *parse_event_restrict ();
  event *parse_event_expr ();

#ifdef ONLY_BASIC_PROBES
  basic_probe *parse_basic_probe ();
#endif
  probe *parse_probe ();
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


void
test_lexer (sj_module* m, istream& i, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  parser p (m, name, i);
  p.test_lexer();
}

void
test_lexer (sj_module* m, const string& n, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  istringstream i (n);
  parser p (m, name, i);
  p.test_lexer();
}


void
parser::test_lexer()
{
  try
    {
      token *t;
      while (t = peek())
        {
          cerr << *t << endl; swallow();
        }
    }
  catch (const parse_error& pe)
    {
      print_error(pe);
    }
}

void
parser::print_error(const parse_error& pe)
{
  // TODOXXX use input.last_tok if no token in error
  cerr << "parse error:" << ' ' << pe.what() << endl;

  if (pe.tok)
    {
      cerr << "         " << *pe.tok << endl;
      // TODOXXX print original source line
    }
}

// TODOXXX add in current token thing
void
parser::throw_expect_error(const string& expected_what, token *t)
{
  ostringstream what("");
  what << "expected " << expected_what;
  if (t)
    what << ", found " << *t; // XXX
  throw parse_error(what.str());
}

// --- parser state management ---

token *
parser::next()
{
  if (! next_tok) next_tok = input.scan();
  if (! next_tok) throw parse_error("unexpected end of file");

  last_tok = next_tok;
  next_tok = NULL;
  return last_tok;
}

token *
parser::peek()
{
  if (! next_tok) next_tok = input.scan();

  last_tok = next_tok;
  return last_tok;
}

void
parser::swallow()
{
  assert (last_tok != NULL);
  delete last_tok;
  last_tok = next_tok = NULL;
}

bool
parser::finished()
{
  return peek() == NULL;
}

bool
parser::peek_op(const string& op, token* &t)
{
  t = peek();
  return (t != NULL && t->type == tok_op && t->content == op);
}

void
parser::swallow_op(const string& op)
{
  token *t = peek();
  if (t == NULL || t->type != tok_op || t->content != op)
    {
      ostringstream s(""); s << "'" << op << "'"; 
      throw_expect_error(s.str(), t); // TODOXXX fix like this throughout
    }
  swallow();
}

// --- parsing expressions ---

expr *
parser::parse_basic_expr()
{
  token* t;

  if (peek_op("(", t))
    {
      swallow();
      expr* e = parse_expr();
      swallow_op(")");
      return e;
    }

  bool require_ident = false;
  basic_expr* e = new basic_expr;
  if (peek_op("$", t) || peek_op("@", t))
    {
      e->sigil = t; next();
      require_ident = true;
    }

  t = peek();
  if (require_ident && t->type != tok_ident)
    throw_expect_error("identifier", t);
  // TODOXXX process this stuff better
  if (t->type == tok_num || t->type == tok_str || t->type == tok_ident)
    {
      e->tok = t;
      next ();
      // TODOXXX permit identifier chaining
      return e;
    }
  else throw_expect_error("identifier, string, or number", t);
}

expr *
parser::parse_unary()
{
  token* t;
  if (peek_op("+", t) || peek_op("-", t) || peek_op("!", t) || peek_op("~", t))
    {
      unary_expr* e = new unary_expr;
      e->tok = t;
      e->op = t->content;
      next ();
      e->operand = parse_unary(); // XXX we allow that, right?
      return e;
    }
  else
    return parse_basic_expr();
}

expr *
parser::parse_multiplicative()
{
  expr *op1 = parse_unary ();

  token* t;
  while (peek_op("*", t) || peek_op("/", t) || peek_op("%", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_unary ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_additive()
{
  expr *op1 = parse_multiplicative ();

  token* t;
  while (peek_op("+", t) || peek_op("-", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_multiplicative ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_bitshift()
{
  expr *op1 = parse_additive ();

  token* t;
  while (peek_op("<<", t) || peek_op(">>", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_additive ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_comparison()
{
  expr *op1 = parse_bitshift ();

  token* t;
  while (peek_op("<", t) || peek_op("<=", t)
         || peek_op(">", t) || peek_op(">=", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_bitshift ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_equality()
{
  expr *op1 = parse_comparison ();

  token* t;
  while (peek_op("==", t) || peek_op("!=", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_comparison ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_bitwise_and()
{
  expr *op1 = parse_equality ();

  token* t;
  while (peek_op("&", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_equality ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_bitwise_xor()
{
  expr *op1 = parse_bitwise_and ();

  token* t;
  while (peek_op("^", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_bitwise_and ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_bitwise_or()
{
  expr *op1 = parse_bitwise_xor ();

  token* t;
  while (peek_op("|", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_bitwise_xor ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_logical_and()
{
  expr *op1 = parse_bitwise_or ();

  token* t;
  while (peek_op("&&", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_bitwise_or ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_logical_or()
{
  expr *op1 = parse_logical_and ();

  token* t;
  while (peek_op("||", t))
    {
      binary_expr* e = new binary_expr;
      e->tok = t;
      e->op = t->content;
      e->left = op1;
      next ();
      e->right = parse_logical_and ();
      op1 = e;
    }

  return op1;
}

expr *
parser::parse_ternary()
{
  expr *op1 = parse_logical_or ();

  token* t;
  if (peek_op("?", t))
    {
      conditional_expr *e = new conditional_expr;
      e->tok = t;
      e->cond = op1;
      next ();
      e->truevalue = parse_expr (); // XXX does this work?
      swallow_op(":");
      e->falsevalue = parse_ternary ();
      return e;
    }
  else
    return op1;
}

expr *
parser::parse_expr()
{
  return parse_ternary ();
}

// --- parsing event expressions ---

event *
parser::parse_basic_event ()
{
  token *t = peek();
  if (t->type != tok_ident) throw_expect_error("identifier", t);

  named_event *e = new named_event;
  e->tok = t;
  e->ident = t->content;
  e->subevent = NULL;

  next ();
  return e;
}

event *
parser::parse_subevent ()
{
  event *op1 = parse_basic_event (); // XXX parse_conditional_event(), perhaps?

  token *t;
  while (peek_op(".", t))
    {
      named_event *e = new named_event;
      e->tok = t;
      e->subevent = op1;
      next ();

      t = peek ();
      if (t->type != tok_ident) throw_expect_error("identifier", t);

      e->ident = t->content;
      swallow (); // XXX: don't need the actual token?

      op1 = e;
    }

  return op1;
}

event *
parser::parse_conditional_event ()
{
  event *op1 = parse_subevent ();
  conditional_event *e = NULL;

  token *t;
  while (peek_op("(", t))
    {
      e = new conditional_event;
      e->tok = t;
      e->subevent = op1;
      next ();
      e->condition = parse_expr ();
      swallow_op(")");

      op1 = e;
    }

  return op1;
}

event *
parser::parse_event_not ()
{
  token *t;
  if (peek_op("not", t))
    {
      compound_event *e = new compound_event;
      e->tok = t;
      e->op = t->content;
      next ();
      e->subevents.push_back(parse_event_not ());
      return e;
    }
  else
    return parse_conditional_event ();
}

event *
parser::parse_event_and ()
{
  event *op1 = parse_event_not ();
  compound_event *e = NULL;

  token* t;
  while (peek_op("and", t))
    {
      e = new compound_event;
      e->tok = t;
      e->op = t->content;
      e->subevents.push_back(op1);
      next ();

      op1 = parse_event_not ();
    }

  // capture the final operand
  if (e != NULL)
    {
      e->subevents.push_back(op1); op1 = e;
    }

  return op1;
}

event *
parser::parse_event_or ()
{
  event *op1 = parse_event_and ();
  compound_event *e = NULL;

  token* t;
  while (peek_op("or", t))
    {
      e = new compound_event;
      e->tok = t;
      e->op = t->content;
      e->subevents.push_back(op1); op1 = NULL;
      next ();

      op1 = parse_event_and ();
    }

  // capture the final operand
  if (e != NULL)
    {
      e->subevents.push_back(op1); op1 = NULL;
    }

  return op1 == NULL ? e : op1;
}

event *
parser::parse_event_restrict ()
{
  event *op1 = parse_event_or ();
  compound_event *e = NULL;

  token* t;
  while (peek_op("::", t))
    {
      e = new compound_event;
      e->tok = t;
      e->op = t->content;
      e->subevents.push_back(op1);
      next ();
      e->subevents.push_back(parse_event_or ());
      op1 = e;
    }

  return op1;
}

event *
parser::parse_event_expr ()
{
  return parse_event_restrict ();
}

// --- parsing toplevel declarations ---

#ifdef ONLY_BASIC_PROBES
basic_probe *
parser::parse_basic_probe ()
{
  // TODOXXX this is still very buggy...
  unsigned probe_start = input.get_pos();

  swallow_op("probe");

  basic_probe *bp = new basic_probe;

  token *t = peek();
  if (t->type != tok_ident)
    throw_expect_error("probe type: begin, end, insn", t); // TODOXXX fcall, freturn, malloc, maccess

  if (t->content == "insn")
    bp->mechanism = EV_INSN;
  else if (t->content == "begin")
    bp->mechanism = EV_BEGIN;
  else if (t->content == "end")
    bp->mechanism = EV_END;
  // else if (t->content == "fcall")
  //   bp->mechanism = EV_FCALL;
  // else if (t->content == "freturn")
  //   bp->mechanism = EV_FRETURN;
  else
    throw_expect_error("probe type: begin, end, insn", t); // TODOXXX fcall, freturn, malloc, maccess
  swallow();

  // parse conditions
  if (peek_op("(", t))
    {
      do
        {
          swallow();
          expr *e = parse_expr();
          condition *c = new condition;
          c->id = bp->get_ticket();
          c->content = e;
          bp->conditions.push_back(c);
        }
      while (peek_op(",", t));
      swallow_op(")");
    }

  bp->body = new handler; // TODOXXX
  bp->body->id = m->get_handler_ticket();

  bp->body->orig_source = input.source(probe_start, input.get_pos()); // TODOXXX only works well for oneliner probes -- need to collapse multiline condition chains

  swallow_op("{");
  swallow_op("}");

  return bp;
}
#endif

probe *
parser::parse_probe ()
{
  swallow_op("probe");

  probe *p = new probe;
  p->probe_point = parse_event_expr();

  // XXX probe handler would be parsed here
  swallow_op("{");
  swallow_op("}");

  return p;
}

sj_file *
parser::parse()
{
  try
    {
      for (;;)
        {
          token *t;
          if (peek_op("probe", t))
            {
#ifdef ONLY_BASIC_PROBES
              f->resolved_probes.push_back(parse_basic_probe());
#else
              f->probes.push_back(parse_probe());
#endif
            }
          else if (finished())
            break;
          else
            throw_expect_error("'probe' keyword", t);
        }
    }
  catch (const parse_error& pe)
    {
      print_error (pe);
      return NULL;
    }

  // TODOXXX if this is the last pass, print out a tree representation

  return f;
}


// TODOXXX Grammar Brainstorm for the Further Language
//
// script ::= declaration script
// script ::=
//
// declaration ::= "func" signature [ ":" type ]"{" statement "}"
// declaration ::= "class" signature "{" declaration "}"
// declaration ::= "probe" event_expr "{" statement "}"
// declaration ::= "var" IDENTIFIER [ "=" expr ]
//
// signature ::= IDENTIFIER "(" [ params ] ")"
// params ::= IDENTIFIER [":" type]
// params ::= params "," params
// type ::= "int"
// type ::= "str"
//
// statement ::= statement ";" statement
// statement ::= "{" statement "}"
// statement ::= expr // TODOXXX -- including assignment operations
// statement ::= "if" "(" expr ")" expr [ "else" expr ]
// statement ::= "while" "(" expr ")" statement
// statement ::= "for" "(" expr ";" expr ";" expr ")" statement
// statement ::= "while" "(" expr ")" statement
// statement ::= "break"
// statement ::= "continue"
// statement ::= "next"
// statement ::= "return" [ expr ]
