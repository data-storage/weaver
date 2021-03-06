/*
 * ===============================================================
 *    Description:  Client methods implementation.
 *
 *        Created:  03/19/2014 11:58:26 PM
 *
 *         Author:  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ===============================================================
 */

#include <random>

#define weaver_debug_
#include "common/message.h"
#include "common/config_constants.h"
#include "client/client_constants.h"
#include "client/weaver_client.h"

// ugly, declared only to get rid of undefined symbol error
// not used in client
DECLARE_CONFIG_CONSTANTS;

using cl::client;
using transaction::pending_update;

client :: client(const char *coordinator="127.0.0.1", uint16_t port=5200, const char *config_file="/etc/weaver.yaml")
    : m_sm(coordinator, port)
    , cur_tx_id(UINT64_MAX)
    , tx_id_ctr(0)
    , handle_ctr(0)
{
    //google::InitGoogleLogging("weaver-client");
    //google::InstallFailureSignalHandler();
    //google::LogToStderr();

    if (!init_config_constants(config_file)) {
        WDEBUG << "error in init_config_constants, exiting now." << std::endl;
        exit(-1);
    }

    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_int_distribution<uint64_t> distribution(0, NumVts-1);
    vtid = distribution(generator);

    assert(m_sm.get_replid(myid) && "get repl_id");
    assert(myid > MaxNumServers);
    myid_str = std::to_string(myid);

    int try_sm = 0;
    while (!maintain_sm_connection()) {
        WDEBUG << "retry sm connection " << try_sm++ << std::endl;
    }

    comm.reset(new cl::comm_wrapper(myid, *m_sm.config()));
}

void
client :: begin_tx()
{
    assert(cur_tx_id == UINT64_MAX && "only one concurrent transaction per client");
    cur_tx_id = ++tx_id_ctr;
}

std::string
client :: create_node(std::string &handle)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::NODE_CREATE_REQ;
    if (handle == "") {
        upd->handle = generate_handle();
    } else {
        upd->handle = handle;
    }
    cur_tx.emplace_back(upd);
    return upd->handle;
}

std::string
client :: create_edge(std::string &handle, std::string &node1, std::string &node2)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::EDGE_CREATE_REQ;
    if (handle == "") {
        upd->handle = generate_handle();
    } else {
        upd->handle = handle;
    }
    upd->handle1 = node1;
    upd->handle2 = node2;
    cur_tx.emplace_back(upd);
    return upd->handle;
}

void
client :: delete_node(std::string &node)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::NODE_DELETE_REQ;
    upd->handle1 = node;
    cur_tx.emplace_back(upd);
}

void
client :: delete_edge(std::string &edge, std::string &node)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::EDGE_DELETE_REQ;
    upd->handle1 = edge;
    upd->handle2 = node;
    cur_tx.emplace_back(upd);
}

void
client :: set_node_property(std::string &node,
    std::string key, std::string value)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::NODE_SET_PROPERTY;
    upd->handle1 = node;
    upd->key.reset(new std::string(std::move(key)));
    upd->value.reset(new std::string(std::move(value)));
    cur_tx.emplace_back(upd);
}

void
client :: set_edge_property(std::string &node, std::string &edge,
    std::string key, std::string value)
{
    assert(cur_tx_id != UINT64_MAX);
    std::shared_ptr<pending_update> upd = std::make_shared<pending_update>();
    upd->type = transaction::EDGE_SET_PROPERTY;
    upd->handle1 = edge;
    upd->handle2 = node;
    upd->key.reset(new std::string(std::move(key)));
    upd->value.reset(new std::string(std::move(value)));
    cur_tx.emplace_back(upd);
}

bool
client :: end_tx()
{
    if (cur_tx_id == UINT64_MAX) {
        WDEBUG << "no active tx to commit" << std::endl;
        return false;
    }

    bool retry;
    bool success;
    message::message recv_msg;
    // currently no retry on timeout/disrupted, pass error to client
    // so it is responsibility of client to ensure that they do not reexec tx that was completed
    do {
        message::message msg;
        msg.prepare_message(message::CLIENT_TX_INIT, cur_tx_id, cur_tx);
        busybee_returncode send_code = send_coord(msg.buf);

        if (send_code == BUSYBEE_DISRUPTED) {
            reconfigure();
            //retry = true;
            //continue;
            return false;
        } else if (send_code != BUSYBEE_SUCCESS) {
            WDEBUG << "end_tx() got busybee_send code " << send_code << ", failing tx" << std::endl;
            return false;
        }

        busybee_returncode recv_code = recv_coord(&recv_msg.buf);

        switch (recv_code) {
            case BUSYBEE_TIMEOUT:
            case BUSYBEE_DISRUPTED:
            reconfigure();
            //retry = true;
            return false;
            break;

            case BUSYBEE_SUCCESS:
            success = true;
            retry = false;
            break;

            default:
            WDEBUG << "end_tx() got busybee_recv code " << recv_code << ", failing tx" << std::endl;
            success = false;
            retry = false;
        }

        if (retry) {
            WDEBUG << "retry end_tx" << std::endl;
        }
    } while (retry);

    if (success) {
        message::msg_type mtype = recv_msg.unpack_message_type();
        assert(mtype == message::CLIENT_TX_SUCCESS || mtype == message::CLIENT_TX_ABORT);
        if (mtype == message::CLIENT_TX_ABORT) {
            success = false;
        }
    }

    cur_tx_id = UINT64_MAX;
    cur_tx.clear();

    return success;
}

template <typename ParamsType>
std::unique_ptr<ParamsType>
client :: run_node_program(node_prog::prog_type prog_to_run, std::vector<std::pair<std::string, ParamsType>> &initial_args)
{
    message::message msg;
    busybee_returncode send_code, recv_code;

#ifdef weaver_benchmark_

    msg.prepare_message(message::CLIENT_NODE_PROG_REQ, prog_to_run, initial_args);
    send_code = send_coord(msg.buf);

    if (send_code != BUSYBEE_SUCCESS) {
        WDEBUG << "node prog send msg fail, send_code: " << send_code << std::endl;
        return nullptr;
    }

    recv_code = recv_coord(&msg.buf);

    if (recv_code != BUSYBEE_SUCCESS) {
        WDEBUG << "node prog return msg fail, recv_code: " << recv_code << std::endl;
        return nullptr;
    }

#else

    bool retry;
    do {
        msg.prepare_message(message::CLIENT_NODE_PROG_REQ, prog_to_run, initial_args);
        send_code = send_coord(msg.buf);

        if (send_code == BUSYBEE_DISRUPTED) {
            reconfigure();
            retry = true;
            continue;
        } else if (send_code != BUSYBEE_SUCCESS) {
            WDEBUG << "node prog send msg fail, send_code: " << send_code << std::endl;
            return nullptr;
        }

        recv_code = recv_coord(&msg.buf);

        switch (recv_code) {
            case BUSYBEE_TIMEOUT:
            case BUSYBEE_DISRUPTED:
            reconfigure();
            retry = true;
            break;

            case BUSYBEE_SUCCESS:
            if (msg.unpack_message_type() == message::NODE_PROG_FAIL) {
                uint64_t fail_req_id;
                msg.unpack_message(message::NODE_PROG_FAIL, fail_req_id);
                retry = true;
            } else {
                retry = false;
            }
            break;

            default:
            WDEBUG << "node prog return msg fail, recv_code: " << recv_code << std::endl;
            return nullptr;
        }
    } while (retry);

#endif

    uint64_t ignore_req_id, ignore_vt_ptr;
    node_prog::prog_type ignore_type;
    std::unique_ptr<ParamsType> toRet(new ParamsType());
    msg.unpack_message(message::NODE_PROG_RETURN, ignore_type, ignore_req_id, ignore_vt_ptr, *toRet);
    return std::move(toRet);
}

node_prog::reach_params
client :: run_reach_program(std::vector<std::pair<std::string, node_prog::reach_params>> &initial_args)
{
    return *run_node_program(node_prog::REACHABILITY, initial_args);
}

node_prog::pathless_reach_params
client :: run_pathless_reach_program(std::vector<std::pair<std::string, node_prog::pathless_reach_params>> &initial_args)
{
    return *run_node_program(node_prog::PATHLESS_REACHABILITY, initial_args);
}

node_prog::clustering_params
client :: run_clustering_program(std::vector<std::pair<std::string, node_prog::clustering_params>> &initial_args)
{
    return *run_node_program(node_prog::CLUSTERING, initial_args);
}

node_prog::two_neighborhood_params
client :: run_two_neighborhood_program(std::vector<std::pair<std::string, node_prog::two_neighborhood_params>> &initial_args)
{
    return *run_node_program(node_prog::TWO_NEIGHBORHOOD, initial_args);
}

/*
node_prog::dijkstra_params
client :: run_dijkstra_program(std::vector<std::pair<std::string, node_prog::dijkstra_params>> initial_args)
{
    return *run_node_program(node_prog::DIJKSTRA, initial_args);
}
*/

node_prog::read_node_props_params
client :: read_node_props_program(std::vector<std::pair<std::string, node_prog::read_node_props_params>> &initial_args)
{
    return *run_node_program(node_prog::READ_NODE_PROPS, initial_args);
}

node_prog::read_edges_props_params
client :: read_edges_props_program(std::vector<std::pair<std::string, node_prog::read_edges_props_params>> &initial_args)
{
    return *run_node_program(node_prog::READ_EDGES_PROPS, initial_args);
}

node_prog::read_n_edges_params
client :: read_n_edges_program(std::vector<std::pair<std::string, node_prog::read_n_edges_params>> &initial_args)
{
    return *run_node_program(node_prog::READ_N_EDGES, initial_args);
}

node_prog::edge_count_params
client :: edge_count_program(std::vector<std::pair<std::string, node_prog::edge_count_params>> &initial_args)
{
    return *run_node_program(node_prog::EDGE_COUNT, initial_args);
}

node_prog::edge_get_params
client :: edge_get_program(std::vector<std::pair<std::string, node_prog::edge_get_params>> &initial_args)
{
    return *run_node_program(node_prog::EDGE_GET, initial_args);
}

node_prog::traverse_props_params
client :: traverse_props_program(std::vector<std::pair<std::string, node_prog::traverse_props_params>> &initial_args)
{
    for (auto &p: initial_args) {
        if (p.second.node_props.size() != (p.second.edge_props.size()+1)) {
            WDEBUG << "bad params, #node_props should be (#edge_props + 1)" << std::endl;
            return node_prog::traverse_props_params();
        }
    }
    return *run_node_program(node_prog::TRAVERSE_PROPS, initial_args);
}

void
client :: start_migration()
{
    message::message msg;
    msg.prepare_message(message::START_MIGR);
    send_coord(msg.buf);
}

void
client :: single_stream_migration()
{
    message::message msg;
    msg.prepare_message(message::ONE_STREAM_MIGR);
    send_coord(msg.buf);

    if (recv_coord(&msg.buf) != BUSYBEE_SUCCESS) {
        WDEBUG << "single stream migration return msg fail" << std::endl;
    }
}


void
client :: exit_weaver()
{
    message::message msg;
    msg.prepare_message(message::EXIT_WEAVER);
    send_coord(msg.buf);
}

std::vector<uint64_t>
client :: get_node_count()
{
    std::vector<uint64_t> node_count;

    while(true) {
        message::message msg;
        msg.prepare_message(message::CLIENT_NODE_COUNT);
        busybee_returncode send_code = send_coord(msg.buf);

        if (send_code == BUSYBEE_DISRUPTED) {
            reconfigure();
            continue;
        } else if (send_code != BUSYBEE_SUCCESS) {
            WDEBUG << "node count send msg fail with " << send_code << std::endl;
            return node_count;
        }

        busybee_returncode recv_code = recv_coord(&msg.buf);

        switch (recv_code) {
            case BUSYBEE_DISRUPTED:
            case BUSYBEE_TIMEOUT:
            reconfigure();
            break;

            case BUSYBEE_SUCCESS:
            msg.unpack_message(message::NODE_COUNT_REPLY, node_count);
            return node_count;

            default:
            WDEBUG << "node count recv msg fail with " << recv_code << std::endl;
            return node_count;
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
busybee_returncode
client :: send_coord(std::auto_ptr<e::buffer> buf)
{
    return comm->send(vtid, buf);
}

busybee_returncode
client :: recv_coord(std::auto_ptr<e::buffer> *buf)
{
    busybee_returncode ret;
    while (true) {
        comm->quiesce_thread();
        ret = comm->recv(buf);
        switch (ret) {
            case BUSYBEE_SUCCESS:
            case BUSYBEE_TIMEOUT:
                return ret;

            case BUSYBEE_INTERRUPTED:
                continue;

            default:
                return ret;
        }
    }
}
#pragma GCC diagnostic pop

std::string
client :: generate_handle()
{
    std::string s = std::to_string(handle_ctr++);
    s += myid_str;
    return s;
}

bool
client :: maintain_sm_connection()
{
    replicant_returncode rc;

    if (!m_sm.ensure_configuration(&rc))
    {
        if (rc == REPLICANT_INTERRUPTED)
        {
            WDEBUG << "signal received";
        }
        else if (rc == REPLICANT_TIMEOUT)
        {
            WDEBUG << "operation timed out";
        }
        else
        {
            WDEBUG << "coordinator failure: " << m_sm.error().msg();
        }

        return false;
    }

    return true;
}

void
client :: reconfigure()
{
    uint32_t try_sm = 0;
    while (!maintain_sm_connection()) {
        WDEBUG << "retry sm connection " << try_sm++ << std::endl;
    }

    comm.reset(new cl::comm_wrapper(myid, *m_sm.config()));
    comm->reconfigure(*m_sm.config());
}
