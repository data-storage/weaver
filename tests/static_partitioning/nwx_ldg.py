import sys
import load_from_snap
import networkx as nx
import matplotlib.pyplot as plt
import random

# adds node attribute of which shard node should be placed on
num_shards = 5
capacity = 2
assignments = dict()
shard_sizes = [0] * num_shards


def get_balanced_assignment(tied_shards):
    min_size = shard_sizes[0]
    min_indices = []
    for s in tied_shards:
        if shard_sizes[s] < min_size:
            min_size = shard_sizes[s]
            min_indices = [s]
        elif shard_sizes[s] == min_size:
            min_indices.append(s)

    return random.choice(min_indices)


def penalty(shard):
    return 1.0 - (float(shard_sizes[shard])/float(capacity))

def get_ldg_assignment(nbr_iter):
    num_intersections = [0] * num_shards
    for nbr in nbr_iter:
        if nbr in assignments:
            num_intersections [assignments[nbr]] += 1

    arg_max = 0.0
    max_indices = []
    for i in range(num_shards):
        val = (float(num_intersections[i])*penalty(i))
        if arg_max < val:
            arg_max = val
            max_indices = [i]
        elif arg_max == val:
            max_indices.append(i)
    if len(max_indices) is 1:
        return max_indices[0]
    else:
        return get_balanced_assignment(max_indices)



G = load_from_snap.load(sys.argv)
for n,nbrdict in G.adjacency_iter():
    put_on_shard = get_ldg_assignment(nbrdict.iterkeys())
    assignments[n] = put_on_shard 
    shard_sizes[put_on_shard] += 1

print assignments
colors = [0.0]*len(assignments)
for (n,shard) in assignments.iteritems():
    print n
    colors[n-1] = float(shard)/float(num_shards)
nx.draw(G, node_color=colors)
plt.show()
