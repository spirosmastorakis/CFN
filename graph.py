import networkx as nx
import matplotlib.pyplot as plt
import hashlib
import dill
import json


#Generate a random graph
def generateRandomTree(nodes):
    #G = nx.random_tree(int(nodes))
    #G = nx.gnp_random_tree(nodes, 0.4)
    G = nx.gnr_graph(nodes, 0.1)
    G = nx.generators.directed.random_k_out_graph(10, 3, 0.5)
    return G
    #convert the graph to a tree (unidirectional edges)
    #return nx.bfs_tree(G,0).to_directed() # already directed, returns deep copy


def createUpdate(G, node, root):
    update = {}
    update['name'] = node
    update['type'] = G.nodes[node]['type']
    #data doesn't have duration
    try:
        update['duration'] = G.nodes[node]['duration']
    except KeyError as e:
        pass
    update['root'] = root

    digest = hashlib.md5(dill.dumps(update)).hexdigest()

    return update, digest

def addChildren(G, node, updateList, root):
    print("addChildren", node)
    update, digest = createUpdate(G, node, root)
    updateList.append((G.nodes[node]['start'], update, digest))

    for succ in G.successors(node):
        addChildren(G, succ, updateList, digest)

    return updateList


def translateGraphIntoUpdates(G):
    updates = list()
    #update, digest = createUpdate(G, '/exec/main', 0)

    addChildren(G, '/exec/main', updates, 0)
    print("List of updates", updates)

    #Sort in the end
    sortedUpdates = sorted(updates, key=lambda k: k[0])
    print("Sorted list of updates", sortedUpdates)

    json.dump(sortedUpdates, open("test.json", "w"))

    #print("Received update", update, "digest", digest)

    #addChildren(G, node, list)


def generateDummyTree():
    G = nx.DiGraph()
    G.add_edges_from(
        [('/exec/main', '/exec/Sim/init'), ('/exec/Sim/init', 'Sim/state/0'), ('/exec/main', '/exec/Sim/SetRange/(#)'), ('Sim/state/0', '/exec/Sim/SetRange/(#)'),
        ('/exec/Sim/SetRange/(#)', 'Sim/SetRange/(#)'),('/exec/Sim/SetRange/(#)', 'Sim/state/1'), ('/exec/main', '/exec/getRandom/&'), ('/exec/getRandom/&', 'getRandom/&'),
        ('getRandom/&', '/exec/Sim/run/(#)'), ('Sim/state/1', '/exec/Sim/run/(#)'), ('/exec/main', '/exec/Sim/run/(#)'), ('/exec/Sim/run/(#)', 'Sim/run/(#)')])

    G.nodes['/exec/main']['type'] = 'trans'
    G.nodes['/exec/main']['duration'] = 60
    G.nodes['/exec/main']['start'] = 0

    G.nodes['/exec/Sim/init']['type'] = 'trans'
    G.nodes['/exec/Sim/init']['duration'] = 2
    G.nodes['/exec/Sim/init']['start'] = 2

    G.nodes['Sim/state/0']['type'] = 'data'
    G.nodes['Sim/state/0']['start'] = 4

    G.nodes['/exec/Sim/SetRange/(#)']['type'] = 'trans'
    G.nodes['/exec/Sim/SetRange/(#)']['duration'] = 10
    G.nodes['/exec/Sim/SetRange/(#)']['start'] = 10

    G.nodes['Sim/SetRange/(#)']['type'] = 'data'
    G.nodes['Sim/SetRange/(#)']['start'] = 20

    G.nodes['Sim/state/1']['type'] = 'data'
    G.nodes['Sim/state/1']['start'] = 20

    G.nodes['/exec/getRandom/&']['type'] = 'opaque'
    G.nodes['/exec/getRandom/&']['duration'] = 5
    G.nodes['/exec/getRandom/&']['start'] = 30

    G.nodes['getRandom/&']['type'] = 'data'
    G.nodes['getRandom/&']['start'] = 35

    G.nodes['/exec/Sim/run/(#)']['type'] = 'trans'
    G.nodes['/exec/Sim/run/(#)']['duration'] = 20
    G.nodes['/exec/Sim/run/(#)']['start'] = 40

    G.nodes['Sim/run/(#)']['type'] = 'data'
    G.nodes['Sim/run/(#)']['start'] = 60


    red_edges = [('A', 'C'), ('E', 'C')]

    #G.edges[('/exec/main', '/exec/Sim/init')]['start'] = 2
    #G.edges[('/exec/Sim/init', 'Sim/state/0')]['start'] = 2
    #G.edges[('/exec/main', '/exec/Sim/SetRange/(#)')]['start'] = 5
    #G.edges[('Sim/state/0', '/exec/Sim/SetRange/(#)')]['start'] =
    #G.edges[('/exec/Sim/SetRange/(#)', 'Sim/SetRange/(#)')]['start'] =
    #G.edges[('/exec/Sim/SetRange/(#)', 'Sim/state/1')]['start'] =
    #G.edges[('/exec/main', '/exec/getRandom/&')]['start'] = 10
    #G.edges[('/exec/getRandom/&', 'getRandom/&')]['start'] =
    #G.edges[('getRandom/&', '/exec/Sim/run/(#)')]['start'] =
    #G.edges[('Sim/state/1', '/exec/Sim/run/(#)')]['start'] =
    #G.edges[('/exec/main', '/exec/Sim/run/(#)')]['start'] = 20
    #G.edges[('/exec/Sim/run/(#)', 'Sim/run/(#)')]['start'] =

    return G

#G = generateRandomTree(30)
G = generateDummyTree()

node_colors = []
print("We have", len(G.nodes), "nodes")



for nodeID in G.nodes:
    print(nodeID)
    nodeType = G.node[nodeID]['type']
    if nodeType == 'trans':
        node_colors.append('r')
    elif nodeType == 'opaque':
        node_colors.append('g')
    elif nodeType == 'data':
        node_colors.append('y')
    else:
        print("Unknown node type", nodeType)
        quit()

print("We have", len(G.edges), "edges")
for edgeID in G.edges:
    print(edgeID)

print("Predescessor of /exec/Sim/init", G.pred['/exec/Sim/init'])
print("Sucsessor of /exec/Sim/init", G.successors('/exec/Sim/init'))

translateGraphIntoUpdates(G)

#nx.draw_networkx(G, node_color=node_colors)
#plt.show()
