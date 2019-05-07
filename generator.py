import networkx as nx
import matplotlib.pyplot as plt
import hashlib
import dill
import json
import random

def generateRandomTree(nodes):
    G = nx.random_tree(int(nodes))
    #G = nx.gnp_random_tree(nodes, 0.4)
    #G = nx.gnr_graph(nodes, 0.1)
    #G = nx.generators.directed.random_k_out_graph(10, 3, 0.5)
    return nx.bfs_tree(G,0).to_directed() # already directed, returns deep copy
    return G
    #convert the graph to a tree (unidirectional edges)
    #return nx.bfs_tree(G,0).to_directed() # already directed, returns deep copy

tree_size = 1000
outputs_size = 10
for data_size in range(10, 65, 5):
    for inputs_num in range (0, 21):
        for topo in range(1, 2):
            G = generateRandomTree(tree_size)

            print("Our roots:", [n for n,d in G.in_degree() if d==0] )
            print("Our leafs:", [n for n,d in G.out_degree() if d==0] )

            counter = 0
            topo_sorted = list(nx.topological_sort(G))
            update_list = list()

            for node in  topo_sorted:
                update = {}
                outputs = random.randrange(0, outputs_size)

                update['name'] = "/exec/f"+str(node)
                update['type'] = 0

                update['caller'] = ""
                preds = G.predecessors(node)
                for pred in preds:
                    print("pred", pred)
                    update['caller'] = "/exec/f" + str(pred)

                update['inputs'] = list()
                update['outputs'] = list()
                for i in range(0, outputs):
                    size = random.randrange(1, data_size)
                    update['outputs'].append(update['name'] + "/d" + str(i) + ":" + str(size))
                    #outputs = random.randrange(0, 3)
                update['thunk'] = ""
                update['start'] = counter
                update['duration'] =  random.randrange(1, 30)

                counter += 1
                update_list.append(update)

            for index in range(0, len(update_list)):
                update = update_list[index]
                for nested_index in range(index+1, len(update_list)):

                    nested_update = update_list[nested_index]
                    #print("Nested name:", nested_update['name'], "len:", len(nested_update['inputs']), end='')
                    #if(len(nested_update['inputs']) < inputs_num):
                        #print("in")
                    for output in update['outputs']:
                        if(random.random() > 0.7 and len(nested_update['inputs']) < inputs_num):
                            nested_update['inputs'].append(output)

                    #else:
                        #print("out")

            for update in update_list:
                print(update)

            json.dump(update_list, open("locality"+"_inputs_" + str(inputs_num) + "_datasize_" + str(data_size) +"_" + str(topo) + ".json", "w"))
#nx.draw_networkx(G)
#plt.show()

quit()


update_list = list()
for i in range(0, 10):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    if(i > 0):
        update['caller'] = "/exec/f"+str(i-1)
    else:
        update['caller'] = ""
    update['inputs'] = list()
    update['outputs'] = list()
    update['thunk'] = ""
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_10.json", "w"))


update_list = list()
for i in range(0, 1000):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    if(i > 0):
        update['caller'] = "/exec/f"+str(i-1)
    else:
        update['caller'] = ""
    update['inputs'] = list()
    update['outputs'] = list()
    update['thunk'] = ""
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_1000.json", "w"))



update_list = list()
for i in range(0, 10):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    if(i > 0):
        update['caller'] = "/exec/f"+str(i-1)
    else:
        update['caller'] = ""
    update['inputs'] = list()
    if(i > 0):
        update['inputs'].append("d"+str(i-1) + ":44")
    update['outputs'] = list()
    update['outputs'].append("d"+str(i) + ":44")
    update['thunk'] = ""
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_data_10.json", "w"))


update_list = list()
for i in range(0, 1000):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    if(i > 0):
        update['caller'] = "/exec/f"+str(i-1)
    else:
        update['caller'] = ""
    update['inputs'] = list()
    if(i > 0):
        update['inputs'].append("d"+str(i-1) + ":44")
    update['outputs'] = list()
    update['outputs'].append("d"+str(i) + ":44")
    update['thunk'] = ""
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_data_1000.json", "w"))
