import networkx as nx
import matplotlib.pyplot as plt



def step(G, current):
    global data
    weights = {}
    print("Current:", current)
    for target in data:
        path = nx.shortest_path(G, source=current, target=target[0])
        length = len(path) - 1
        data_size = target[1]
        if(length > 0):
            next_hop = path[1]
            metric = length * data_size
        else:
            next_hop = current
            metric = data_size
        print("Weights:", weights, "Next hop:", next_hop)
        if next_hop in weights:
            weights[next_hop] += metric
        else:
            weights[next_hop] = metric

    next_hop = max(weights, key=lambda i: weights[i])
    print("Chosen next hop", next_hop, "with weight", weights[next_hop])
    return next_hop

data = [(1, 10), (6, 20)]
starting_node = 3

G = nx.erdos_renyi_graph(10, 0.4, seed=1213, directed=False)
print("Is graph connected?", nx.is_connected(G))

previous = ""
node = starting_node
path = []
while(True):
    next_hop = step(G, node)
    if(next_hop == node or next_hop == previous):
        break
    path.append(next_hop)
    previous = node
    node = next_hop

end_node = node

data_nodes = set([item[0] for item in data ])
print("Nodes with data", data_nodes)

layout = nx.kamada_kawai_layout(G)
plt.axis('off')
ax = plt.gca()

node_colors = []
bbox = dict(boxstyle="round", fc="0.8")
for nodeID in G.nodes:
    if( nodeID in data_nodes):
        node_colors.append('red')
        x = layout[nodeID][0]
        y = layout[nodeID][1]
        #ax.annotate(results[nodeID], xy=(x, y), bbox=bbox)
    elif( nodeID in path):
        node_colors.append('gray')
    else:
        node_colors.append('blue')

x = layout[starting_node][0]
y = layout[starting_node][1]
ax.annotate("start", xy=(x, y+0.05), bbox=bbox)

x = layout[end_node][0]
y = layout[end_node][1]
ax.annotate("end", xy=(x, y+0.05), bbox=bbox)


nx.draw_networkx(G, ax=ax, pos = layout, node_color=node_colors, with_labels=True, font_size=16, alpha=0.3)
plt.show()
