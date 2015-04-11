// insn_div.c :: count the number of DIV instructions executed by the target

#include "dr_api.h"

// forward decls
static void handle_insn_event(app_pc addr, uint divisor); // XXX need to transmit context
static dr_emit_flags_t bb_event(void *drcontext, void *tag, instrlist_t *bb,
                                bool for_trace, bool translating);
static void exit_event(void);

// globals
static int div_count = 0;
static int div_p2_count = 0;
static void *div_mutex; // XXX unify mutex for the two variables

DR_EXPORT void
dr_init(client_id_t id)
{
	dr_register_exit_event(exit_event);
	dr_register_bb_event(bb_event);
	div_mutex = dr_mutex_create(); // XXX unify mutex for the two variables
}

static void
handle_insn_event(app_pc addr, uint divisor) // XXX need to transmit context
{
	dr_mutex_lock(div_mutex);
	div_count++;
	if ((divisor & (divisor - 1)) == 0)
		div_p2_count++;
	dr_mutex_unlock(div_mutex);
}

static dr_emit_flags_t
bb_event(void *drcontext, void *tag, instrlist_t *bb,
         bool for_trace, bool translating)
{
	instr_t *instr, *next_instr;
	int opcode;

	for (instr = instrlist_first_app(bb); instr != NULL; instr = next_instr) {
		next_instr = instr_get_next_app(instr);
		opcode = instr_get_opcode(instr);

		if (opcode == OP_div || opcode == OP_idiv) {
			dr_insert_clean_call(drcontext, bb, instr,
			                     (void *)handle_insn_event,
			                     false /* no fp save */, 2,
			                     OPND_CREATE_INTPTR(instr_get_app_pc(instr)),
			                     instr_get_src(instr, 0)
			                       /* divisor is 1st src */);
		}
	}
	return DR_EMIT_DEFAULT;
}

static void
exit_event(void)
{
	dr_fprintf(STDERR, "======\n");
	dr_fprintf(STDERR, "TOTALS\n");
	dr_fprintf(STDERR, " %d div | %d div_p2\n", div_count, div_p2_count); // XXX no need to lock mutex here
}
