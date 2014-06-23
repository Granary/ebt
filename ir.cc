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
operator << (std::ostream &o, const sj_file &f)
{
  f.print(o);
  return o;
}

void
sj_file::print (std::ostream &o) const
{
  for (unsigned i = 0; i < probes.size(); i++)
    o << *(probes[i]) << endl;
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
  : last_pass(3)
{
  if (events.empty())
    {
      add_event("insn");
      events["insn"]->add_context("$opcode", type_string);
      events["insn"]->add_context("@op", type_array(type_int)); // TODOXXX add array syntax

      add_event("function");
      events["function"]->add_context("$name", type_string);
      events["function"]->add_context("$module", type_string);
      // TODOXXX support for combined name/module notation as in SystemTap
      // TODOXXX support for function variables

      add_event("function.entry");

      add_event("function.exit");

      add_event("malloc");
      events["malloc"]->add_context("@addr", type_int);

      add_event("access");
      events["access"]->add_context("@addr", type_int);
    }
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
      if (result == NULL) return; // XXX error handling
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
      if (result == NULL) return; // XXX error handling
      script_files.push_back(result);
    }

  // print results of parsing pass
  if (last_pass < 2)
    {
      for (unsigned i = 0; i < script_files.size(); i++)
        cerr << *(script_files[i]);
    }

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

  /* TODOXXX (!!!) go through all script files and resolve_events() to build probe_map */

}

/* XXX no particularly fancy infrastructure for now */
void
sj_module::emit_dr_client(translator_output& o) const
{
  o.newline() << "#include \"dr_api.h\"";
  o.newline();

  o.newline() << "static void event_exit(void);";
  o.newline();

  o.newline() << "DR_EXPORT void";
  o.newline() << "dr_init(client_id_t id)";
  o.newline() << "{";
    o.newline(1) << "/* empty client */";
    o.newline() << "dr_register_exit_event(event_exit);";
  o.newline(-1) << "}";
  o.newline();

  o.newline() << "static void";
  o.newline() << "event_exit(void)";
  o.newline() << "{";
    o.newline(1) << "/* empty client */";
  o.newline(-1) << "}";
  o.newline();
}
