// backend generation
// Copyright (C) 2014-2015 Serguei Makarov
//
// This file is part of EBT, and is free software. You can
// redistribute it and/or modify it under the terms of the GNU General
// Public License (GPL); either version 2, or (at your option) any
// later version.

#include <iostream>
#include <sstream>

#include <vector>
#include <set>
#include <map>

using namespace std;

#include "emit.h"
#include "parse.h"
#include "ir.h"
#include "util.h"

// -----------------------------------
// --- methods for client_template ---
// -----------------------------------

client_template::client_template(ebt_module *_module) : module(_module)
{
  for (vector<ebt_file *>::iterator it = module->script_files.begin();
       it != module->script_files.end(); it++)
    {
      ebt_file *script = *it;
      if (!script->is_compiled) continue; // -- skip unused libraries.

      // Collect basic probes:
      for (vector<basic_probe *>::iterator it = script->resolved_probes.begin();
           it != script->resolved_probes.end(); it++)
        {
          basic_probe *bp = *it;
          register_probe(bp);
        }

      // Collect functions:
      for (map<string, ebt_function *>::iterator it = script->functions.begin();
             it != script->functions.end(); it++)
        {
          ebt_function *f = it->second;
          register_function(f);
        }

      // Collect global data:
      for (map<string, ebt_global *>::iterator it = script->globals.begin();
          it != script->globals.end(); it++)
        {
          ebt_global *g = it->second;
          register_global(g);
        }
    }

    // Builtin functions from the script_module are either declared in
    // runtime headers or hardcoded into the client template. Thus,
    // they do not need to be registered with the user-defined functions
    // to be emitted.
}

void
client_template::register_probe(basic_probe *bp)
{
  basic_probes[bp->mechanism].push_back(bp);
}

void
client_template::register_function(ebt_function *f)
{
  functions.push_back(f);
}

void
client_template::register_global(ebt_global *g)
{
  globals.push_back(g);
}

// --------------------------------------
// --- methods for dr_client_template ---
// --------------------------------------

dr_client_template::dr_client_template(ebt_module *_module)
  : client_template(_module)
{
  // XXX Some options are easiest to hardcode in at the moment:
  wants_opcode = true;         // -- XXX hardcoded in every script
  wants_forward = false;       // -- computed at the start of emit()
  wants_bb_callback = false;   // -- computed at the start of emit()
  wants_exit_callback = false; // -- computed at the start of emit()
}

// TODOXXX override register_basic_probe to collect wants_drsyms or similar

// Determine if initialization boilerplate corresponding to a given
// probe mechanism should be generated:
bool
dr_client_template::wants_mechanism(basic_probe_type bt)
{
  // XXX Since implementations of mechanisms may depend on each other
  // (e.g. EV_FCALL is based on EV_INSN) this may require more complex testing.
  return basic_probes[bt].size() != 0;
}

void
dr_client_template::emit(translator_output& o)
{
  // Determine which elements of the client template should be used:
  wants_forward = functions.size() > 0 || basic_probes.size() > 0
    || wants_mechanism(EV_INSN) || wants_mechanism(EV_END);
  wants_bb_callback = wants_mechanism(EV_INSN);
  wants_exit_callback = wants_mechanism(EV_END);

  // Emit library includes:
  o.newline() << "#include \"dr_api.h\"";
  if (wants_opcode)
    o.newline() << "#include \"runtime/opcode.h\"";
  o.newline();

  // Emit forward declarations:
  if (wants_forward)
  {
    o.newline() << "// forward decls";
    emit_functions(o, true);
    emit_probe_handlers(o, true);
    emit_basic_block_callback(o, true);
    emit_exit_callback(o, true);
    o.newline();
  }
  
  // Emit global value and function declarations:
  emit_globals(o);
  emit_functions(o);

  // Emit initialization and invocations of EV_BEGIN handlers:
  o.newline() << "DR_EXPORT void";
  o.newline() << "dr_init(client_id_t id)";
  o.newline() << "{";

  o.indent(1);

  // TODOXXX may also want to initialize drsyms and similar
  // TODOXXX may also want to initialize probe context structs?

  /* Register callbacks: */
  if (wants_bb_callback)
    o.newline() << "dr_register_bb_event(bb_event);";
  if (wants_exit_callback)
    o.newline() << "dr_register_exit_event(exit_event);";

  /* Initialize globals: */
  for (unsigned i = 0; i < globals.size(); i++)
    emit_global_initialization(o, globals[i]);

  /* Fire EV_BEGIN probes: */
  emit_event_invocations(o, EV_BEGIN);

  o.newline(-1) << "}";
  o.newline();

  // Emit probe handlers:
  emit_probe_handlers(o);

  // Emit DBT callbacks:
  emit_basic_block_callback(o);
  emit_exit_callback(o);
}

// --- groups of declarations ---

void
dr_client_template::emit_globals (translator_output& o)
{
  // TODOXXX
}

void
dr_client_template::emit_functions (translator_output& o, bool forward)
{
  // TODOXXX
}

void
dr_client_template::emit_probe_handlers (translator_output& o, bool forward)
{
  // TODOXXX
}

void
dr_client_template::emit_basic_block_callback (translator_output& o, bool forward)
{
  if (!wants_bb_callback) return;

  o.newline() << "static dr_emit_flags_t";
  o.line() << " bb_event(void *drcontext, void *tag, instrlist_t *bb,";
  o.line() << " bool for_trace, bool translating)";
  if (forward)
    {
      o.line() << ";";
      return;
    }

  o.newline() << "{";
  o.indent(1);

  o.newline() << "instr_t *instr, *next_instr;";
  o.newline();

  o.newline() << "for (instr = instrlist_first_app(bb); instr != NULL; instr = next_instr) {";
  o.newline(1) << "next_instr = instr_get_next_app(instr);";

  emit_event_instrumentation (o, EV_INSN);

  o.newline(-1) << "}";

  o.newline(-1) << "}";
  o.newline();
}

void
dr_client_template::emit_exit_callback (translator_output& o, bool forward)
{
  if (!wants_exit_callback) return;

  o.newline() << "static void exit_event(void)";
  if (forward)
    {
      o.line() << ";";
      return;
    }

  o.newline() << "{";
  o.indent(1);

  /* Fire EV_END probes: */
  emit_event_invocations(o, EV_END);

  o.newline(-1) << "}";
  o.newline();
}

// --- helpers to emit specific boilerplate ---

void
dr_client_template::emit_global_initialization (translator_output& o, ebt_global *g)
{
  // TODOXXX
}

void
dr_client_template::emit_event_invocations (translator_output& o, basic_probe_type bt)
{
  // TODOXXX
  if (!wants_mechanism(bt)) return;

  o.newline() << "// TODOXXX invocations for " << bt << " go here";
}

void
dr_client_template::emit_event_instrumentation (translator_output& o, basic_probe_type bt)
{
  // TODOXXX
  if (!wants_mechanism(bt)) return;

  o.newline() << "// TODOXXX instrumentation for " << bt << " goes here";
}

// TODOXXX old version of dr_client_template::emit()
//   o.newline() << "#include \"dr_api.h\"";
//   o.newline() << "#include \"runtime/opcode.h\""; // TODOXXX
//   o.newline();
//
//   /* TODOXXX context data structure types for handlers */
//
//   o.newline() << "// forward declarations for callbacks";
//   emit_function_decls(o, true);
//
//   o.newline() << "// global data -- probe counters, variables, mutexes";
//   emit_global_data(o);
//
//   o.newline() << "// callbacks";
//   emit_function_decls(o);
// }

// TODOXXX

// ------------------------------
// --- methods for c_unparser ---
// ------------------------------

// TODOXXX

// ----------------------------------------
// --- methods for fake_client_template ---
// ----------------------------------------

void
fake_client_template::emit(translator_output& to)
{
  ostream &o = to.line();

  o << endl;
  o << "RESULTING BASIC PROBES (by mechanism)" << endl;
  for (probe_map::const_iterator it = basic_probes.begin();
       it != basic_probes.end(); it++)
    {
      const vector<basic_probe *> &group = it->second;
      if (group.empty()) continue;

      if (it != basic_probes.begin())
        o << "---" << endl;
      for (unsigned i = 0; i < group.size(); i++)
        o << *(group[i]) << endl;
    }
  o << "===" << endl;
  if (!functions.empty())
    {
      o << "RESULTING FUNCTIONS" << endl;
      for (unsigned i = 0; i < functions.size(); i++)
        o << *(functions[i]) << endl;
      o << "===" << endl;
    }
  if (!globals.empty())
    {
      o << "RESULTING GLOBAL DATA" << endl;
      for (unsigned i = 0; i < functions.size(); i++)
        o << *(globals[i]) << endl;
      o << "===" << endl;
    }
}

// ==========================================================================
// ==========================================================================
// ==========================================================================
// ==========================================================================
// TODOXXX REWRITE BELOW

// // === general functions for emitting C code ===
//
// /* a basic traversing visitor to emit the expression */
// class unparsing_visitor : public visitor {
//   translator_output *o;
//   ebt_context *ctx;
//
// public:
//   unparsing_visitor(translator_output *o, ebt_context *ctx) : o(o), ctx(ctx) {}
//   void visit_basic_expr (basic_expr *s);
//   void visit_unary_expr (unary_expr *s);
//   void visit_binary_expr (binary_expr *s);
//   void visit_conditional_expr (conditional_expr *s);
// };
//
// void
// unparsing_visitor::visit_basic_expr (basic_expr *s)
// {
//   // TODOXXX error handling, escaping intricacies
//   if (s->tok->type == tok_ident)
//     o->line() << "(" << (*ctx)[s->tok->content] << ")";
//   else if (s->tok->type == tok_str)
//     o->line() << "\"" << s->tok->content << "\"";
//   else if (s->tok->type == tok_num)
//     o->line() << s->tok->content;
// }
//
// void
// unparsing_visitor::visit_unary_expr (unary_expr *s)
// {
//   o->line() << s->op << "(";
//   s->operand->visit(this);
//   o->line() << ")";
// }
//
// void
// unparsing_visitor::visit_binary_expr (binary_expr *s)
// {
//   o->line() << "(";
//   s->left->visit(this);
//   o->line() << ")" << s->op << "(";
//   s->right->visit(this);
//   o->line() << ")";
// }
//
// void
// unparsing_visitor::visit_conditional_expr (conditional_expr *s)
// {
//   o->line() << "(";
//   s->cond->visit(this);
//   o->line() << ")" << "?" << "(";
//   s->truevalue->visit(this);
//   o->line() << ")" << ":" << "(";
//   s->falsevalue->visit(this);
//   o->line() << ")";
// }
//
// /* we create instances of the c_unparser to make it possible for each
//    client_template to (down the line) configure some of its own options */
// c_unparser::c_unparser() {}
//
// void
// c_unparser::emit_expr(translator_output &o, expr *e, ebt_context *ctx) const
// {
//   unparsing_visitor v(&o, ctx);
//   e->visit(&v);
// }
//
// // === DynamoRIO backend ===
//
// // TODOXXX class var_collector : public traversing visitor {
// //   set<string> required_contextvars; // TODOXXX
// //
// // public:
// //   void visit_basic_expr (basic_expr *s);
// // };
//
// ebt_context dr_client_template::ctxvars;
//
// dr_client_template::dr_client_template(ebt_module *module)
//   : client_template(module)
// {
//   if (ctxvars.empty())
//     {
//       // TODOXXX quick-and-dirty populate this with ctxvars
//       // TODOXXX obviously we need a more sophisticated namespacing thing
//       ctxvars["opcode"] = "opcode_string(opcode)"; // TODOXXX runtime
//       ctxvars["fname"] = "TODOXXX_get_fname";
//     }
// }
//
// // --- registering data ---
//
// // HOW THE PIECES COME TOGETHER
// //
// // The client template is a set of boilerplate code where certain
// // features are enabled and disabled depending on the
// // presence of probes making use of a corresponding mechanism.
// // For instance, EV_INSN requires a bb_event callback which
// // iterates through the instructions in each basic block.
//
// // TODOXXX current batch of things to implement: EV_BEGIN, EV_END, EV_INSN
//
// // TODOXXX implement EV_FCALL
// // TODOXXX implement EV_FRETURN
// // TODOXXX implement EV_MALLOC
// // TODOXXX implement EV_MACCESS
//
// // TODOXXX implement EV_END on interrupt
//
// void
// dr_client_template::register_probe(basic_probe *bp)
// {
//   basic_probes[bp->mechanism].push_back(bp);
//
//   /* add handler to handlers */
//   unsigned id = bp->body->id;
//   if (seen_handlers.count(id) == 0)
//     {
//       seen_handlers.insert(id);
//       all_handlers.push_back(bp->body);
//     }
// }
//
// // Check if there are probes needing a certain mechanism. This could
// // be complicated as some mechanisms depend among each other,
// // e.g. EV_FCALL builds on the per-instruction instrumentation
// // provided by EV_INSN.
// bool
// dr_client_template::have(basic_probe_type m) const
// {
//   return basic_probes.count(m) != 0;
// }
//
// // --- naming conventions ---
//
// // TODOXXX need c++0x to_string or such
// string
// tostring(unsigned id)
// {
//   ostringstream result; result << id; return result.str();
// }
//
// string
// dr_client_template::global(unsigned id) const
// {
//   return "global_" + tostring(id);
// }
//
// string
// dr_client_template::global_mutex(unsigned id) const
// {
//   return "global_" + tostring(id) + "_mutex";
// }
//
// #ifdef PROBE_COUNTERS
// string
// dr_client_template::probecounter(unsigned id) const
// {
//   return "probecounter_" + tostring(id);
// }
//
// string
// dr_client_template::probecount_mutex(unsigned id) const
// {
//   return "probecounter_" + tostring(id) + "_mutex";
// }
// #endif
//
// string
// dr_client_template::chain_label(unsigned id) const
// {
//   return "chain_label_" + tostring(id);
// }
//
// string
// dr_client_template::condvar(unsigned hid, int cid) const
// {
//   return "_handler_" + tostring(hid) + "_cond_" + tostring(cid);
// }
//
// string
// dr_client_template::handlerfn(unsigned id) const
// {
//   return "ebt_handler_" + tostring(id);
// }
//
// // --- client template ---
//
// void
// dr_client_template::emit_condition_chain(translator_output &o,
//                                          vector<basic_probe *> chain,
//                                          bool handle_inline)
// {
//   /* STEP 2 -- implement the condition chain thing */
//
//   /* TODOXXX initialize conditions/context */
//   for (vector<basic_probe *>::const_iterator it = chain.begin();
//        it != chain.end(); it++)
//     {
//       basic_probe *bp = *it;
//
//       /* obtain name of next label in the chain */
//       string next_label = it+1 != chain.end() ?
//         chain_label(bp->body->id) : "chain_finished";
//
//       for (vector<condition *>::const_iterator jt = bp->conditions.begin();
//            jt != bp->conditions.end(); jt++)
//         {
//           condition *c = *jt;
//
//           /* TODOXXX ensure necessary context data for conditions */
//
//           // TODOXXX check actual conditions
//
//           o.newline() << "int " << condvar(bp->body->id, c->id)
//                       << " = ";
//           unparser.emit_expr(o, c->content, &ctxvars);
//           o.line() << ";";
//
//           o.newline() << "if (!" << condvar(bp->body->id, c->id) << ") "
//                       << "goto " << next_label << ";";
//         }
//
//       /* TODOXXX enable handler for bp -- perhaps inline */
//       if (handle_inline)
//         o.newline() << handlerfn(bp->body->id) << "();";
//       else
//         /* TODOXXX need various hints at current instrumentation point ? */
//         o.newline() << "dr_insert_clean_call(drcontext, bb, instr, (void *)"
//                     << handlerfn(bp->body->id) << ", false /* no fp save */, 0);";
//       /* TODOXXX need to attach context info -- use a struct for this */
//
//       o.newline();
//       o.newline() << next_label << ":";
//     }
//
//   o.newline() << ";";
// }
//
// void
// dr_client_template::emit_handler(translator_output &o, handler *h)
// {
//   /* TODOXXX mutex usage should be configurable by a command line option */
//   /* TODOXXX OPTIONAL grab mutexes for required variables */
// #ifdef PROBE_COUNTERS
//   o.newline() << "dr_mutex_lock(" << probecount_mutex(h->id) << ");";
//   o.newline() << probecounter(h->id) << "++;";
// #endif
//   /* TODOXXX handler contents */
//   /* TODOXXX OPTIONAL release mutexes for required variables */
// #ifdef PROBE_COUNTERS
//   o.newline() << "dr_mutex_unlock(" << probecount_mutex(h->id) << ");";
// #endif
// }
//
// void
// dr_client_template::emit_begin_callback(translator_output &o)
// {
//   /* initialize global data */
// #ifdef PROBE_COUNTERS
//   for (vector<handler *>::const_iterator it = all_handlers.begin();
//        it != all_handlers.end(); it++)
//     {
//       handler *h = *it;
//       unsigned id = h->id;
//
//       o.newline() << probecounter(id) << " = 0;";
//       o.newline() << probecount_mutex(id) << " = dr_mutex_create();";
//     }
// #endif
//   for (vector<global_data *>::const_iterator it = globals.begin();
//        it != globals.end(); it++)
//     {
//       global_data *g = *it;
//       unsigned id = g->id;
//
//       /* TODOXXX initialize global data content */
//       o.newline() << "static void *" << global_mutex(id) << ";";
//     }
//
//   /* handlers for EV_BEGIN */
//   emit_condition_chain(o, basic_probes[EV_BEGIN], true);
//
//   /* register callbacks */
// #ifdef PROBE_COUNTERS
//   o.newline() << "dr_register_exit_event(event_exit);";
// #else
//   if (have(EV_END)) o.newline() << "dr_register_exit_event(event_exit);";
// #endif
//   if (have(EV_INSN)) o.newline() << "dr_register_bb_event(bb_event);";
// }
//
// void
// dr_client_template::emit_exit_callback(translator_output &o)
// {
//   /* handlers for EV_END */
//   emit_condition_chain(o, basic_probes[EV_END], true);
//
// #ifdef PROBE_COUNTERS
//   /* print a summary of probe counters */
//   for (vector<handler *>::const_iterator it = all_handlers.begin();
//        it != all_handlers.end(); it++)
//     {
//       handler *h = *it;
//       unsigned id = h->id;
//
//       o.newline() << "dr_fprintf(STDERR, \"%5d :: "
//                   << c_stringify(h->title())
//                   << "\\n\", " << probecounter(id) << ");";
//       // XXX <-- do we need to lock a mutex here?
//     }
// #endif
// }
//
// void
// dr_client_template::emit_bb_callback(translator_output &o)
// {
//   o.newline() << "instr_t *instr, *next_instr;";
//   o.newline() << "int opcode;";
//   o.newline();
//
//   o.newline() << "for (instr = instrlist_first(bb); instr != NULL; instr = next_instr) {";
//   o.newline(1) << "/* handle per-instruction events */";
//   o.newline() << "next_instr = instr_get_next(instr);";
//   o.newline() << "opcode = instr_get_opcode(instr);"; // TODOXXX enable conditionally
//
//   /* handlers for EV_INSN */
//   emit_condition_chain(o, basic_probes[EV_INSN]);
//   /* TODOXXX will need to interleave EV_FCALL, EV_FRETURN events */
//
//   o.newline(-1) << "}";
// }
//
// void
// dr_client_template::emit_function_decls(translator_output &o, bool forward)
// {
//   if (!forward)
//     {
//       o.newline() << "DR_EXPORT void";
//       o.newline() << "dr_init(client_id_t id)";
//       o.newline() << "{";
//       o.newline(1) << "/* initialize client */";
//
//       emit_begin_callback(o);
//
//       o.newline(-1) << "}";
//       o.newline();
//     }
//
//   for (vector<handler *>::const_iterator it = all_handlers.begin();
//        it != all_handlers.end(); it++)
//     {
//       handler *h = *it;
//       unsigned id = h->id;
//
//       o.newline() << "static void ";
//       (forward ? o.line() : o.newline()) << handlerfn(id) << "()"; // TODOXXX context struct
//       if (forward)
//         o.line() << ";";
//       else
//         {
//           o.newline() << "{";
//           o.newline(1) << "/* " << c_comment(h->title()) << " */";
//
//           emit_handler(o, h);
//
//           o.newline(-1) << "}";
//           o.newline();
//         }
//     }
//
// #ifdef PROBE_COUNTERS
//   o.newline() << "static void ";
//   (forward ? o.line() : o.newline()) << "event_exit(void)";
//   if (forward)
//     o.line() << ";";
//   else
//     {
//       o.newline() << "{";
//       o.newline(1) << "/* terminate client */";
//
//       emit_exit_callback(o);
//
//       o.newline(-1) << "}";
//       o.newline();
//     }
// #else
//   if (have(EV_END))
//     {
//       o.newline() << "static void ";
//       (forward ? o.line() : o.newline()) << "event_exit(void)";
//     }
//   if (have(EV_END) && forward)
//     o.line() << ";";
//   else if (have(EV_END))
//     {
//       o.newline() << "{";
//       o.newline(1) << "/* terminate client */";
//
//       emit_exit_callback(o);
//
//       o.newline(-1) << "}";
//       o.newline();
//     }
// #endif
//
//   if (have(EV_INSN))
//     {
//       o.newline() << "static dr_emit_flags_t";
//       o.newline() << "bb_event(void *drcontext, void *tag, instrlist_t *bb, "
//                   << "bool for_trace, bool translating)";
//     }
//   if (have(EV_INSN) && forward)
//     o.line() << ";";
//   else if (have(EV_INSN))
//     {
//       o.newline() << "{";
//       o.newline(1) << "/* instrument basic block */";
//
//       emit_bb_callback(o);
//
//       o.newline(-1) << "}";
//       o.newline();
//     }
//
//   o.newline();
// }
//
// void
// dr_client_template::emit_global_data(translator_output &o)
// {
// #ifdef PROBE_COUNTERS
//   for (vector<handler *>::const_iterator it = all_handlers.begin();
//        it != all_handlers.end(); it++)
//     {
//       handler *h = *it;
//       unsigned id = h->id;
//
//       o.newline() << "unsigned " << probecounter(id) << ";";
//       o.newline() << "static void *" << probecount_mutex(id) << ";";
//     }
// #endif
//
//   for (vector<global_data *>::const_iterator it = globals.begin();
//        it != globals.end(); it++)
//     {
//       // TODOXXX iterate through global data; define variables & mutexes
//     }
//
//   o.newline();
// }
//
// void
// dr_client_template::emit(translator_output& o)
// {
//   o.newline() << "#include \"dr_api.h\"";
//   o.newline() << "#include \"runtime/opcode.h\""; // TODOXXX
//   o.newline();
//
//   /* TODOXXX context data structure types for handlers */
//
//   o.newline() << "// forward declarations for callbacks";
//   emit_function_decls(o, true);
//
//   o.newline() << "// global data -- probe counters, variables, mutexes";
//   emit_global_data(o);
//
//   o.newline() << "// callbacks";
//   emit_function_decls(o);
// }
