#ifndef weaver_node_prog_triangle_program_h_
#define weaver_node_prog_triangle_program_h_
#include <vector>

#include "common/weaver_constants.h"
#include "db/node.h"
#include "db/remote_node.h"
#include "common/message.h"
#include "common/vclock.h"

namespace node_prog
{
    class triangle_params : public virtual Packable 
    {
        public:
            //uint64_t triangles_found;
            uint64_t responses_left;
            uint64_t num_edges;
            bool returning;
            std::vector<uint64_t> neighbors;
            db::element::remote_node super_node;

        public:
            triangle_params()
            : responses_left(0)
            , num_edges(0)
            , returning(false)
            , neighbors()
            , super_node()
            {
            }
            
            virtual ~triangle_params() { }

            virtual uint64_t size() const 
            {
                uint64_t toRet = message::size(responses_left)
                    + message::size(num_edges)
                    + message::size(returning) 
                    + message::size(neighbors) 
                    + message::size(super_node);
                    return toRet;
            }

            virtual void pack(e::buffer::packer &packer) const 
            {
                message::pack_buffer(packer, responses_left);
                message::pack_buffer(packer, num_edges);
                message::pack_buffer(packer, returning);
                message::pack_buffer(packer, neighbors);
                message::pack_buffer(packer, super_node);
            }

            virtual void unpack(e::unpacker &unpacker)
            {
                message::unpack_buffer(unpacker, responses_left);
                message::unpack_buffer(unpacker, num_edges);
                message::unpack_buffer(unpacker, returning);
                message::unpack_buffer(unpacker, neighbors);
                message::unpack_buffer(unpacker, super_node);
            }
    };

    struct triangle_node_state : Packable_Deletable
    {
            uint64_t responses_left;
            uint64_t total;

        triangle_node_state()
            : responses_left(0)
            , total(0)
        { }

        virtual ~triangle_node_state() { }

        virtual uint64_t size() const 
        {
            uint64_t toRet = message::size(responses_left)
                + message::size(total);
            return toRet;
        }

        virtual void pack(e::buffer::packer& packer) const 
        {
            message::pack_buffer(packer, responses_left);
            message::pack_buffer(packer, total);
        }

        virtual void unpack(e::unpacker& unpacker)
        {
            message::unpack_buffer(unpacker, responses_left);
            message::unpack_buffer(unpacker, total);
        }
    };

    inline int get_num_edges(db::element::node &n, vc::vclock &req_vclock) {
        int toRet = 0;
        db::element::edge *e;
        for (auto &iter: n.out_edges) {
            e = iter.second;
            // check edge created and deleted in acceptable timeframe
            int64_t cmp_1 = order::compare_two_vts(e->get_creat_time(), req_vclock);
            assert(cmp_1 != 2);
            bool traverse_edge = (cmp_1 == 0);
            if (traverse_edge) {
                int64_t cmp_2 = order::compare_two_vts(e->get_del_time(), req_vclock);
                assert(cmp_2 != 2);
                traverse_edge = (cmp_2 == 1);
            }
            if (traverse_edge){
                toRet++;
            }
        }
        return toRet;
    }

    std::vector<std::pair<db::element::remote_node, triangle_params>> 
    triangle_node_program(uint64_t, // TODO used to be req_id, now replaced by vclock
            db::element::node &n,
            db::element::remote_node &rn,
            triangle_params &params,
            std::function<triangle_node_state&()> state_getter,
            vc::vclock &req_vclock)
    {
        WDEBUG << "inside node prog!\n";
        std::vector<std::pair<db::element::remote_node, triangle_params>> next;
        if (rn.handle == params.super_node.handle) {
            triangle_node_state &state = state_getter();

            if (state.responses_left == 0) { // this only happens when state is not yet initialized
                state.responses_left = params.responses_left;
                state.total = 0;
            }
            if (!params.returning) { // this is the prog to count for the super node, happens once
                state.total += get_num_edges(n, req_vclock);

            } else {
                state.total += params.num_edges;
            }
            // at end, send total for this shard to coordinator
            if (--state.responses_left == 0) {
                params.num_edges = state.total;
                params.super_node.loc = COORD_ID;
                next.emplace_back(std::make_pair(params.super_node, params));
            }
        } else {  // not at super node
            params.num_edges = get_num_edges(n, req_vclock);
            params.returning = true;
            // send to canonical node
            next.emplace_back(std::make_pair(params.super_node, params));
        }
        return next;
    }

    /*
    std::vector<std::pair<db::element::remote_node, reach_params>> 
    reach_node_deleted_program(uint64_t req_id,
                db::element::node &n, // node who asked to go to deleted node
                uint64_t deleted_handle, // handle of node that didn't exist
            reach_params &params_given, // params we had sent to deleted node
            std::function<reach_node_state&()> state_getter)
    {
        UNUSED(req_id);
        UNUSED(n);
        UNUSED(deleted_handle);
        UNUSED(params_given);
        UNUSED(state_getter);
        return std::vector<std::pair<db::element::remote_node, reach_params>>(); 
    }
    */
}

#endif //__TRIANGLE_PROG__
