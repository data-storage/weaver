/*
 * ===============================================================
 *    Description:  Data associated with periodic heartbeat
 *                  message sent from each vector timestamper to
 *                  each shard.       
 *
 *        Created:  09/19/2013 09:43:24 PM
 *
 *         Author:  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#ifndef weaver_db_nop_data_h_
#define weaver_db_nop_data_h_

#include "node_prog/node_prog_type.h"

namespace db
{
    struct nop_data
    {
        uint64_t vt_id;
        vc::vclock vclk;
        vc::qtimestamp_t qts;
        uint64_t req_id;
        std::vector<std::pair<uint64_t, node_prog::prog_type>> done_reqs;
        uint64_t max_done_id;
        vc::vclock_t max_done_clk;
        uint64_t outstanding_progs;
        std::vector<uint64_t> shard_node_count;
    };
}

#endif
