/*
 * ===============================================================
 *    Description:  Thread pool for all servers except central
 *
 *        Created:  01/09/2013 12:00:30 PM
 *
 *         Author:  Ayush Dubey, dubey@cs.cornell.edu
 *
 * Copyright (C) 2013, Cornell University, see the LICENSE file
 *                     for licensing agreement
 * ================================================================
 */

#ifndef __THREADPOOL__
#define __THREADPOOL__

#include <vector>
#include <deque>
#include <queue>
#include <thread>
#include <po6/threads/mutex.h>
#include <po6/threads/cond.h>

#include "graph.h"
#include "common/message.h"

namespace db
{
class graph;
class update_request;
class batch_request;

namespace thread
{
    class pool;
    void worker_thread_loop(pool *tpool);

    class unstarted_thread
    {
        public:
            unstarted_thread(
                size_t s,
                void (*f)(graph *, void *),
                graph *g,
                void *a);

        public:
            bool operator>(const unstarted_thread &t) const;

        public:
            size_t start_time;
            void (*func)(graph *, void *);
            graph *G;
            void *arg;
    };

    inline
    unstarted_thread :: unstarted_thread( 
            size_t s,
            void (*f)(graph *, void *),
            graph *g,
            void *a)
        : start_time(s)
        , func(f)
        , G(g)
        , arg(a)
    {
    }

    // for priority_queue
    struct work_thread_compare 
        : std::binary_function<unstarted_thread*, unstarted_thread*, bool>
    {
        bool operator()(const unstarted_thread* const &r1, const unstarted_thread* const&r2)
        {
            return (r1->start_time) > (r2->start_time);
        }
    };
    
    /*
    class unstarted_update_thread
    {
        public:
            unstarted_update_thread(
                void (*f)(graph*, update_request*),
                graph *g,
                update_request *r);

        public:
            bool operator>(const unstarted_update_thread &t) const;

        public:
            void (*func)(graph*, update_request*);
            graph *G;
            update_request *req;
    };

    inline
    unstarted_update_thread :: unstarted_update_thread( 
            void (*f)(graph*, update_request*),
            graph *g,
            update_request *r)
        : func(f)
        , G(g)
        , req(r)
    {
    }

    // for priority_queue
    struct update_req_compare 
        : std::binary_function<unstarted_update_thread*, unstarted_update_thread*, bool>
    {
        bool operator()(const unstarted_update_thread* const &r1, const unstarted_update_thread* const&r2)
        {
            return (*r1 > *r2);
        }
    };
    */
    
    class pool
    {
        public:
            int num_threads;
            std::priority_queue<unstarted_thread*, std::vector<unstarted_thread*>, work_thread_compare> work_queue;
            po6::threads::mutex queue_mutex;
            po6::threads::cond work_queue_cond;
       
        public:
            void add_request(unstarted_thread*);
        
        public:
            pool(int n_threads);
    };

    inline
    pool :: pool(int n_threads)
        : num_threads(n_threads)
        , work_queue_cond(&queue_mutex)
    {
        int i;
        std::unique_ptr<std::thread> t;
        for (i = 0; i < num_threads; i++)
        {
            t.reset(new std::thread(worker_thread_loop, this));
            t->detach();
        }
    }

    inline void
    pool :: add_request(unstarted_thread *t)
    {
        queue_mutex.lock();
        work_queue_cond.broadcast();
        work_queue.push(t);
        queue_mutex.unlock();
    }
} 
}

#endif //__THREADPOOL__
