// language representation
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef EBT_IR_H
#define EBT_IR_H

#include <string>
#include <vector>
#include <map>

/* from parse.h */
struct token;

#define ONLY_BASIC_PROBES

// --- type system ---

struct ebt_type {
  ebt_type(const std::string &type_name): type_name(type_name) {}
  std::string type_name;
  // TODOXXX equality comparisons, copy constructors, printing, all that stuff
};

extern ebt_type type_string;
extern ebt_type type_int;

struct ebt_array_type: public ebt_type {
  ebt_array_type(const std::string &type_name): ebt_type(type_name) {}
  // TODOXXX
};

// XXX move out of the header
inline ebt_type
type_array(const ebt_type &base_type)
{
  // XXX: error if base_type is already an array
  return ebt_array_type(base_type.type_name);
  // TODOXXX
}

// --- predicate expressions ---

// An expression is one of the following:
// - basic_expr  : NUMBER, STRING, IDENTIFIER
// - unary_expr  : OPERATOR EXPR
// - binary_expr : EXPR OPERATOR EXPR

struct visitor;
struct expr {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
  virtual void visit (visitor* u) = 0;
};

std::ostream& operator << (std::ostream &o, const expr &e);

struct basic_expr: public expr {
  // TODOXXX more specific subtypes for value
  token *sigil; // must be NULL or tok_op: "$", "@'

  basic_expr() : sigil(NULL) {}

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct unary_expr: public expr {
  std::string op;
  expr *operand;
  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct binary_expr: public expr {
  expr *left;
  std::string op;
  expr *right;
  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct conditional_expr: public expr {
  expr *cond;
  expr *truevalue;
  expr *falsevalue;
  void print (std::ostream &o) const;
  void visit (visitor *u);
};

// --- event hierarchy ---

// An event is one of the following:
// - named_event       : [EVENT '.'] IDENTIFIER
// - conditional_event : EVENT '(' EXPR ')'
// - compound_event    : 'not' EVENT, EVENT 'and' EVENT, EVENT 'or' EVENT, EVENT '::' EVENT

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

// --- visitors ---

struct visitor {
  virtual void visit_basic_expr (basic_expr *s) = 0;
  virtual void visit_unary_expr (unary_expr *s) = 0;
  virtual void visit_binary_expr (binary_expr *s) = 0;
  virtual void visit_conditional_expr (conditional_expr *s) = 0;
};

// basic traversing visitor -- iterates the leaves of an expression
struct traversing_visitor : public visitor {
  void visit_basic_expr (basic_expr *s);
  void visit_unary_expr (unary_expr *s);
  void visit_binary_expr (binary_expr *s);
  void visit_conditional_expr (conditional_expr *s);
};

// TODOXXX variable collecting visitor for the context computations

// --- script declarations ---

struct condition {
  // TODOXXX something to indicate what context information we need to obtain
  // TODOXXX can multiple 'instances' of the same context info conflict?
  unsigned id; // used in code generator
  expr *content;
};

struct handler {
  unsigned id;
  std::string orig_source; // XXX explanatory string shown in some contexts
  const std::string title() const { return "probe" + orig_source; } // TODOXXX probe should be included in orig_source
  // TODOXXX represent handler content
};

struct probe {
  event *probe_point;
  handler *body;
  void print (std::ostream &o) const;
};

std::ostream& operator << (std::ostream &o, const probe &p);

struct global_data {
  unsigned id; // TODOXXX set id when adding to ebt_file
  void print (std::ostream &o) const;
};

// --- built-in events and context values ---

// TODOXXX rename this to something intelligible?
struct ebt_event {
  // TODOXXX
  void add_context(const std::string &path, const ebt_type &type);
};

// --- probe representation ---

enum basic_probe_type {
  EV_NONE,    // TODOXXX special case to be used in event resolution

  EV_FCALL,   // -- XXX at entry point inside the function
  EV_FRETURN, // -- XXX at return insn inside the function
  EV_INSN,    // -- at every insn of every basic block
  EV_MALLOC,  // -- XXX after return from malloc() type function
  EV_MACCESS, // -- XXX after each memory access

  EV_BEGIN,   // -- at the beginning of script execution
  EV_END,     // -- just before the script terminates
};

struct basic_probe {
private:
  unsigned ticket;
public:
  basic_probe() : ticket(0) {}
  unsigned get_ticket() { return ticket++; }

  basic_probe_type mechanism;
  std::vector<condition *> conditions;
  handler *body;

  void print (std::ostream &o) const;
};

std::ostream& operator << (std::ostream &o, const basic_probe &p);

// --- files and modules ---

struct ebt_file {
  ebt_file ();
  ebt_file (const std::string& name);

  std::string name;

  std::vector<probe *> probes;
  std::vector<basic_probe *> resolved_probes;
  void print(std::ostream &o) const; // TODOXXX
};

std::ostream& operator << (std::ostream &o, const ebt_file &f);

/* from util.h */
struct translator_output;

/* from emit.h */
struct client_template;

struct ebt_module {
private:
  static std::map<std::string, ebt_event *> events;

  std::vector<ebt_file *> script_files;

  // Used to distinguish top-level script elements with a unique numerical id:
  unsigned handler_ticket;
  unsigned global_ticket;

  // Define a new event type:
  void add_event (const std::string &path);

  // Add the event represented by event_expr to basic_probes:
  void resolve_events (event *event_expr);
  // XXX also take into account the handler code!

  void prepare_client_template (client_template &t);

public:
  ebt_module ();

  unsigned get_handler_ticket() { return handler_ticket++; }
  unsigned get_global_ticket() { return global_ticket++; }

  bool has_contents;
  std::string script_name; // short path or "<command line>"
  std::string script_path; // FILENAME
  std::string script_contents; // -e PROGRAM

  int last_pass;

  void compile();
  void emit_fake_client(translator_output &o);
  void emit_dr_client(translator_output &o);
};

#endif // EBT_IR_H
