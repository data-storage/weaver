/*
 * ===============================================================
 *    Description:  Replicant state machine definition.
 *
 *        Created:  2014-02-10 12:06:06
 *
 *         Author:  Robert Escriva, escriva@cs.cornell.edu
 *                  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

// Most of the following code has been 'borrowed' from
// Robert Escriva's HyperDex coordinator.
// see https://github.com/rescrv/HyperDex for the original code.

/* Replicant */
#include <replicant_state_machine.h>

/* Weaver */
#include "coordinator/transitions.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"

struct replicant_state_machine rsm = {
    weaver_server_manager_create,
    weaver_server_manager_recreate,
    weaver_server_manager_destroy,
    weaver_server_manager_snapshot,
    {{"config_get", weaver_server_manager_config_get},
     {"config_ack", weaver_server_manager_config_ack},
     {"config_stable", weaver_server_manager_config_stable},
     {"replid_get", weaver_server_manager_replid_get},
     {"server_register", weaver_server_manager_server_register},
     {"server_online", weaver_server_manager_server_online},
     {"server_offline", weaver_server_manager_server_offline},
     {"server_shutdown", weaver_server_manager_server_shutdown},
     {"server_kill", weaver_server_manager_server_kill},
     {"server_forget", weaver_server_manager_server_forget},
     {"server_suspect", weaver_server_manager_server_suspect},
     {"alarm", weaver_server_manager_alarm},
     {"debug_dump", weaver_server_manager_debug_dump},
     {"init", weaver_server_manager_init},
     {NULL, NULL}}
};

#pragma GCC diagnostic pop
