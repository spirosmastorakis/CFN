import networkx as nx
import matplotlib.pyplot as plt
import hashlib
import dill
import json


#[{\"name\": \"/exec/main/()\", \"type\": \"0\", \"inputs\": [], \"outputs\": [\"data1:10\", \"data2:20\"], \"thunk\": \"/node1/\", \"start\": 10, \"duration\": \"30\"}
update_list = list()
for i in range(1, 10):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    update['inputs'] = list()
    update['outputs'] = list()
    update['thunk'] = 0
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_10.json", "w"))


update_list = list()
for i in range(1, 1000):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    update['inputs'] = list()
    update['outputs'] = list()
    update['thunk'] = 0
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_1000.json", "w"))



update_list = list()
for i in range(1, 10):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    update['inputs'] = list()
    if(i > 1):
        update['inputs'].append("d"+str(i-1) + ":44")
    update['outputs'] = list()
    update['outputs'].append("d"+str(i) + ":44")
    update['thunk'] = "dupa"
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_data_10.json", "w"))


update_list = list()
for i in range(1, 1000):
    update = {}
    update['name'] = "/exec/f"+str(i)
    update['type'] = 0
    update['inputs'] = list()
    if(i > 1):
        update['inputs'].append("d"+str(i-1) + ":44")
    update['outputs'] = list()
    update['outputs'].append("d"+str(i) + ":44")
    update['thunk'] = "dupa"
    update['start'] = 0
    update['duration'] = 10
    print(update['name'])

    update_list.append(update)

json.dump(update_list, open("simple_data_1000.json", "w"))
