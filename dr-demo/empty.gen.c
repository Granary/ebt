/* generated by ebt version 0.0 */

#include "dr_api.h"
#include "runtime/opcode.h"

// forward declarations for callbacks
static void event_exit(void);

// global data -- probe counters, variables, mutexes

// callbacks
DR_EXPORT void
dr_init(client_id_t id)
{
  /* initialize client */
  ;
  dr_register_exit_event(event_exit);
}

static void 
event_exit(void)
{
  /* terminate client */
  ;
}

