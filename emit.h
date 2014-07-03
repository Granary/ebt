// backend generation
// Copyright (C) 2014 Serguei Makarov
//
// This file is part of SJ, an experimental event-based
// instrumentation language employing DBT systems DynamoRIO and
// Granary as its backend, and inspired by SystemTap. SJ is currently
// internal-use code written as part of Serguei Makarov's 2014 USRA at
// UofT, not for distribution.

#ifndef SJ_EMIT_H
#define SJ_EMIT_H

#include <vector>
#include <map>

/* from util.h */
struct translator_output;

#include "ir.h"

// --- client templates ---

class client_template {
  sj_module *module;

public:
  client_template(sj_module *module) : module(module) {}
  virtual void register_probe(basic_probe *bp) = 0;
  virtual void emit(translator_output& o) const = 0;
};

class fake_client_template: public client_template {
  std::map<basic_probe_type, std::vector<basic_probe *> > basic_probes;

public:
  fake_client_template(sj_module *module) : client_template(module) {}
  void register_probe(basic_probe *bp);
  void emit(translator_output& o) const;
};

class dr_client_template: public client_template {
public:
  dr_client_template(sj_module *module) : client_template(module) {}
  void register_probe(basic_probe *bp);
  void emit(translator_output& o) const;
};

#endif // SJ_EMIT_H
