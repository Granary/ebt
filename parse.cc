// language parser
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#include <string>
#include <set>
#include <stack>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdexcept>

#include <assert.h>

using namespace std;

#include "ir.h"
#include "parse.h"
#include "util.h"

////////////////////////////////////
// TODOXXX Language Features to Add
// - special assignment operators += -= etc.
// - more sophisticated 'foreach' loop
// - expression for 'return' should be optional
// - more sophisticated 'func' declaration (e.g. with type annotations)
// - shadow memory tables
// - statistical aggregates
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

  o << "\"";
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
  bool suppress_tok;
  parse_error (const string& msg, bool suppress_tok = false)
    : runtime_error (msg), tok(0), suppress_tok(suppress_tok) {}
  parse_error (const string& msg, const token* t, bool suppress_tok = false)
    : runtime_error (msg), tok(t), suppress_tok(suppress_tok) {}
};

// ------------------------
// --- lexing functions ---
// ------------------------

class lexer
{
private:
  ebt_module *m;
  ebt_file *f;

  string input_contents;
  unsigned input_size;

  unsigned cursor;
  unsigned curr_line;
  unsigned curr_col;

  std::vector<unsigned> line_coords;

  int input_get();
  int input_peek(unsigned offset = 0);

public:
  lexer(ebt_module* m, ebt_file *f, istream& i);

  static set<string> keywords;
  static set<string> operators;

  // Used to extract a subrange of the source code (e.g. for diagnostics):
  unsigned get_pos();
  string source(unsigned start, unsigned end);
  string source_line(const token *tok);

  token *scan();

  // Clear unused last_tok and next_tok; skips any lookahead tokens:
  void swallow();
  // XXX check and re-check memory management for our tokens!
};

set<string> lexer::keywords;
set<string> lexer::operators;

lexer::lexer(ebt_module* m, ebt_file *f, istream& input)
  : m(m), f(f), cursor(0), curr_line(1), curr_col(1)
{
  getline(input, input_contents, '\0');
  input_size = input_contents.size();
  line_coords.push_back(0); // start of first line

  if (keywords.empty())
    {
      keywords.insert("probe");
      keywords.insert("global");
      keywords.insert("array");
      // XXX keywords.insert("shadow");
      keywords.insert("func");

      keywords.insert("not");
      keywords.insert("and");
      keywords.insert("or");

#ifdef ONLY_BASIC_PROBES
      // Not really keywords, thus should be checked with swallow_ident():
      //keywords.insert("begin");
      //keywords.insert("end");
      //keywords.insert("insn");
      //keywords.insert("function");
      //keywords.insert("entry");
      //keywords.insert("exit");
#endif

      keywords.insert("return");
      keywords.insert("if");
      keywords.insert("else");
      keywords.insert("while");
      keywords.insert("for");
      keywords.insert("foreach");
      keywords.insert("in");
      keywords.insert("break");
      keywords.insert("continue");
      keywords.insert("next");
    }

  if (operators.empty())
    {
      // XXX: all operators currently at most 2 characters
      operators.insert("{");
      operators.insert("}");
      operators.insert("(");
      operators.insert(")");
      operators.insert("[");
      operators.insert("]");

      operators.insert(",");
      operators.insert(";");
      
      operators.insert(".");
      operators.insert("::");

      operators.insert("$");
      operators.insert("@");

      operators.insert("*");
      operators.insert("/");
      operators.insert("%");
      operators.insert("+");
      operators.insert("-");
      operators.insert("!");
      operators.insert("~");
      operators.insert(">>");
      operators.insert("<<");
      operators.insert("<");
      operators.insert(">");
      operators.insert("<=");
      operators.insert(">=");
      operators.insert("==");
      operators.insert("!=");
      operators.insert("&");
      operators.insert("^");
      operators.insert("|");
      operators.insert("&&");
      operators.insert("||");

      operators.insert("?");
      operators.insert(":");

      operators.insert("="); // -- imperative assignment operation
      operators.insert("+=");
      operators.insert("-=");
      operators.insert("*=");
      operators.insert("/=");
      operators.insert("%=");
      operators.insert("<<=");
      operators.insert(">>=");
      operators.insert("&=");
      operators.insert("^=");
      operators.insert("|=");

      operators.insert("++");
      operators.insert("--");
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
  assert (end >= start);
  return input_contents.substr(start, end - start);
}

string
lexer::source_line(const token *tok)
{
  // TODOXXX When we implement multi-file support, will need to fix
  // this (as well as a whole bunch of other diagnostics stuff)
  // to print the line number from the approriate file for each token.
  unsigned line_num, line_start, line_next;
  line_num = tok ? tok->location.line : line_coords.size();
 retry:
  line_start = line_coords[line_num-1];
  line_next = line_start >= input_contents.size() ?
    input_contents.size() : input_contents.find('\n', line_start);

  // XXX When we are printing the last line:
  if (tok == NULL
      && (line_next == string::npos || line_next - line_start <= 0))
    {
      line_start = line_next;
      if (line_num > 0) { line_num --; goto retry; }
    }

  return input_contents.substr(line_start, line_next - line_start);
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
      line_coords.push_back(cursor);
      // -- XXX Recall the cursor shows the next character to look,
      //        i.e. the first character of the new line.
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
  int c3 = input_peek(1);

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

      string s, s2, s3;
      s.push_back(c); s2.push_back(c);
      s2.push_back(c2);

      if (operators.count(s3)) // valid three-character operator
        {
          input_get(); input_get(); // consume c2,c3
          t->content = s3;
        }
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
          t->content = ispunct(c2) && ispunct(c3) ? s3
            : ispunct(c2) ? s2 : s;
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

// -------------------------
// --- parsing functions ---
// -------------------------

// Grammar Overview for the Language
//
// script ::= declaration script
// script ::=
//
#ifndef ONLY_BASIC_PROBES
// declaration ::= "probe" event_expr "{" smts "}"
#else
// declaration ::= "probe" probe_type ["(" [conditions] ")"] "{" stmts "}"
#endif
// declaration ::= "global" IDENTIFIER ["=" expr]
// declaration ::= "array" IDENTIFIER
// declaration ::= "func" IDENTIFIER "(" [params_spec] ")" "{" stmts "}"
// XXX declaration ::= "shadow" IDENTIFIER ":" NUMBER
//
#ifndef ONLY_BASIC_PROBES
// event_expr ::= [event_expr "."] IDENTIFIER
// event_expr ::= event_expr "(" expr ")"
// event_expr ::= "not" event_expr
// event_expr ::= event_expr "and" event_expr
// event_expr ::= event_expr "or" event_expr
// event_expr ::= event_expr "::" event_expr
#else
// probe_type ::= "begin"
// probe_type ::= "end"
// probe_type ::= "insn"
// probe_type ::= "function" "." "entry"
// probe_type ::= "function" "." "exit"
// XXX probe_type ::= ...
// conditions ::= expr ["," conditions]
#endif
//
// stmts ::= stmt stmts
// stmts ::=
//
// stmt ::= ";"
// stmt ::= "{" stmts "}"
// stmt ::= expr
// stmt ::= "return" expr
// stmt ::= "if" "(" expr ")" stmt ["else" stmt]
// stmt ::= "while" "(" expr ")" stmt
// stmt ::= "for" "(" [expr] ";" [expr] ";" [expr] ")" stmt
// stmt ::= "foreach" "(" IDENTITIER "in" expr ")" stmt
// stmt ::= "break" | "continue" | "next"
//
// expr ::= ["$" | "@"] designator
// expr ::= NUMBER
// expr ::= STRING
// expr ::= "(" expr ")"
// expr ::= UNARY expr
// expr ::= expr ["++" | "--"]
// expr ::= expr BINARY expr
// expr ::= expr "?" expr ":" expr
// expr ::= IDENTIFIER "(" [params] ")"
//
// params ::= expr ["," params]
// params_spec ::= IDENTIFIER ["," params_spec]
//
// designator ::= IDENTIFIER
// designator ::= designator "." IDENTIFIER
// designator ::= designator "." NUMBER
// designator ::= designator "[" expr "]"

class parser
{
private:
  ebt_module *m;
  ebt_file *f;
  lexer input;

  void print_error(const parse_error& pe);
  void throw_expect_error(const string& expected_what, token *t = NULL);

  // use to store lookahead:
  token *last_tok;
  token *next_tok;

  token *peek();  // -- preview the next token
  token *next();  // -- get the next token and advance forward
  void swallow(); // -- get the next token, delete it, and advance forward

  bool finished();

  // These return true iff the next token is op:
  bool peek_op(const string& op, token* &t, bool throw_err = false);
  bool next_op(const string& op, token* &t, bool throw_err = false);
  bool swallow_op(const string& op, bool throw_err = true);

  // These return true iff the next token is an identifier:
  bool peek_ident(token* &t, bool throw_err = false);
  bool next_ident(token* &t, bool throw_err = false);
  bool swallow_ident(bool throw_err = true);
  bool swallow_ident(const string& text, bool throw_err = false);
    // -- require exact text

public:
  parser(ebt_module* m, const string& name, istream& i)
    : m(m), f(new ebt_file(name, m)), input(lexer(m, f, i)),
      last_tok(NULL), next_tok(NULL) {}

  void test_lexer();

  ebt_file *parse();

  // Precedence table for expressions:
  // 0        - IDENTIFIER, LITERAL
  // 1 left   - unary $ @
  // (1.5      - unary postfix ++ --)
  // 2 right  - unary + - ! ~, unary prefix ++ --
  // 3 left   - * / %
  // 4 left   - + -
  // 5 left   - << >>
  // 6 left   - < <= > >=
  // 7 left   - == !=
  // 8 left   - &
  // 9 left   - ^
  // 10 left  - |
  // 11 left  - in
  // 12 left  - &&
  // 13 left  - ||
  // 20 right - ?: // -- so `a ? b : c ? d : e` becomes `(a ? b : (c ? d : e))
  // 21 right - = += -= *= /= %= <<= >>= &= ^= |=
  // 22 left  - ,
  // 1000     - (not an operator)
#define PREC_NONE 1000
  unsigned prec_expr(const string &op) {
    // For binary operations only:
    if (op == "*" || op == "/" || op == "%") return 3;
    else if (op == "+" || op == "-") return 4;
    else if (op == "<<" || op == ">>") return 5;
    else if (op == "<" || op == "<=" || op == ">" || op == ">=") return 6;
    else if (op == "==" || op == "!=") return 7;
    else if (op == "&") return 8;
    else if (op == "^") return 9;
    else if (op == "|") return 10;
    else if (op == "in") return 11;
    else if (op == "&&") return 12;
    else if (op == "||") return 13;
    else if (op == "="
             || op == "+=" || op == "-=" || op == "*=" || op == "/="
             || op == "%=" || op == "<<=" || op == ">>="
             || op == "&=" || op == "^=" || op == "|=")
      return 21;
    else if (op == ",") return 22;
    else return PREC_NONE;
  }

  expr *parse_basic_expr();
  expr *parse_unary();
  expr *parse_binary(); // -- all binary operators except assignment
  expr *parse_ternary();
  expr *parse_assignment();
  expr *parse_comma();
  expr *parse_expr();

  stmt *parse_expr_stmt();
  stmt *parse_ifthen_stmt();
  stmt *parse_while_loop();
  stmt *parse_for_loop();
  stmt *parse_foreach_loop();
  stmt *parse_compound_stmt();
  stmt *parse_stmt();

  std::vector<stmt *> *parse_block(std::vector <stmt *> *stmts = NULL);

  // Precedence table for events:
  // 0       - IDENTIFIER
  // 1 left  - EVENT . IDENTIFIER
  // 2 right - EVENT ( EXPR )
  // 3 left  - not EVENT
  // 4 left  - EVENT and EVENT
  // 5 left  - EVENT or EVENT
  // 6 left  - EVENT :: EVENT
  // 1000    - (not an event operator)
  unsigned prec_event(const string& op)
  {
    if (op == "and") return 4;
    else if (op == "or") return 5;
    else if (op == "::") return 6;
    else return PREC_NONE;
  }

  event_expr *parse_basic_event ();
  event_expr *parse_event_not ();
  event_expr *parse_binary_event ();
  event_expr *parse_event_expr ();

#ifdef ONLY_BASIC_PROBES
  basic_probe *parse_basic_probe_decl ();
#endif
  probe *parse_probe_decl ();
  ebt_function *parse_func_decl();
  ebt_global *parse_global_decl();
  ebt_global *parse_array_decl();
};

ebt_file *
parse (ebt_module* m, istream& i, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  parser p (m, name, i);
  return p.parse();
}

ebt_file *
parse (ebt_module* m, const string& n, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  istringstream i (n);
  parser p (m, name, i);
  return p.parse();
}


void
test_lexer (ebt_module* m, istream& i, const string source_name)
{
  const std::string& name = source_name == "" ? m->script_name : source_name;
  parser p (m, name, i);
  p.test_lexer();
}

void
test_lexer (ebt_module* m, const string& n, const string source_name)
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

// --- parser error handling ---

void
parser::print_error(const parse_error& pe)
{
  cerr << "parse error:" << ' ' << pe.what() << endl;

  if (pe.tok)
    {
      if (!pe.suppress_tok)
        cerr << "             " << *pe.tok << endl;
      cerr << input.source_line(pe.tok) << endl;

      // Point out the position of the token in the line:
      for (unsigned i = 0; i < pe.tok->location.col - 1; i++)
        cerr << " ";
      cerr << "^" << endl;
    }
  // If pe.tok is unavailable and we are at end of input, print the last line:
  else if (!pe.tok && peek() == NULL)
    {
      string last_line = input.source_line(NULL);
      cerr << last_line << endl;

      // Point out the end of the line:
      for (unsigned i = 0; i < last_line.size(); i++)
        cerr << " ";
      cerr << "^" << endl;
    }
  // XXX If pe.tok is unavailable, we may want to use parser.last_tok
  // XXX Optionally this could benefit from some syntax coloring
  cerr << endl;
}

void
parser::throw_expect_error(const string& expected_what, token *t)
{
  // By default, print the upcoming token as what was 'found':
  if (t == NULL)
    t = peek();

  ostringstream what("");
  what << "expected " << expected_what;
  if (t)
    // XXX Aligned to "parse error:"
    what << ",\n             " << "found " << *t;
    // what << ", found " << *t;
    // XXX We may want to only print newline if expected_what is long enough.
  else
    what << ", found end of input";

  throw parse_error(what.str(),t,true);
}

// --- basic parser state management ---

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

// --- additional token handling utilities ---

bool
parser::peek_op(const string& op, token* &t, bool throw_err)
{
  t = peek();
  bool valid = (t != NULL && t->type == tok_op && t->content == op);
  if (!valid && throw_err)
    {
      ostringstream s(""); s << "'" << op << "'";
      throw_expect_error(s.str());
    }
  return valid;
}

bool
parser::next_op(const string& op, token* &t, bool throw_err)
{
  bool valid = peek_op(op,t,throw_err);
  if (valid) next();
  return valid;
}

bool
parser::swallow_op(const string& op, bool throw_err)
{
  token *t;
  bool valid = peek_op(op,t,throw_err);
  if (valid) swallow();
  return valid;
}

bool
parser::peek_ident(token* &t, bool throw_err)
{
  t = peek();
  bool valid = (t != NULL && t->type == tok_ident);
  if (!valid && throw_err)
    throw_expect_error("identifier");
  return valid;
}

bool
parser::next_ident(token* &t, bool throw_err)
{
  bool valid = peek_ident(t,throw_err);
  if (valid) next();
  return valid;
}

// XXX We may eventually want an analogous peek_ident() variant:
bool
parser::swallow_ident(const string& text, bool throw_err)
{
  token *t;
  bool valid = peek_ident(t,false) && t->content == text;
  if (valid) swallow();
  else if (throw_err)
    {
      ostringstream s(""); s << "'" << text << "'";
      throw_expect_error(s.str());
    }
  return valid;
}

bool
parser::swallow_ident(bool throw_err)
{
  token *t;
  bool valid = peek_ident(t,throw_err);
  if (valid) swallow();
  return valid;
}

// --- parsing toplevel declarations ---

ebt_file *
parser::parse()
{
  try
    {
      for (;;)
        {
          token *t;
          if (peek_op("probe", t))
#ifdef ONLY_BASIC_PROBES
            f->resolved_probes.push_back(parse_basic_probe_decl());
#else
            f->probes.push_back(parse_probe_decl());
#endif
          else if (peek_op("func", t))
            {
              ebt_function *fn = parse_func_decl();
              f->functions[fn->name] = fn;
            }
          else if (peek_op("global", t))
            {
              ebt_global *gl = parse_global_decl();
              f->globals[gl->name] = gl;
            }
          else if (peek_op("array", t))
            {
              ebt_global *gl = parse_array_decl();
              f->globals[gl->name] = gl;
            }
          else if (finished())
            break;
          else
            throw_expect_error("probe, func or global declaration");
        }
    }
  catch (const parse_error& pe)
    {
      print_error(pe); return NULL;
    }

  return f;
}

#ifdef ONLY_BASIC_PROBES
basic_probe *
parser::parse_basic_probe_decl()
{
  // unsigned probe_start = input.get_pos();

  basic_probe *p = new basic_probe;
  next_op("probe", p->tok, true);

  // Identify the probe point:
  if (swallow_ident("begin",false))
    p->mechanism = EV_BEGIN;
  else if (swallow_ident("end",false))
    p->mechanism = EV_END;
  else if (swallow_ident("insn",false))
    p->mechanism = EV_INSN;
  else if (swallow_ident("function",false))
    {
      swallow_op(".");

      if (swallow_ident("entry",false))
        p->mechanism = EV_FENTRY;
      else if (swallow_ident("exit",false))
        p->mechanism = EV_FEXIT;
      else
        throw_expect_error("entry or exit");
    }
  else
    throw_expect_error("basic probe type "
                       "(begin, end, insn, function.entry, function.exit)");

  // Parse condition chain:
  if (swallow_op("(",false))
    {
      token *t;
      if (!peek_op(")",t))
        while (true)
          {
            condition *c = new condition;
            c->id = p->get_variable_ticket();
            c->e = parse_assignment(); // no comma operator allowed
            p->conditions.push_back(c);
            
            if (!swallow_op(",",false)) break;
          }
      swallow_op(")");
    }

  // unsigned probe_end = input.get_pos();

  p->body = new handler;
  p->body->id = f->get_handler_ticket();
  p->body->action = parse_compound_stmt();
  // XXX Only works for oneliner probes: p->body->orig_source = "probe " + input.source(probe_start, probe_end) + " { ... }";
  p->body->orig_source = "probe " + input.source_line(p->tok) + " ...";

  return p;
}
#endif

probe *
parser::parse_probe_decl()
{
  // unsigned probe_start = input.get_pos();

  probe *p = new probe;
  next_op("probe", p->tok, true);

  p->probe_point = parse_event_expr();

  // unsigned probe_end = input.get_pos();

  p->body = new handler;
  p->body->id = f->get_handler_ticket();
  p->body->action = parse_compound_stmt();
  // XXX Only works for oneliner probes: p->body->orig_source = "probe " + input.source(probe_start, probe_end) + " { ... }";
  p->body->orig_source = "probe " + input.source_line(p->tok) + " ...";

  return p;
}

ebt_function *
parser::parse_func_decl()
{
  ebt_function *fn = new ebt_function;
  next_op("func", fn->tok, true);

  fn->id = f->get_global_ticket();

  // Parse function name:
  next_ident(fn->tok,true);
  fn->name = fn->tok->content;

  // Parse function arguments:
  swallow_op("(");
  token *t;
  if (!peek_op(")",t))
    while (true)
      {
        next_ident(t,true);
        fn->argument_names.push_back(t->content);
        delete t;

        if (!swallow_op(",",false)) break;
      }
  swallow_op(")");

  fn->body = parse_compound_stmt();

  return fn;
}

ebt_global *
parser::parse_global_decl()
{
  swallow_op("global");

  ebt_global *g = new ebt_global;
  g->id = f->get_global_ticket();
  g->array_type = d_scalar;

  // Parse global name:
  next_ident(g->tok,true);
  g->name = g->tok->content;

  // Parse global initializer:
  if (swallow_op("=",false))
    g->initializer = parse_expr();

  return g;
}

ebt_global *
parser::parse_array_decl()
{
  swallow_op("array");

  ebt_global *g = new ebt_global;
  g->id = f->get_global_ticket();
  g->array_type = d_array;

  // Parse global name:
  next_ident(g->tok,true);
  g->name = g->tok->content;

  return g;
}

// --- parsing statements ---

stmt *
parser::parse_stmt()
{
  token *t;
  if (peek_op(";",t))
    {
      empty_stmt *s = new empty_stmt;
      s->tok = next();
      return s;
    }
  else if (peek_op("{",t))
    return parse_compound_stmt();
  else if (peek_op("return",t) || peek_op("break",t)
           || peek_op("continue",t) || peek_op("next",t))
    {
      jump_stmt *s = new jump_stmt;
      s->tok = next();
      if (s->tok->content == "break")
        s->kind = j_break;
      else if (s->tok->content == "continue")
        s->kind = j_continue;
      else if (s->tok->content == "next")
        s->kind = j_next;
      if (s->tok->content == "return")
        { s->kind = j_return; s->value = parse_expr(); }
      return s;
    }
  else if (peek_op("if",t))
    return parse_ifthen_stmt();
  else if (peek_op("while",t))
    return parse_while_loop();
  else if (peek_op("for",t))
    return parse_for_loop();
  else if (peek_op("foreach",t))
    return parse_foreach_loop();
  else
    try {
      t = peek();
      return parse_expr_stmt();
    } catch (const parse_error& pe) {
      // We *only* change errors ocurring at the start of the expression.
      if (t == pe.tok || t == peek())
        throw_expect_error("statement or expression", t);
      else
        throw pe;
    }
}

stmt *
parser::parse_expr_stmt()
{
  expr_stmt *s = new expr_stmt;
  s->e = parse_expr();
  s->tok = s->e->tok;
  return s;
}

stmt *
parser::parse_ifthen_stmt()
{
  ifthen_stmt *s = new ifthen_stmt;

  next_op("if", s->tok, true);

  swallow_op("(");
  s->condition = parse_expr();
  swallow_op(")");

  s->then_stmt = parse_stmt();

  // Dangling 'else' adheres to the closest 'if':
  if (swallow_op("else",false))
    s->else_stmt = parse_stmt();

  return s;
}

stmt *
parser::parse_while_loop()
{
  loop_stmt *s = new loop_stmt;

  next_op("while", s->tok, true);

  swallow_op("(");
  s->condition = parse_expr();
  swallow_op(")");

  s->body = parse_stmt();

  return s;
}

stmt *
parser::parse_for_loop()
{
  loop_stmt *s = new loop_stmt;

  next_op("for", s->tok, true);

  swallow_op("(");
  if (!swallow_op(";",false))
    { s->initial = parse_expr(); swallow_op(";"); }
  if (!swallow_op(";",false))
    { s->condition = parse_expr(); swallow_op(";"); }
  if (!swallow_op(")",false))
    { s->update = parse_expr(); swallow_op(")"); }

  s->body = parse_stmt();

  return s;
}

stmt *
parser::parse_foreach_loop()
{
  foreach_stmt *s = new foreach_stmt;

  next_op("foreach", s->tok, true);

  swallow_op("(");

  token *t;
  next_ident(t, true);
  s->identifier = t->content;
  delete t;

  swallow_op("in");
  s->array = parse_expr();

  swallow_op(")");

  s->body = parse_stmt();

  return s;
}

/* XXX Helper used only for parse_compound_stmt(). */
std::vector<stmt *> *
parser::parse_block(std::vector<stmt *> *stmts)
{
  if (!stmts) stmts = new vector<stmt *>;

  swallow_op("{");
  while (!swallow_op("}",false))
    stmts->push_back(parse_stmt());

  return stmts;
}

stmt *
parser::parse_compound_stmt()
{
  compound_stmt *s = new compound_stmt;
  parse_block(&s->stmts);
  return s;
}

// --- parsing event expressions ---

event_expr *
parser::parse_event_expr()
{
  return parse_binary_event();
}

event_expr *
parser::parse_basic_event()
{
  event_expr *e = NULL;

  token *t;
  next_ident(t,true);

  named_event *start = new named_event;
  start->tok = t;
  start->ident = t->content;
  start->subevent = NULL;
  e = start;

  while (next_op(".",t) || next_op("(",t))
    {
      if (t->content == ".")
        {
          named_event *ne = new named_event;
          next_ident(ne->tok,true);
          ne->ident = ne->tok->content;
          ne->subevent = e;
          e = ne;
        }
      else // t->content == "("
        {
          conditional_event *ce = new conditional_event;
          ce->condition = parse_expr();
          ce->tok = ce->condition->tok;
          ce->subevent = e;
          e = ce;

          swallow_op(")");
        }
      delete t;
    }

  return e;
}

event_expr *
parser::parse_event_not()
{
  token* t;
  if (next_op("not", t))
    {
      compound_event* e = new compound_event;
      e->tok = t;
      e->op = t->content;
      e->subevents.push_back(parse_event_not());
      // -- XXX Recursion may be unnecessary here.
      return e;
    }
  else
    return parse_basic_event();
}

event_expr *
parser::parse_binary_event()
{
  stack<std::pair<event_expr *, token *> > left_frags;
  event_expr *right_frag = parse_event_not();
  token *op_right = NULL;

  for (;;)
    {
      // Try to input a binary operator of suitable precedence:
      op_right = peek();
      if (!op_right || op_right->type != tok_op
          || prec_event(op_right->content) >= PREC_NONE)
        op_right = NULL;
      else
        next();
      
      // Combine pending fragments of lower precedence:
      while (!left_frags.empty())
        {
          event_expr *left_frag = left_frags.top().first;
          token *op_left = left_frags.top().second;

          // All operators are left associative:
          if (op_right
              && prec_event(op_left->content) > prec_event(op_right->content))
            break;

          // Combine the left fragment:
          compound_event *ev = new compound_event;
          ev->tok = op_left;
          ev->op = op_left->content;
          ev->subevents.push_back(left_frag);
          ev->subevents.push_back(right_frag);
          left_frags.pop();
          right_frag = ev;
        }

      if (!op_right && left_frags.empty())
        break;

      // Try to input the expression to the right of the operator:
      left_frags.push(std::make_pair(right_frag, op_right));
      right_frag = parse_event_not();
      op_right = NULL;
    }

  return right_frag;
}

// --- parsing expressions ---

expr *
parser::parse_expr()
{
  return parse_comma();
}

expr *
parser::parse_basic_expr()
{
  token *t = peek();
  if (t == NULL)
    goto not_found;
  if (t != NULL && (t->type == tok_num || t->type == tok_str))
    {
      basic_expr *e = new basic_expr;
      e->tok = next();
      return e;
    }
  if (swallow_op("(",false))
    {
      expr *e = parse_expr();
      swallow_op(")");
      return e;
    }

  // Otherwise, we are dealing with a designator or function call:
  basic_expr *e;
  if (next_op("$",t) || next_op("@",t))
    {
      e = new basic_expr;
      e->sigil = t;
      next_ident(e->tok,true);
      goto designator_chain;
    }
  if (!next_ident(t))
    goto not_found;

  if (swallow_op("(",false))
    {
      call_expr *ce = new call_expr;
      ce->tok = t;
      ce->func = t->content;
      if (!peek_op(")",t))
        while (true)
          {
            ce->args.push_back(parse_assignment()); // no comma operator allowed
            if (!swallow_op(",",false)) break;
          }
      swallow_op(")");
      return ce;
    }

  // Otherwise, parse a designator chain
  e = new basic_expr;
  e->tok = t;

 designator_chain:
  while (next_op(".",t) || next_op("[",t))
    {
      std::pair<chain_type, expr *> chain_item;
      if (t->content == ".")
        {
          chain_item.first = chain_ident;
          basic_expr *id = new basic_expr;
          id->tok = next();
          if (id->tok->type != tok_ident && id->tok->type != tok_num)
            throw_expect_error("identifier or constant index");
          chain_item.second = id;
        }
      else // t->content == "["
        {
          chain_item.first = chain_index;
          chain_item.second = parse_expr();
          swallow_op("]");
        }
      delete t;
      e->chain.push_back(chain_item);
    }

  return e;

 not_found:
  throw_expect_error("expression e.g. designator, function call, or constant");
  // return NULL; // -- XXX May want to suppress a warning.
}

expr *
parser::parse_unary()
{
  token* t;
  if (next_op("+", t) || next_op("-", t)
      || next_op("!", t) || next_op("~", t)
      || next_op("++",t) || next_op("--", t))
    {
      unary_expr* e = new unary_expr;
      e->tok = t;
      e->op = t->content;
      e->operand = parse_unary();
      // -- XXX Recursion may be unnecessary here.
      return e;
    }
  else
    {
      expr *e = parse_basic_expr();
      while (next_op("++",t) || next_op("--",t))
        {
          unary_expr *ue = new unary_expr;
          ue->tok = t;
          ue->op = t->content;
          ue->operand = e;
          e = ue;
        }
      return e;
    }
}

expr *
parser::parse_binary()
{
  stack<std::pair<expr *, token *> > left_frags;
  expr *right_frag = parse_unary();
  token *op_right = NULL;

  for (;;)
    {
      // Try to input a binary operator of suitable precedence:
      op_right = peek();
      if (!op_right || op_right->type != tok_op
          || prec_expr(op_right->content) >= 20)
        op_right = NULL;
      else
        next();
      
      // Combine pending fragments of lower precedence:
      while (!left_frags.empty())
        {
          expr *left_frag = left_frags.top().first;
          token *op_left = left_frags.top().second;

          // All operators are left associative:
          if (op_right
              && prec_expr(op_left->content) > prec_expr(op_right->content))
            break;

          // Combine the left fragment:
          binary_expr *e = new binary_expr;
          e->tok = op_left;
          e->left = left_frag;
          e->op = op_left->content;
          e->right = right_frag;
          left_frags.pop();
          right_frag = e;
        }

      if (!op_right && left_frags.empty())
        break;

      // Try to input the expression to the right of the operator:
      left_frags.push(std::make_pair(right_frag, op_right));
      right_frag = parse_unary();
      op_right = NULL;
    }

  return right_frag;
}

expr *
parser::parse_ternary()
{
  expr *e = parse_binary();

  // The ternary operator is right-associative:
  token* t;
  if (next_op("?",t))
    {
      conditional_expr *ce = new conditional_expr;
      ce->tok = t;
      ce->cond = e;
      ce->truevalue = parse_ternary();
      swallow_op(":");
      ce->falsevalue = parse_ternary();
      // -- XXX Recursion may be unnecessary here.
      e = ce;
    }

  return e;
}

expr *
parser::parse_assignment()
{
  expr *e = parse_ternary();

  // The assignment operators are right-associative:
  token *t;
  if (next_op("=",t)
         || next_op("+=",t) || next_op("-=",t) || next_op("*=",t)
         || next_op("/=",t) || next_op("%=",t)
         || next_op("<<=",t) || next_op(">>=",t)
         || next_op("&=",t) || next_op("^=",t) || next_op("|=",t))
    {
      binary_expr *be = new binary_expr;
      be->tok = t;
      be->left = e;
      be->op = t->content;
      be->right = parse_assignment();
      e = be;
    }

  return e;
}

expr *
parser::parse_comma()
{
  expr *e = parse_assignment();

  // The comma is left-associative:
  token *t;
  while (next_op(",",t))
    {
      binary_expr *be = new binary_expr();
      be->tok = t;
      be->left = e;
      be->op = t->content;
      be->right = parse_assignment();
      e = be;
    }

  return e;
}
