// language representation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "ir.h"
#include "parse.h"
#include "emit.h"

using namespace std;

// --- basic types ---

sj_type type_string("string");
sj_type type_int("foo");

// --- pretty-print predicate expressions ---

// TODOXXX add precedence a linebreaking feature of some kind?

std::ostream&
operator << (std::ostream &o, const expr &e)
{
  e.print(o);
  return o;
}

void
basic_expr::print (std::ostream &o) const
{
  if (sigil)
    o << sigil->content;
  o << tok->content;
}

void
unary_expr::print (std::ostream &o) const
{
  o << op << " (";
  operand->print(o);
  o << ")";
}

void
binary_expr::print (std::ostream &o) const
{
  o << "(";
  left->print(o);
  o << ") " << op << " (";
  right->print(o);
  o << ")";
}

void
conditional_expr::print (std::ostream &o) const
{
  o << "(";
  cond->print(o);
  o << ") ? (";
  truevalue->print(o);
  o << ") : (";
  falsevalue->print(o);
  o << ")";
}

// --- pretty-print event expressions ---

std::ostream&
operator << (std::ostream &o, const event &e)
{
  e.print(o);
  return o;
}

void
named_event::print (std::ostream &o) const
{
  if (subevent) {
    o << "(";
    subevent->print(o);
    o << ").";
  }
  o << ident;
}

void
conditional_event::print (std::ostream &o) const
{
  o << "(";
  subevent->print(o);
  o << ") (";
  condition->print(o);
  o << ")";
}

void
compound_event::print (std::ostream &o) const
{
  if (op == "not")
    {
      o << op << " ";
      subevents[0]->print(o);
      return;
    }

  o << "(";
  subevents[0]->print(o);
  for (unsigned i = 1; i < subevents.size(); i++) {
    o << ") " << op << " (";
    subevents[i]->print(o);
  }
  o << ")";
}

// --- pretty-print overall file ---

std::ostream&
operator << (std::ostream &o, const probe &p)
{
  p.print(o);
  return o;
}

void
probe::print (std::ostream &o) const
{
  // XXX also print body
  o << "probe " << *probe_point << " {}";
}

std::ostream&
operator << (std::ostream &o, const basic_probe &p)
{
  p.print(o);
  return o;
}

void
basic_probe::print (std::ostream &o) const
{
  o << "probe{" << body->id << "} ";
  switch (mechanism) {
  case EV_NONE: o << "EV_NONE"; break;
  case EV_FCALL: o << "EV_FCALL"; break;
  case EV_FRETURN: o << "EV_FRETURN"; break;
  case EV_INSN: o << "EV_INSN"; break;
  case EV_MALLOC: o << "EV_MALLOC"; break;
  case EV_MACCESS: o << "EV_MACCESS"; break;
  case EV_END: o << "EV_END"; break;
  case EV_BEGIN: o << "EV_BEGIN"; break;
  }
  o << " ";
  if (!conditions.empty()) o << "(";
  if (!conditions.empty()) o << *(conditions[0]->content);
  for (unsigned i = 1; i < conditions.size(); i++)
    o << ", " << *(conditions[i]->content);
  if (!conditions.empty()) o << ")";
}

std::ostream&
operator << (std::ostream &o, const sj_file &f)
{
  f.print(o);
  return o;
}

void
global_data::print (std::ostream &o) const
{
  o << "global{" << id << "}";
  // TODOXXX additional info on the global
}

void
sj_file::print (std::ostream &o) const
{
#ifdef ONLY_BASIC_PROBES
  for (unsigned i = 0; i < resolved_probes.size(); i++)
    o << *(resolved_probes[i]) << endl;
#else
  for (unsigned i = 0; i < probes.size(); i++)
    o << *(probes[i]) << endl;
#endif
}

// --- methods for the visitor pattern ---

void
basic_expr::visit (visitor *u)
{
  u->visit_basic_expr(this);
}

void
unary_expr::visit (visitor *u)
{
  u->visit_unary_expr(this);
}

void
binary_expr::visit (visitor *u)
{
  u->visit_binary_expr(this);
}

void
conditional_expr::visit (visitor *u)
{
  u->visit_conditional_expr(this);
}

// --- methods for traversing_visitor ---

void
traversing_visitor::visit_basic_expr (basic_expr *s)
{
  // nothing to do here
}

void
traversing_visitor::visit_unary_expr (unary_expr *s)
{
  s->operand->visit(this);
}

void
traversing_visitor::visit_binary_expr (binary_expr *s)
{
  s->left->visit(this);
  s->right->visit(this);
}

void
traversing_visitor::visit_conditional_expr (conditional_expr *s)
{
  s->cond->visit(this);
  s->truevalue->visit(this);
  s->falsevalue->visit(this);
}

// --- methods for sj_event ---

void
sj_event::add_context(const string &path, const sj_type &type)
{
  // TODOXXX
}

// --- methods for sj_file ---

sj_file::sj_file() {}
sj_file::sj_file(const string& name) : name(name) {}

// --- methods for sj_module ---

map<string, sj_event *> sj_module::events;

sj_module::sj_module()
  : handler_ticket(0), global_ticket(0), last_pass(3)
    // TODOXXX bump last_pass to 4 when run_client stuff is empty
{
  // TODOXXX populate context hierarchy using something like this:
  // if (events.empty())
  //   {
  //     add_event("insn");
  //     events["insn"]->add_context("$opcode", type_string);
  //     events["insn"]->add_context("@op", type_array(type_int)); // TODOXXX add array syntax

  //     add_event("function");
  //     events["function"]->add_context("$name", type_string);
  //     events["function"]->add_context("$module", type_string);
  //     // TODOXXX support for combined name/module notation as in SystemTap
  //     // TODOXXX support for function variables

  //     add_event("function.entry");

  //     add_event("function.exit");

  //     add_event("malloc");
  //     events["malloc"]->add_context("@addr", type_int);

  //     add_event("access");
  //     events["access"]->add_context("@addr", type_int);
  //   }
}

void
sj_module::add_event (const string &path)
{
  // TODOXXX
}

void
sj_module::compile()
{
  sj_file *f;
  if (has_contents)
    {
      istringstream input(script_contents);

      if (last_pass < 1)
        {
          test_lexer(this, input);
          return;
        }

      sj_file *result = parse(this, input);
      if (result == NULL) { exit(1); } // TODOXXX error handling
      script_files.push_back(result);
    }
  else
    {
      ifstream input(script_path.c_str());
      if (!input.is_open()) { perror("cannot open script file"); exit(1); } // TODOXXX: exit() doesn't really fit here -- use an rc

      if (last_pass < 1)
        {
          test_lexer(this, input);
          return;
        }

      sj_file *result = parse(this, input);
      if (result == NULL) { exit(1); } // TODOXXX error handling
      script_files.push_back(result);
    }

  // print results of parsing pass
  if (last_pass < 3)
    {
      cerr << "RESULTING INITIAL PROBES" << endl;
      for (unsigned i = 0; i < script_files.size(); i++)
        cerr << *(script_files[i]);
      cerr << "===" << endl;
      return;
    }

#ifndef ONLY_BASIC_PROBES
  // TODOXXX SKETCH OF HOW EVENT RESOLUTION PROCEEDS
  // ===============================================
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

  // TODOXXX
#endif
}

void
sj_module::prepare_client_template(client_template &t)
{
  for (vector<sj_file *>::iterator it = script_files.begin();
       it != script_files.end(); it++) {
    for (vector<basic_probe *>::iterator jt = (*it)->resolved_probes.begin();
         jt != (*it)->resolved_probes.end(); jt++) {
      basic_probe *bp = *jt;
      t.register_probe(bp);
    }
  }
}

void
sj_module::emit_fake_client(translator_output& o)
{
  fake_client_template t(this);
  prepare_client_template(t);
  t.emit(o);
}

void
sj_module::emit_dr_client(translator_output& o)
{
  dr_client_template t(this);
  prepare_client_template(t);
  t.emit(o);
}
