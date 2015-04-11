// insn_div_fn.c :: count the number of DIV instructions
// ... only include divisions in the function calculate()

#include "dr_api.h"
#include "drsyms.h"

// common defines
#define MAX_SYM_RESULT 256

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

	if (drsym_init(0) != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: unable to initialize symbol translation\n");
	}
}

// The instruction event remains minimal; we check the function name
// during the basic block callback.
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
	int opcode;

	// XXX cache context computations
	app_pc addr;
	drsym_error_t symres;
	drsym_info_t sym;
	char function_name[MAX_SYM_RESULT];
	char module_name[MAXIMUM_PATH];
	module_data_t *data; int found_module = 0;
	int found_function = 0;

	instr_t *instr, *next_instr;

	for (instr = instrlist_first_app(bb); instr != NULL; instr = next_instr) {
		next_instr = instr_get_next_app(instr);
		opcode = instr_get_opcode(instr);

		if (opcode == OP_div || opcode == OP_idiv) {
			// Check the name of the current function:
			if (!found_module) {
				addr = instr_get_app_pc(instr);
				data = dr_lookup_module(addr);
				if (data == NULL) {
					dr_snprintf(function_name, MAX_SYM_RESULT, "(function in unknown module)");
					found_function = 1;
				}
				found_module = 1;
			}

			if (!found_function) {
				sym.struct_size = sizeof(sym);
				sym.name = function_name;
				sym.name_size = MAX_SYM_RESULT;
				sym.file = module_name;
				sym.file_size = MAXIMUM_PATH;
				symres = drsym_lookup_address(data->full_path, addr - data->start,
				                              &sym, DRSYM_DEFAULT_FLAGS);
				if (symres != DRSYM_SUCCESS
				    && symres != DRSYM_ERROR_LINE_NOT_AVAILABLE) {
					dr_snprintf(function_name, MAX_SYM_RESULT, "(unknown function)");
				}
				found_function = 1;
			}

			if (found_function && strcmp(function_name, "calculate") == 0) {
				dr_insert_clean_call(drcontext, bb, instr,
				                     (void *)handle_insn_event,
				                     false /* no fp save */, 2,
				                     OPND_CREATE_INTPTR(addr),
				                     instr_get_src(instr, 0)
				                       /* divisor is 1st src */);
			}
		}
	}
	return DR_EMIT_DEFAULT;
}

static void
exit_event(void)
{
	dr_fprintf(STDERR, "======\n");
	dr_fprintf(STDERR, "TOTALS\n");
	dr_fprintf(STDERR, " %d div | %d div_p2 in %s()\n",
	           div_count, div_p2_count, "calculate");
	// XXX no need to lock mutex here

	if (drsym_exit() != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: error cleaning up symbol library\n");
	}
}
