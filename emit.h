// backend generation
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#ifndef EBT_EMIT_H
#define EBT_EMIT_H

#define PROBE_COUNTERS

#include <vector>
#include <map>
#include <set>

/* from util.h */
struct translator_output;

#include "ir.h"
/* provides ebt_module, handlers, basic_probe_type, basic_probe, expr, global_data */

typedef std::map<basic_probe_type, std::vector<basic_probe *> > probe_map;

// --- generic C "unparser", to be used for both DR and Granary tasks ---

// TODOXXX start stupidly simple with the context mapping
typedef std::map<std::string, std::string> ebt_context;

class c_unparser {
 public:
  c_unparser();
  void emit_expr(translator_output& o, expr *e, ebt_context *ctx) const;
};

// --- client templates ---

class client_template {
  ebt_module *module;

public:
  client_template(ebt_module *module) : module(module) {}
  virtual void register_probe(basic_probe *bp) = 0;
  virtual void register_global_data(global_data *g) = 0;
  virtual void emit(translator_output& o) = 0;
};

class fake_client_template: public client_template {
  probe_map basic_probes;
  std::vector<global_data *> globals;

public:
  fake_client_template(ebt_module *module) : client_template(module) {}
  void register_probe(basic_probe *bp);
  void register_global_data(global_data *g);
  void emit(translator_output& o);
};

class dr_client_template: public client_template {
  probe_map basic_probes;
  std::vector<global_data *> globals;

  std::vector<handler *> all_handlers; // -- used to iterate through handlers
  std::set<unsigned> seen_handlers; // -- used to avoid duplicates in all_handlers

  c_unparser unparser;
  static ebt_context ctxvars; // TODOXXX initialize

  bool have(basic_probe_type mechanism) const;

  /* standard names for various generated variables */
  std::string global(unsigned id) const;
  std::string global_mutex(unsigned id) const;

#ifdef PROBE_COUNTERS
  std::string probecounter(unsigned id) const;
  std::string probecount_mutex(unsigned id) const;
#endif

  std::string chain_label(unsigned id) const;
  std::string condvar(unsigned hid, int cid) const;
  std::string handlerfn(unsigned id) const;

  void emit_condition_chain(translator_output& o,
                            std::vector<basic_probe *> chain,
                            bool handle_inline = false);
  void emit_handler(translator_output& o, handler *h);

  void emit_begin_callback(translator_output& o);
  void emit_exit_callback(translator_output& o);
  void emit_bb_callback(translator_output& o);

  void emit_function_decls(translator_output& o, bool forward = false);
  void emit_global_data(translator_output &o);

public:
  dr_client_template(ebt_module *module);
  void register_probe(basic_probe *bp);
  void register_global_data(global_data *g);
  void emit(translator_output& o);
};

#endif // EBT_EMIT_H
