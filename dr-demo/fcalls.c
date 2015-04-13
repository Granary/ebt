// fcalls.ebt :: Function call tracing (XXX single-threaded for now)

#include "dr_api.h"
#include "drsyms.h"

// common defines
#define MAX_SYM_RESULT 256

// forward decls
static void handle_function_entry(app_pc call_addr, app_pc fn_addr); // XXX need to transmit context properly
static void handle_function_exit(app_pc call_addr, app_pc fn_addr); // XXX need to transmit context properly
static dr_emit_flags_t bb_event(void *drcontext, void *tag, instrlist_t *bb,
                                bool for_trace, bool translating);
static void exit_event(void);

// globals
int level;
static void *level_mutex; // XXX multiple threads using this is obvious nonsense

static void
lookup_fname(char *function_name, app_pc addr)
{
	// XXX cache context computations
	drsym_error_t symres;
	drsym_info_t sym;
	char module_name[MAXIMUM_PATH];
	module_data_t *data;

	data = dr_lookup_module(addr);
	if (data == NULL) {
		dr_snprintf(function_name, MAX_SYM_RESULT, "(function in unknown module)");
		return;
	}

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
}

static void
handle_function_entry(app_pc call_addr, app_pc fn_addr) // XXX need to transmit context properly
{
	// XXX compute fname from fn_addr -- should be done statically when possible
	char fname[MAX_SYM_RESULT]; lookup_fname(fname, fn_addr);

	dr_mutex_lock(level_mutex);
	int i;
	for (i = 0; i < level; i++) dr_fprintf(STDERR, "  ");
	dr_fprintf(STDERR, "--> %s()\n", fname);
	level++;
	dr_mutex_unlock(level_mutex);
}

static void
handle_function_exit(app_pc addr, app_pc fn_addr) // XXX need to transmit context properly
{
	// XXX compute fname from fn_addr -- should be done statically when possible
	char fname[MAX_SYM_RESULT]; lookup_fname(fname, fn_addr);

	dr_mutex_lock(level_mutex);
	int i;
	for (i = 0; i < level; i++) dr_fprintf(STDERR, "  ");
	dr_fprintf(STDERR, "<-- %s()\n", fname);
	level--;
	dr_mutex_unlock(level_mutex);
}

DR_EXPORT void
dr_init(client_id_t id)
{
	dr_register_exit_event(exit_event);
	dr_register_bb_event(bb_event);

	level_mutex = dr_mutex_create();

	if (drsym_init(0) != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: unable to initialize symbol translation\n");
	}
}

// TODOXXX rewrite below

static dr_emit_flags_t
bb_event(void *drcontext, void *tag, instrlist_t *bb,
         bool for_trace, bool translating)
{
	instr_t *instr, *next_instr;

	for (instr = instrlist_first_app(bb); instr != NULL; instr = next_instr) {
		next_instr = instr_get_next_app(instr);
		// opcode = instr_get_opcode(instr); // XXX not used

		if (!instr_opcode_valid(instr))
			continue; // XXX this check may be necessary elsewhere

		// instrument calls and returns -- ignore far calls/rets
		if (instr_is_call_direct(instr)) {
			dr_insert_call_instrumentation(drcontext, bb, instr, (app_pc)handle_function_entry);
		} else if (instr_is_call_indirect(instr)) {
			dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)handle_function_entry, SPILL_SLOT_1);
		} else if (instr_is_return(instr)) {
			dr_insert_mbr_instrumentation(drcontext, bb, instr, (app_pc)handle_function_exit, SPILL_SLOT_1);
		}
	}
	return DR_EMIT_DEFAULT;
}

static void
exit_event(void)
{
	if (drsym_exit() != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: error cleaning up symbol library\n");
	}
}
