// language representation
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

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

// -------------------------
// --- files and modules ---
// -------------------------

// --- TODOXXX NECESSARY/LOCATION?? visitor for event-resolution --

class resolving_visitor : public visitor {
  probe *orig_probe;

public:
  resolving_visitor(probe *pr) : orig_probe(pr) {}

  vector<basic_probe *> resolved_probes;

  void visit_named_event (named_event *c);
  void visit_conditional_event (conditional_event *c);
  void visit_compound_event (compound_event *c);
};

void
resolving_visitor::visit_named_event (named_event *c)
{
  // TODOXXX
}

void
resolving_visitor::visit_conditional_event (conditional_event *c)
{
  // TODOXXX
}

void
resolving_visitor::visit_compound_event (compound_event *c)
{
  // TODOXXX
}

// --- methods for ebt_module ---

map<string, ebt_event *> ebt_module::builtin_events;
map<string, ebt_function *> ebt_module::builtin_functions;

ebt_module::ebt_module()
  : handler_ticket(0), global_ticket(0), last_pass(4)
{
  // Initialize builtin_events and corresponding context values:
  if (builtin_events.empty())
  {
    ebt_event *e_begin = new ebt_event("begin"); // TODOXXX
    e_begin->mechanism = EV_BEGIN;
    builtin_events["begin"] = e_begin;

    ebt_event *e_end = new ebt_event("end");
    e_end->mechanism = EV_END;
    builtin_events["end"] = e_end;

    ebt_event *e_insn = new ebt_event("insn");
    e_insn->mechanism = EV_INSN;
    e_insn->context["opcode"] = new ebt_context(l_static, t_str);
    e_insn->context["op"] = new ebt_context(l_dynamic, t_int, d_array);
    e_insn->context["op"]->key_type = t_int;
    builtin_events["insn"] = e_insn;

    ebt_event *e_function = new ebt_event("function");
    e_function->mechanism = EV_NONE;
    e_function->context["name"] = new ebt_context(l_static, t_str);
    builtin_events["function"] = e_function;

    ebt_event *e_fentry = new ebt_event("entry", e_function);
    e_fentry->mechanism = EV_FENTRY; // TODOXXX
    e_function->subevents["entry"] = e_fentry;

    ebt_event *e_fexit = new ebt_event("exit", e_function);
    e_fexit->mechanism = EV_FEXIT; // TODOXXX
    e_function->subevents["exit"] = e_fexit;

    // TODOXXX add watchpoint events: obj, obj.alloc, obj.access
  }

  // Initialize builtin_functions:
  if (builtin_functions.empty())
  {
    ebt_function *f_printf = new ebt_function("printf");
    // printf() is variadic, so we can't hardcode argument_types
    f_printf->return_type = t_void;
    f_printf->is_builtin = true;
    builtin_functions["printf"] = f_printf;
  }
}

int
ebt_module::compile()
{
  istream *input; string orig_path;
  if (has_contents)
    {
      input = new istringstream(script_contents);
    }
  else
    {
      ifstream *ifs = new ifstream(script_path.c_str());
      input = ifs;

      if (!ifs->is_open()) {
        string mesg("cannot open script file '" + script_path + "'");
        perror(mesg.c_str());
        return 1;
      }
    }

  /* For testing the lexer pass: */
  if (last_pass < 1)
    {
      test_lexer(this, *input);
      return 0; // TODOXXX test_lexer should return an rc
    }

  /* Parse the script file. (TODOXXX: Also parse and load any libraries.) */
  ebt_file *result = parse(this, *input, script_name);

  if (result == NULL)
    return 1;

  script_files.push_back(result);

  /* For testing the parsing pass: */
  if (last_pass < 3)
    {
      cerr << "RESULTING SCRIPT FILES" << endl << endl;
      for (unsigned i = 0; i < script_files.size(); i++)
        cerr << script_files[i];
      return 0;
    }

  /* Only the first script_file is guaranteed to be needed.
     For any other ones (i.e. libraries), compile() is invoked on demand. */
  script_files[0]->compile();
  return 0;
}

// --- methods for ebt_file ---

// XXX ebt_file::ebt_file() : is_compiled(false) {}
ebt_file::ebt_file(const string& name, ebt_module *parent)
  : parent(parent), name(name), is_compiled(false) {}

struct resolving_visitor;

void
ebt_file::compile()
{
#ifndef ONLY_BASIC_PROBES
  // TODOXXXX Perform event resolution on the current file.

  // TODOXXX very approximate outline
  //for (vector<ebt_file *>::iterator it = script_files.begin();
  //     it != script_files.end(); it++) {
  //  ebt_file * script = *it;
  for (vector<probe *>::iterator it = probes.begin();
       it != probes.end(); it++) {
    probe * pr = *it;

    // Generate all of the basic_probes corresponding to this probe's event:
    // resolving_visitor v(pr); // TODOXXX too virtual
    // pr->probe_point->visit(v);
    // resolved_probes.insert(resolved_probes.end(),
    //                        resolved_probes.begin(), resolved_probes.end());
  }
  //}
  // TODOXXX check if any events were unused and report back with an error

#endif

  is_compiled = true;
}

// ----------------------------
// --- probe representation ---
// ----------------------------

// XXX

// ---------------------------------------------------------------
// --- declarations: globals, functions, and (built-in) events ---
// ---------------------------------------------------------------

// XXX

// --------------------------------------
// --- scripting language: statements ---
// --------------------------------------

// XXX

// --- statement visitors ---

void
empty_stmt::visit (visitor *u)
{
  u->visit_empty_stmt(this);
}

void
expr_stmt::visit (visitor *u)
{
  u->visit_expr_stmt(this);
}

void
compound_stmt::visit (visitor *u)
{
  u->visit_compound_stmt(this);
}

void
ifthen_stmt::visit (visitor *u)
{
  u->visit_ifthen_stmt(this);
}

void
loop_stmt::visit (visitor *u)
{
  u->visit_loop_stmt(this);
}

void
foreach_stmt::visit (visitor *u)
{
  u->visit_foreach_stmt(this);
}

void
jump_stmt::visit (visitor *u)
{
  u->visit_jump_stmt(this);
}

// ---------------------------------------------
// --- scripting language: value expressions ---
// ---------------------------------------------

// XXX

// --- expression visitors ---

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

void
call_expr::visit (visitor *u)
{
  u->visit_call_expr(this);
}

// ---------------------------------------------
// --- scripting language: event expressions ---
// ---------------------------------------------

// XXX

void
named_event::visit (visitor *u)
{
  u->visit_named_event(this);
}

void
conditional_event::visit (visitor *u)
{
  u->visit_conditional_event(this);
}

void
compound_event::visit (visitor *u)
{
  u->visit_compound_event(this);
}

// -----------------------------------------------
// --- AST visitors: used for various analyses ---
// -----------------------------------------------

// --- TODOXXX methods for traversing_visitor ---

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

void
traversing_visitor::visit_call_expr (call_expr *e)
{
  for (vector<expr *>::iterator it = e->args.begin();
       it != e->args.end(); it++)
  {
    (*it)->visit(this);
  }
}

// --------------------------------
// --- diagnostic functionality ---
// --------------------------------

// TODOXXX The pretty-printing sorely needs precedence (removal of unnecessary
// parentheses) as well as a linebreaking feature of some sort.

// --- statements ---

ostream&
operator << (ostream &o, const stmt &st)
{
  st.print(o);
  return o;
}

void
empty_stmt::print (ostream &o) const
{
  // TODOXXX
}

void
expr_stmt::print (ostream &o) const
{
  e->print(o);
}

void
compound_stmt::print (ostream &o) const
{
  // TODOXXX
}

void
ifthen_stmt::print (ostream &o) const
{
  // TODOXXX
}

void
loop_stmt::print (ostream &o) const
{
  // TODOXXX
}

void
foreach_stmt::print (ostream &o) const
{
  // TODOXXX
}

void
jump_stmt::print (ostream &o) const
{
  // TODOXXX
}

// --- value expressions ---

ostream&
operator << (ostream &o, const expr &e)
{
  e.print(o);
  return o;
}

void
basic_expr::print (ostream &o) const
{
  if (sigil)
    o << sigil->content;
  if (tok->type == tok_str)
    o << "\"" << c_stringify(tok->content) << "\"";
  else
    o << tok->content;
}

void
unary_expr::print (ostream &o) const
{
  o << op << " (";
  operand->print(o);
  o << ")";
}

void
binary_expr::print (ostream &o) const
{
  o << "(";
  left->print(o);
  o << ") " << op << " (";
  right->print(o);
  o << ")";
}

void
conditional_expr::print (ostream &o) const
{
  o << "(";
  cond->print(o);
  o << ") ? (";
  truevalue->print(o);
  o << ") : (";
  falsevalue->print(o);
  o << ")";
}

void
call_expr::print (ostream &o) const
{
  o << func << "(";
  for (vector<expr *>::const_iterator it = args.begin();
       it != args.end(); it++)
    {
      if (it != args.begin()) o << ", ";
      (*it)->print(o);
    }
  o << ")";
}

// --- event expressions ---

ostream&
operator << (ostream &o, const event_expr &e)
{
  e.print(o);
  return o;
}

void
named_event::print (ostream &o) const
{
  if (subevent) {
    o << "(";
    subevent->print(o);
    o << ").";
  }
  o << ident;
}

void
conditional_event::print (ostream &o) const
{
  o << "(";
  subevent->print(o);
  o << ") (";
  condition->print(o);
  o << ")";
}

void
compound_event::print (ostream &o) const
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

// --- script modules and declarations ---

ostream&
operator << (ostream &o, const ebt_printable &pr)
{
  pr.print(o);
  return o;
}

// TODOXXX MAKE SURE THIS IS TAKEN ADVANTAGE OF
ostream&
operator << (ostream &o, const ebt_printable *pr)
{
  pr->print(o);
  return o;
}

void
ebt_global::print (ostream &o) const
{
  o << "global{" << id << "}";
  // TODOXXX
}

void
ebt_function::print (ostream &o) const
{
  // TODOXXX
}

void
ebt_context::print (ostream &o) const
{
  // TODOXXX
}

void
ebt_event::print (ostream &o) const
{
  // TODOXXX
}

void
probe::print (ostream &o) const
{
  // TODOXXX
  // TODOXXX also print body
  o << "probe " << *probe_point << " {}";
}

void
condition::print (ostream &o) const
{
  // TODOXXX
}

void
handler::print (ostream &o) const
{
  // TODOXXX
}

ostream&
operator << (ostream &o, basic_probe_type bt)
{
  switch (bt) {
  case EV_NONE: o << "(unknown)"; break;
  case EV_BEGIN: o << "EV_BEGIN"; break;
  case EV_END: o << "EV_END"; break;
  case EV_INSN: o << "EV_INSN"; break;
  case EV_FENTRY: o << "EV_FENTRY"; break;
  case EV_FEXIT: o << "EV_FEXIT"; break;
  default: o << "(BUG: unknown basic probe mechanism)";
  }
  return o;
}

void
basic_probe::print (ostream &o) const
{
  o << "probe{" << body->id << "} ";
  o << mechanism;
  o << " ";
  if (!conditions.empty()) o << "(";
  if (!conditions.empty()) o << *(conditions[0]->e);
  for (unsigned i = 1; i < conditions.size(); i++)
    o << ", " << *(conditions[i]->e);
  if (!conditions.empty()) o << ")";
  if (body) o << body;
}

void
ebt_file::print (ostream &o) const
{
  o << "SCRIPT FILE " << name;
  if (!is_compiled) o << " (not compiled)";
  o << endl;
  // XXX print the path where the script file originated from

#ifndef ONLY_BASIC_PROBES
  if (!probes.empty())
    {
      o << "Original probes:" << endl; 
      for (unsigned i = 0; i < probes.size(); i++)
        o << "* " << *(probes[i]) << endl;
    }
#endif
#ifdef ONLY_BASIC_PROBES
  if (!resolved_probes.empty())
#else
  if (is_compiled && !resolved_probes.empty())
#endif
    {
      o << "Basic probes:" << endl;
      for (unsigned i = 0; i < resolved_probes.size(); i++)
        o << "* " << *(resolved_probes[i]) << endl;
    }
  if (!functions.empty())
    {
      o << "Functions:" << endl;
      for (map<string, ebt_function *>::const_iterator it = functions.begin();
           it != functions.end(); it++)
        o << "* " << *(it->second) << endl;
    }
  if (!globals.empty())
    {
      o << "Globals:" << endl;
      for (map<string, ebt_global *>::const_iterator it = globals.begin();
           it != globals.end(); it++)
        o << "* " << *(it->second) << endl;
    }
  o << endl;
}

void
ebt_module::print (ostream &o) const
{
  // XXX Perhaps print all EBT builtin declarations as well.

  // Print all script files in the current session, separated by blank lines:
  o << "EBT MODULE " << script_name << endl << endl;
  for (unsigned i = 0; i < script_files.size(); i++)
    o << *(script_files[i]) << endl;
}
