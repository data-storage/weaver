/*
 * ===============================================================
 *    Description:  Implementation of read-node node program.
 *
 *        Created:  2014-05-30 12:37:27
 *
 *         Author:  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#include "common/message.h"
#include "node_prog/node.h"
#include "node_prog/edge.h"
#include "node_prog/read_node_props_program.h"

using node_prog::search_type;
using node_prog::read_node_props_params;
using node_prog::read_node_props_state;
using node_prog::cache_response;

uint64_t
read_node_props_params :: size() const 
{
    uint64_t toRet = message::size(keys)
        + message::size(node_props);
    return toRet;
}

void
read_node_props_params :: pack(e::buffer::packer& packer) const 
{
    message::pack_buffer(packer, keys);
    message::pack_buffer(packer, node_props);
}

void
read_node_props_params :: unpack(e::unpacker& unpacker)
{
    message::unpack_buffer(unpacker, keys);
    message::unpack_buffer(unpacker, node_props);
}

std::pair<search_type, std::vector<std::pair<db::element::remote_node, read_node_props_params>>>
node_prog :: read_node_props_node_program(
        node &n,
        db::element::remote_node &,
        read_node_props_params &params,
        std::function<read_node_props_state&()>,
        std::function<void(std::shared_ptr<node_prog::Cache_Value_Base>,
            std::shared_ptr<std::vector<db::element::remote_node>>, cache_key_t)>&,
        cache_response<Cache_Value_Base>*)
{
    bool fetch_all = params.keys.empty();
    for (property &prop : n.get_properties()) {
        if (fetch_all || (std::find(params.keys.begin(), params.keys.end(), prop.get_key()) != params.keys.end())) {
            params.node_props.emplace_back(prop.get_key(), prop.get_value());
        }
    }

    return std::make_pair(search_type::DEPTH_FIRST, std::vector<std::pair<db::element::remote_node, read_node_props_params>>
            (1, std::make_pair(db::element::coordinator, std::move(params)))); 
}
