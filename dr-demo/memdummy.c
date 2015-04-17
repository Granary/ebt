// memdummy.c :: Just initialize some shadow memory

#include "dr_api.h"
#include "umbra.h"

// forward decls
static void event_exit(void);

// TODOXXX need to also declare the shadow memory

DR_EXPORT void
dr_init(client_id_t id)
{
	umbra_init(id);

	dr_fprintf(STDERR, "It did not asplode.\n");
	dr_register_exit_event(event_exit);
}

static void
event_exit(void)
{
	umbra_exit();
}
