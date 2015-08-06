// backend generation
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef EBT_EMIT_H
#define EBT_EMIT_H

// Diagnostic option. After terminating, the script prints a summary of
// how many times each probe handler was invoked.
#define PROBE_COUNTERS

#include <vector>
#include <map>
#include <set>

/* from util.h */
struct translator_output;

#include "ir.h"

// ----------------------------
// --- DBT client templates ---
// ----------------------------

// The client template organizes basic probes by mechanism:
typedef std::map<basic_probe_type, std::vector<basic_probe *> > probe_map;

class client_template {
protected:
  ebt_module *module;
  probe_map basic_probes;
  std::vector<ebt_function *> functions;
  std::vector<ebt_global *> globals;

public:
  client_template(ebt_module *module);
  virtual void register_probe(basic_probe *bp);
  virtual void register_function(ebt_function *f);
  virtual void register_global(ebt_global *g);
  virtual void emit(translator_output& o) = 0;
};

// Prettyprints the compiled basic probes for diagnostic purposes:
class fake_client_template: public client_template {
public:
  fake_client_template(ebt_module *module) : client_template(module) {}
  void emit(translator_output& o);
};

// Emits a client written in C for the DynamoRIO framework:
class dr_client_template: public client_template {
  // Optional features enabled when a probe requires them:
  bool wants_opcode;        // -- #include "runtime/opcodes.h"
  bool wants_forward;       // -- // forward declarations
  bool wants_bb_callback;   // -- dr_register_exit_event(exit_event);
  bool wants_exit_callback; // -- dr_register_bb_event(bb_event);
  bool wants_mechanism(basic_probe_type bt);

  // Standard names for various generated variables:
  // TODOXXX

  // Groups of declarations:
  void emit_globals (translator_output& o);
  void emit_functions (translator_output& o, bool forward = false);
  void emit_probe_handlers (translator_output& o, bool forward = false);
  void emit_basic_block_callback (translator_output& o, bool forward = false);
  void emit_exit_callback (translator_output& o, bool forward = false);
  // -- some components have the option to emit forward declarations.

  // Helpers to emit specific boilerplate:
  void emit_global_initialization (translator_output& o, ebt_global *g);
  void emit_event_invocations (translator_output& o, basic_probe_type bt);
  void emit_event_instrumentation (translator_output& o, basic_probe_type bt);
  // Client code can either invoke a handler directly,
  // or ask DR to instrument target code with a clean call.

public:
  dr_client_template(ebt_module *module);
  void emit(translator_output& o);
};

// TODOXXX REWRITE AND STUB OUT BELOW

// TODOXXX start stupidly simple with the context mapping
//typedef std::map<std::string, std::string> ebt_context;

// class dr_client_template: public client_template {
//   std::vector<handler *> all_handlers; // -- used to iterate through handlers
//   std::set<unsigned> seen_handlers; // -- used to avoid duplicates in all_handlers
//
//   c_unparser unparser;
//   static ebt_context ctxvars; // TODOXXX initialize
//
//   bool have(basic_probe_type mechanism) const;
//
//   /* standard names for various generated variables */
//   std::string global(unsigned id) const;
//   std::string global_mutex(unsigned id) const;
//
// #ifdef PROBE_COUNTERS
//   std::string probecounter(unsigned id) const;
//   std::string probecount_mutex(unsigned id) const;
// #endif
//
//   std::string chain_label(unsigned id) const;
//   std::string condvar(unsigned hid, int cid) const;
//   std::string handlerfn(unsigned id) const;
//
//   void emit_condition_chain(translator_output& o,
//                             std::vector<basic_probe *> chain,
//                             bool handle_inline = false);
//   void emit_handler(translator_output& o, handler *h);
//
//   void emit_begin_callback(translator_output& o);
//   void emit_exit_callback(translator_output& o);
//   void emit_bb_callback(translator_output& o);
//
//   void emit_function_decls(translator_output& o, bool forward = false);
//   void emit_global_data(translator_output &o);

// public:
//  dr_client_template(ebt_module *module) : client_template(module) {}
//  void register_probe(basic_probe *bp);
//  void emit(translator_output& o);
// };

// ---------------------------------------------------------------
// --- generic C "unparser", used for any C/C++ output formats ---
// ---------------------------------------------------------------

// TODOXXX
// class c_unparser {
//  public:
//   c_unparser();
//   void emit_expr(translator_output& o, expr *e, ebt_context *ctx) const;
// };

#endif // EBT_EMIT_H
