/*
 * ===============================================================
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#ifndef weaver_node_prog_node_h_
#define weaver_node_prog_node_h_

#include <stdint.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <unordered_map>

#include "common/types.h"
#include "node_prog/edge_list.h"

namespace node_prog
{
    class node
    {
        public:
            virtual ~node() { }
            virtual node_handle_t get_handle() const = 0;
            virtual edge_list get_edges() = 0;
            virtual prop_list get_properties() = 0;
            virtual bool has_property(std::pair<std::string, std::string> &p) = 0;
            virtual bool has_all_properties(std::vector<std::pair<std::string, std::string>> &props) = 0;
    };
}

#endif
