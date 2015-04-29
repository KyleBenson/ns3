import sys

"""
Helps determine if all nodes that attempted to contact
a server via the overlay successfully managed to do so.
"""

with open(sys.argv[1], 'r') as f:
    attempting_nodes = set()
    successful_nodes = set()
    for line in f.readlines():
        nodeId = line.split()[1]
        if 'sent indirect packet' in line:
            attempting_nodes.add(nodeId)
        elif 'received indirect ACK' in line:
            successful_nodes.add(nodeId)

    print "Attempting nodes:", attempting_nodes
    print "Successful nodes:", successful_nodes
    assert(attempting_nodes == successful_nodes)
