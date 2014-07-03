// language representation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#ifndef SJ_IR_H
#define SJ_IR_H

#include <string>
#include <vector>
#include <map>

/* from parse.h */
struct token;

#define ONLY_BASIC_PROBES

// --- type system ---

struct sj_type {
  sj_type(const std::string &type_name): type_name(type_name) {}
  std::string type_name;
  // TODOXXX equality comparisons, copy constructors, printing, all that stuff
};

extern sj_type type_string;
extern sj_type type_int;

struct sj_array_type: public sj_type {
  sj_array_type(const std::string &type_name): sj_type(type_name) {}
  // TODOXXX
};

// XXX move out of the header
inline sj_type
type_array(const sj_type &base_type)
{
  // XXX: error if base_type is already an array
  return sj_array_type(base_type.type_name);
  // TODOXXX
}

// --- predicate expressions ---

// An expression is one of the following:
// - basic_expr  : NUMBER, STRING, IDENTIFIER
// - unary_expr  : OPERATOR EXPR
// - binary_expr : EXPR OPERATOR EXPR

struct expr {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
};

std::ostream& operator << (std::ostream &o, const expr &e);

struct basic_expr: public expr {
  // TODOXXX more specific subtypes for value
  token *sigil; // must be NULL or tok_op: "$", "@'

  basic_expr() : sigil(NULL) {}

  void print (std::ostream &o) const;
};

struct unary_expr: public expr {
  std::string op;
  expr *operand;
  void print (std::ostream &o) const;
};

struct binary_expr: public expr {
  expr *left;
  std::string op;
  expr *right;
  void print (std::ostream &o) const;
};

struct conditional_expr: public expr {
  expr *cond;
  expr *truevalue;
  expr *falsevalue;
  void print (std::ostream &o) const;
};

// --- event hierarchy ---

// An event is one of the following:
// - named_event       : [EVENT '.'] IDENTIFIER
// - conditional_event : EVENT '(' EXPR ')'
// - compound_event    : 'not' EVENT, EVENT 'and' EVENT, EVENT 'or' EVENT, EVENT '::' EVENT

// TODOXXX printing support for all below

struct event {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
};

std::ostream& operator << (std::ostream &o, const event &e);

struct named_event: public event {
  std::string ident;
  event *subevent; // possibly NULL
  void print (std::ostream &o) const;
};

struct conditional_event: public event {
  event *subevent;
  expr *condition;
  void print (std::ostream &o) const;
};

struct compound_event: public event {
  std::string op;
  std::vector<event *> subevents;
  void print (std::ostream &o) const;
};

// === TODOXXX REWRITE BELOW

// --- script declarations ---

struct condition {}; // TODOXXX

struct handler {}; // TODOXXX

struct probe {
  event *probe_point;
  handler *body;
  void print (std::ostream &o) const;
};

std::ostream& operator << (std::ostream &o, const probe &p);

// === TODOXXX REWRITE ABOVE ===

// --- built-in events and context values ---

// TODOXXX rename this to something intelligible
struct sj_event {
  // TODOXXX
  void add_context(const std::string &path, const sj_type &type);
};

// --- probe representation ---

enum basic_probe_type {
  EV_FCALL,   // -- XXX at entry point inside the function
  EV_FRETURN, // -- XXX at return insn inside the function
  EV_INSN,    // -- at every insn of every basic block
  EV_MALLOC,  // -- XXX after return from malloc() type function
  EV_MACCESS, // -- XXX after each memory access
};

struct basic_probe {
  basic_probe_type mechanism;
  // TODOXXX something to indicate what context information we need to obtain
  // TODOXXX can multiple 'instances' of the same context info conflict?
  std::vector<expr *> conditions; // TODOXXX need separate condition * type
  handler *body;
  // TODOXXX printing functionality
  void print (std::ostream &o) const;
};

std::ostream& operator << (std::ostream &o, const basic_probe &p);

// --- files and modules ---

struct sj_file {
  sj_file ();
  sj_file (const std::string& name);

  std::string name;

#ifdef ONLY_BASIC_PROBES
  std::vector<basic_probe *> probes;
#else
  std::vector<probe *> probes;
#endif
  void print(std::ostream &o) const; // TODOXXX
};

std::ostream& operator << (std::ostream &o, const sj_file &f);

/* from util.h */
struct translator_output;

struct sj_module {
private:
  static std::map<std::string, sj_event *> events;

  std::vector<sj_file *> script_files;
  std::map<basic_probe_type, std::vector<basic_probe *> > basic_probes;

  // Define a new event type:
  void add_event (const std::string &path);

  // Add the event represented by event_expr to basic_probes:
  void resolve_events (event *event_expr);
  // XXX also take into account the handler code!

public:
  sj_module ();

  bool has_contents;
  std::string script_name; // short path or "<command line>"
  std::string script_path; // FILENAME
  std::string script_contents; // -e PROGRAM

  int last_pass;

  void compile();
  void emit_dr_client(translator_output &o) const;
};

#endif // SJ_IR_H
