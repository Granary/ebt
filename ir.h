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

// --- predicate expressions ---

// An expression is one of the following:
// - basic_expr  : NUMBER, STRING, IDENTIFIER
// - unary_expr  : OPERATOR EXPR
// - binary_expr : EXPR OPERATOR EXPR

struct expr {
  token *tok;
};

struct basic_expr: public expr {
  // TODOXXX more specific subtypes for value
  token *sigil; // must be NULL or tok_op: "$", "@'

  basic_expr() : sigil(NULL) {}
};

struct unary_expr: public expr {
  std::string op;
  expr *operand;
};

struct binary_expr: public expr {
  expr *left;
  std::string op;
  expr *right;
};

struct conditional_expr: public expr {
  expr *cond;
  expr *truevalue;
  expr *falsevalue;
};

// --- event hierarchy ---

// An event is one of the following:
// - named_event       : [EVENT '.'] IDENTIFIER
// - conditional_event : EVENT '(' EXPR ')'
// - compound_event    : 'not' EVENT, EVENT 'and' EVENT, EVENT 'or' EVENT, EVENT '::' EVENT

struct event {
  token *tok;
};

struct named_event: public event {
  std::string ident;
  event *subevent; // possibly NULL
};

struct conditional_event: public event {
  event *subevent;
  expr *condition;
};

struct compound_event: public event {
  std::string op;
  std::vector<event *> subevents;
};

// --- script declarations ---

// SKETCH OF HOW EVENT RESOLUTION PROCEEDS
// =======================================
//
// TODOXXX (!!!) also consider the "restrict" operation A :: B
//
// Some basic algebraic laws governing events:
// - A and (B or C) = (A and B) or (A and C)
// - (A or B) and C = (A and C) or (B and C)
//
// - A (expr) . ident = A . ident (expr)
// - A (expr1) (expr2) = A . (expr1 && expr2)
//
// - A (expr1) and B (expr2) = (A and B) (expr1 && expr2)
// - A (expr) and B = (A and B) (expr)
// - A and B (expr) = (A and B) (expr)
//
// - (A or B) (expr) = A (expr) or B (expr)
//
// - (A and B) . ident = (A or B) . ident = (not B) . ident = *nonsense*
//
// (A simplification that results in *nonsense* means we signal an error.)
//
// Use these laws to factor events into the following hierarchy:
//
// compound_event("or")
// -> conditional_event
//    -> compound_event("and")
//       -> named_event -> named event -> ...
//
// Then, in each compound_event("and"), figure out the one primitive
// event/instrumentation mechanism to use, signal an error if multiple
// incompatible mechanisms are being and'ed together. Likewise signal
// an error if a compound_event("and") fails to include a simple event.
//
// (All other events resolve into context to import, conditions to
// check -- these are collapsed into the conditional_event.
//
// XXX Be sure that they are correctly ordered relative to any
// explicit conditions in the expression. The basic rule should be
// that conditions -- both implicit and explicit -- are evaluated in
// "left to right" and "inside to outside" order.)
//
// Once we've performed the simplification, converting the result to a
// basic_event is simple. (XXX Of course, this is the conceptual-level
// explanation; in practice, we may directly compute the basic_event
// without bothering to do the refactoring on an algebraic level.)

enum basic_probe_type {
  EV_FCALL,   // -- XXX at entry point inside the function
  EV_FRETURN, // -- XXX at return insn inside the function
  EV_INSN,    // -- at every insn of every basic block
  EV_MALLOC,  // -- XXX after return from MEV_MACCESS
};

struct condition {}; // TODOXXX

struct handler {}; // TODOXXX

struct probe {
  event *probe_point;
  handler *body;
};

struct basic_probe {
  basic_probe_type mechanism;
  // TODOXXX something to indicate what context information we need to obtain
  // TODOXXX can multiple 'instances' of the same context info conflict?
  std::vector<condition *> conditions;
  handler *body;
};

struct sj_file {
  sj_file ();
  sj_file (const std::string& name);

  std::string name;

  std::vector<probe *> probes;
};


/* from util.h */
struct translator_output;

struct sj_module {
private:
  std::vector<sj_file *> script_files;

  std::map<basic_probe_type, std::vector<basic_probe *> > basic_probes;

  // Add the event represented by event_expr to basic_probes:
  void resolve_events (event *event_expr);
  // XXX also take into account the handler code!

public:

  bool has_contents;
  std::string script_name; // short path or "<command line>"
  std::string script_path; // FILENAME
  std::string script_contents; // -e PROGRAM

  int last_pass;

  sj_module() : last_pass(3) {}

  void compile();
  void emit_dr_client(translator_output &o) const;
};

#endif // SJ_IR_H
