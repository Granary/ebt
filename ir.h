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
#include <utility>

/* from parse.h */
struct token;

#define ONLY_BASIC_PROBES

// -------------------
// --- type system ---
// -------------------

enum ebt_type {t_unknown, t_int, t_str, t_void};
enum ebt_dimension {d_scalar, d_array}; // TODOXXX -- add t_shadow

// Used to distinguish static '$' and dynamic '@' context values.
enum ebt_lifetime {l_none, l_static, l_dynamic};

// --------------------------------------
// --- scripting language: statements ---
// --------------------------------------

// A statement is one of the following:
// - empty_stmt    : ;
// - expr_stmt     : EXPR
// - compound_stmt : STMT ; STMT ; ...
// - ifthen_stmt   : if (EXPR) STMT [else STMT]
// - loop_stmt     : for (EXPR; EXPR; EXPR) STMT
//                 : while(EXPR) STMT
// - foreach_stmt  : for (IDENTIFIER in ARRAY) STMT
// - jump_stmt     : return [EXPR] / break / continue / next

// TODOXXX STUB OUT AND IMPLEMENT BELOW

struct visitor;
struct stmt {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
  virtual void visit (visitor* u) = 0;
};

std::ostream& operator << (std::ostream &o, const stmt &st);

struct empty_stmt : public stmt {
  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct expr;
struct expr_stmt : public stmt {
  expr *e;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct compound_stmt : public stmt {
  std::vector<stmt *> stmts;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct ifthen_stmt : public stmt {
  expr *condition;
  stmt *then_stmt;
  stmt *else_stmt; // -- optional

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct loop_stmt : public stmt {
  expr *initial; // -- optional
  expr *condition; // -- optional, treat as infinite loop when missing
  expr *update; // -- optional
  stmt *body;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct foreach_stmt : public stmt {
  std::string identifier;
  expr *array;
  stmt *body;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

// TODOXXX rename 'next' elsewhere
enum jump_type {j_return, j_break, j_continue, j_next};

// TODOXXX rename elsewhere
struct jump_stmt : public stmt {
  jump_type kind;
  expr *value; // -- optional, for return statement

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

// TODOXXX -- also add break, continue, pass statement types

// ---------------------------------------------
// --- scripting language: value expressions ---
// ---------------------------------------------

// An expression is one of the following:
// - basic_expr       : NUMBER, STRING, IDENTIFIER
// - unary_expr       : OPERATOR EXPR
// - binary_expr      : EXPR OPERATOR EXPR
// - conditional_expr : EXPR ? EXPR : EXPR
// - call_expr        : IDENTIFIER ( EXPR, EXPR, ... )

struct expr {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
  virtual void visit (visitor* u) = 0;
};

std::ostream& operator << (std::ostream &o, const expr &e);

enum chain_type { chain_ident, chain_index };

struct basic_expr: public expr {
  token *sigil; // -- must be either NULL or tok_op: "$", "@"
  std::vector<std::pair<chain_type, expr *> > chain;

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

struct call_expr: public expr {
  std::string func;
  std::vector<expr *> args;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

// ---------------------------------------------
// --- scripting language: event expressions ---
// ---------------------------------------------

// An event is one of the following:
// - named_event       : [EVENT '.'] IDENTIFIER
// - conditional_event : EVENT '(' EXPR ')'
// - compound_event    : 'not' EVENT, EVENT 'and' EVENT, EVENT 'or' EVENT, EVENT '::' EVENT

struct event_expr {
  token *tok;
  virtual void print (std::ostream &o) const = 0;
};

std::ostream& operator << (std::ostream &o, const event_expr &e);

struct named_event: public event_expr {
  std::string ident;
  event_expr *subevent; // possibly NULL

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct conditional_event: public event_expr {
  event_expr *subevent;
  expr *condition;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

struct compound_event: public event_expr {
  std::string op;
  std::vector<event_expr *> subevents;

  void print (std::ostream &o) const;
  void visit (visitor *u);
};

// -----------------------------------------------
// --- AST visitors: used for various analyses ---
// -----------------------------------------------

struct visitor {
  virtual void visit_empty_stmt (empty_stmt *s) = 0;
  virtual void visit_expr_stmt (expr_stmt *s) = 0;
  virtual void visit_compound_stmt (compound_stmt *s) = 0;
  virtual void visit_ifthen_stmt (ifthen_stmt *s) = 0;
  virtual void visit_loop_stmt (loop_stmt *s) = 0;
  virtual void visit_foreach_stmt (foreach_stmt *s) = 0;
  virtual void visit_jump_stmt (jump_stmt *s) = 0; // TODOXXX distinguish kinds?

  virtual void visit_basic_expr (basic_expr *s) = 0;
  virtual void visit_unary_expr (unary_expr *s) = 0;
  virtual void visit_binary_expr (binary_expr *s) = 0;
  virtual void visit_conditional_expr (conditional_expr *s) = 0;
  virtual void visit_call_expr (call_expr *s) = 0;

  virtual void visit_named_event (named_event *e) = 0;
  virtual void visit_conditional_event (conditional_event *e) = 0;
  virtual void visit_compound_event (compound_event *e) = 0; // TODOXXX distinguish kinds??
};

// TODOXXX basic traversing visitor -- iterates the leaves of an expression
struct traversing_visitor : public visitor {
  void visit_basic_expr (basic_expr *s);
  void visit_unary_expr (unary_expr *s);
  void visit_binary_expr (binary_expr *s);
  void visit_conditional_expr (conditional_expr *s);
  void visit_call_expr (call_expr *s);
};

// TODOXXX variable collecting visitor -- finds used context values

// --------------------------------
// --- diagnostic functionality ---
// --------------------------------

struct ebt_printable {
  virtual void print (std::ostream &o) const = 0; // TODOXXX stub implementation
};

std::ostream& operator << (std::ostream &o, const ebt_printable &pr);
std::ostream& operator << (std::ostream &o, const ebt_printable *pr);

// ---------------------------------------------------------------
// --- declarations: globals, functions, and (built-in) events ---
// ---------------------------------------------------------------

struct ebt_value : ebt_printable {
  std::string name;
  ebt_type value_type;
  ebt_dimension array_type;
  ebt_type key_type; // -- only used when array_type = d_array
};

struct ebt_global : ebt_value {
  // Used for codegen; set when this global is first added to an ebt_file.
  unsigned id;

  expr *initializer;

  token *tok;
  void print(std::ostream &o) const;
};

struct ebt_function : ebt_printable {
  // Used for codegen; set when this function is first added to an ebt_file.
  unsigned id;

  ebt_function() : is_builtin(false) {}
  ebt_function(std::string name) : name(name), is_builtin(false) {}

  std::string name;
  std::vector<std::string> argument_names;
  stmt *body;
  bool is_builtin; // -- might be handled specially in emit & typecheck phases

  std::vector<ebt_type *> argument_types;
  ebt_type return_type;

  token *tok;
  void print(std::ostream &o) const;
};

struct ebt_context : ebt_value {
  ebt_lifetime lifetime;

  ebt_context()
    { array_type = d_scalar; }

  ebt_context(ebt_lifetime l, ebt_type t, ebt_dimension d = d_scalar)
    { lifetime = l; value_type = t; array_type = d; key_type = t_unknown; }

  void print(std::ostream &o) const;
};

enum basic_probe_type {
  EV_NONE,    // -- used for an ebt_event that does not provide a mechanism

  EV_BEGIN,   // -- at the beginning of script execution
  EV_END,     // -- just before the script terminates

  EV_INSN,    // -- at every insn of every basic block

  EV_FENTRY,  // -- TODOXXX at the entry point to a function
  EV_FEXIT,   // -- TODOXXX just before a function returns

  // TODOXXX additional mechanisms: object.alloc, object.access
  // XXX remember to update operator << for new basic_probe_types
};

std::ostream& operator << (std::ostream &o, basic_probe_type bt);

struct ebt_event : ebt_printable {
  ebt_event() {}
  ebt_event(std::string name, ebt_event *parent = NULL)
    : name(name), parent(parent) {}

  std::string name;
  ebt_event *parent;
  basic_probe_type mechanism;
  std::map<std::string, ebt_context *> context;
  std::map<std::string, ebt_event *> subevents;

  std::string full_name() {
    if (parent != NULL)
      return parent->full_name() + " " + name;
    return name;
  }

  void print(std::ostream &o) const; // TODOXXX
};

// ----------------------------
// --- probe representation ---
// ----------------------------

struct condition;
struct handler;
struct probe : ebt_printable {
  event_expr *probe_point;
  handler *body;

  token *tok;
  void print(std::ostream &o) const;
};

// TODOXXX
// Represents both conditions to check and context data to compute:
struct condition : ebt_printable {
  // Used for codegen; set when this condition is first added to a basic_probe.
  // Note that multiple basic_probes may use the result of a condition.
  unsigned id;

  // TODOXXX this field is only used for condition values
  expr *e;

  void print(std::ostream &o) const;
};

struct handler : ebt_printable {
  // Used for codegen; set when this handler is first added to an ebt_file.
  // Note that multiple basic_probes may invoke the same handler.
  unsigned id;

  stmt *action;

  // XXX The handler may be identified by an explanatory string in some cases:
  std::string orig_source;
  const std::string title() const { return orig_source; }

  void print(std::ostream &o) const;
};

// Script probes are resolved to basic probes which use a single
// basic_probe_type and a linear chain of context values and conditions:
struct basic_probe : ebt_printable {
private:
  // Used to name subelements (e.g. context vals) with a simple numerical id:
  unsigned variable_ticket;

public:
  basic_probe() : variable_ticket(0) {}
  unsigned get_variable_ticket() { return variable_ticket++; }

  // Information describing the probe:
  basic_probe_type mechanism;
  std::vector<condition *> conditions;
  handler *body;

  token *tok;
  void print(std::ostream &o) const;
};

// -------------------------
// --- files and modules ---
// -------------------------

struct ebt_file;

struct ebt_module : ebt_printable {
private:
  // Information describing builtin EBT events, context data, and functions:
  static std::map<std::string, ebt_event *> builtin_events;
  static std::map<std::string, ebt_function *> builtin_functions;

  // Used to name top-level elements with a simple numerical id:
  unsigned handler_ticket; // -- for probe handlers
  unsigned global_ticket;  // -- for both globals and functions

public:
  ebt_module ();
  unsigned get_handler_ticket() { return handler_ticket++; }
  unsigned get_global_ticket() { return global_ticket++; }

  // Information describing the script...
  std::vector<ebt_file *> script_files;

  bool has_contents;
  std::string script_name; // short path or "<command line>"
  std::string script_path; // from 'FILENAME'
  std::string script_contents; // from '-e PROGRAM'

  int last_pass; // which pass to stop compilation after (default 4:run)

  int compile();
  void print(std::ostream &o) const;
};

struct ebt_file : ebt_printable {
  // XXX ebt_file ();
  ebt_file (const std::string& name, ebt_module *parent);

  ebt_module *parent;
  unsigned get_handler_ticket() { return parent->get_handler_ticket(); }
  unsigned get_global_ticket() { return parent->get_global_ticket(); }

  // Information describing the file:
  std::string name;
  std::vector<probe *> probes;
  std::map<std::string, ebt_function *> functions;
  std::map<std::string, ebt_global *> globals;

  // ... this gets translated by compile() into:
  std::vector<basic_probe *> resolved_probes;

  // Libraries are always loaded but only compiled and emitted on demand.
  // Thus we need to track whether a given ebt_file is actually used:
  bool is_compiled;

  void compile();
  void print(std::ostream &o) const;
};

#endif // EBT_IR_H
