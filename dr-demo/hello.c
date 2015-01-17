// hello.c :: print "Hello" and "Goodbye" as the target starts and ends

#include "dr_api.h"

// forward decls
static void event_exit(void);

DR_EXPORT void
dr_init(client_id_t id)
{
	dr_fprintf(STDERR, "Hello, World!\n");
	dr_register_exit_event(event_exit);
}

static void
event_exit(void)
{
	dr_fprintf(STDERR, "Goodbye, World!\n");
}
