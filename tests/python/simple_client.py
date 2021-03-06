#! /usr/bin/env python
# 
# ===============================================================
# Copyright (C) 2013, Cornell University, see the LICENSE file
#                     for licensing agreement
# ===============================================================
# 

import sys
import time

import weaver.client as client

class simple_client:
    def __init__(self, client_to_wrap):
        self.c= client_to_wrap

    def node_props(self, id, keys = []):
        rp = client.ReadNodePropsParams(keys = keys)
        prog_args = [(id, rp)]
        response = self.c.read_node_props(prog_args)
        return response.node_props

    def edges_props(self, id, keys = []):
        rp = client.ReadEdgesPropsParams(keys = keys)
        prog_args = [(id, rp)]
        response = self.c.read_edges_props(prog_args)
        return response.edges_props

    def reachability(self, source, dest, edge_props = [], caching = False):
        rp = client.ReachParams(dest=dest, edge_props=edge_props, caching=caching)
        prog_args = [(source, rp)]
        response = self.c.run_reach_program(prog_args)
        return (response.reachable, response.path)

    def two_neighborhood(self, id, prop_key, caching = False):
        tp = client.TwoNeighborhoodParams(prop_key = prop_key, caching = caching) 
        prog_args = [(id, tp)]
        response = self.c.run_two_neighborhood_program(prog_args)
        return response.responses;

    def clustering(self, id):
        cp = client.ClusteringParams(caching=False)
        prog_args = [(id, cp)]
        response = self.c.run_clustering_program(prog_args)
        return response.clustering_coeff;
