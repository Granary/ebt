// insn_div_array.c :: count the number of DIV instructions in each function

#include "dr_api.h"
#include "drsyms.h"

// For understanding the hashtable, it's useful to look at DR's implementation:
// - https://github.com/DynamoRIO/dynamorio/blob/master/ext/drcontainers/hashtable.c
// - https://github.com/DynamoRIO/dynamorio/blob/master/ext/drcontainers/hashtable.h
#include "hashtable.h"

// common defines
#define MAX_SYM_RESULT 256
#define HASH_SIZE_DEFAULT 8
// XXX need to pick a reasonable # of hash bits

// common map functions
// XXX this should use the common map implementation in ../runtime/
static int _map_get(hashtable_t *m, const char *key) {
	void *result = hashtable_lookup(m, (void *) key);
	return result == NULL ? 0 : (long int) result;
}

static void _map_set(hashtable_t *m, const char *key, long int val) {
	// XXX actually, the hashtable won't be able to store 0's
	hashtable_add_replace(m, (void *) key, (void *) val);
}

// forward decls
static void handle_insn_event(app_pc addr, const char *fname, uint divisor); // XXX need to transmit context properly
static dr_emit_flags_t bb_event(void *drcontext, void *tag, instrlist_t *bb,
                                bool for_trace, bool translating);
static void exit_event(void);

// globals
hashtable_t div_counts;
hashtable_t div_p2_counts;
static void *div_mutex; // XXX unify mutex for the two variables OR use hashtable_lock for the individual hashtables

DR_EXPORT void
dr_init(client_id_t id)
{
	dr_register_exit_event(exit_event);
	dr_register_bb_event(bb_event);

	// initialize maps
	hashtable_init(&div_counts, HASH_SIZE_DEFAULT, HASH_STRING, 0);
	hashtable_init(&div_p2_counts, HASH_SIZE_DEFAULT, HASH_STRING, 0);
	// XXX probably want unsynchronized hash tables, actually
	div_mutex = dr_mutex_create(); // XXX unify mutex for the two variables

	if (drsym_init(0) != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: unable to initialize symbol translation\n");
	}
}

// The instruction event remains minimal; we check the function name
// during the basic block callback.
static void
handle_insn_event(app_pc addr, const char *fname, uint divisor) // XXX need to transmit context properly
{
	dr_mutex_lock(div_mutex);
	int div_count = _map_get(&div_counts, fname);
	div_count++;
	_map_set(&div_counts, fname, div_count);
	if ((divisor & (divisor - 1)) == 0) {
		int div_p2_count = _map_get(&div_p2_counts, fname);
		div_p2_count++;
		_map_set(&div_p2_counts, fname, div_p2_count);
	}
	
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
	char *function_name = (char *) dr_global_alloc (MAX_SYM_RESULT * sizeof(char)); // XXX pass pointer to the handler function, somehow don't leak the memory
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

			if (found_function) {
				dr_insert_clean_call(drcontext, bb, instr,
				                     (void *)handle_insn_event,
				                     false /* no fp save */, 3,
				                     OPND_CREATE_INTPTR(addr),
				                     OPND_CREATE_INTPTR(function_name), // TODOXXX operand format
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

	// iterate the div_count table
	// XXX a hack requiring some stability of hashtable.h formats
	int i;
	for (i = 0; i < HASHTABLE_SIZE(div_counts.table_bits); i++)
	{
		hash_entry_t *e = div_counts.table[i];
		while (e != NULL) {
			const char *fname = (const char *) e->key;
			int div_count = (long int) e->payload;
			int div_p2_count = _map_get(&div_p2_counts, fname);
			dr_fprintf(STDERR, " %6d div | %6d div_p2 in %s()\n",
			           div_count, div_p2_count, fname);
			e = e->next;
		}
	}
	// XXX no need to lock mutexes here

	if (drsym_exit() != DRSYM_SUCCESS) {
		dr_log(NULL, LOG_ALL, 1, "WARNING: error cleaning up symbol library\n");
	}
}
