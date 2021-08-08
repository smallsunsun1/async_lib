import argparse
import pygraphviz as pgv
from tools.visualize import graph_pb2
from google.protobuf import text_format
from google.protobuf.json_format import MessageToDict


def visualize_graph(async_graph: graph_pb2.AsyncGraphDef, output_filename: str):
    '''
    Visualize AsyncGraphDef, Save to a file
    @params:
        graph_def: 输入的AsyncGraphDef
    @returns:
        Node
    '''
    graph_info_dict = MessageToDict(async_graph)
    g = pgv.AGraph(fontname="Times-Roman", fontsize=14, directed=True,
                   shape="ellipse", style="rounded", color="black", fontcolor="black",
                   pos="x,y(!)", fixedsize=False, width=1920, height=1080)
    nodes = graph_info_dict["nodes"]
    nodes_size = len(nodes)
    node_name_set = {}
    for i in range(nodes_size):
        if (nodes[i]["funcName"]) not in node_name_set:
            node_name_set[nodes[i]["funcName"]] = 0
        else:
            node_name_set[nodes[i]["funcName"]] += 1
        nodes[i]["funcName"] = nodes[i]["funcName"] + \
            ":{}".format(node_name_set[nodes[i]["funcName"]])
        g.add_node(nodes[i]["funcName"])
    edge_node_map = {}
    node_edge_map = {}
    for i in range(nodes_size):
        node_edge_map[nodes[i]["funcName"]] = []
        input_names = nodes[i].get("inputNames", [])
        for j in range(len(input_names)):
            input_name = input_names[j]
            if edge_node_map.get(input_name, None) is None:
                edge_node_map[input_name] = []
                edge_node_map[input_name].append(nodes[i]["funcName"])
            else:
                edge_node_map[input_name].append(nodes[i]["funcName"])
        output_names = nodes[i].get("outputNames", [])
        for j in range(len(output_names)):
            output_name = output_names[j]
            node_edge_map[nodes[i]["funcName"]].append(output_name)
    for node, edges in node_edge_map.items():
        for edge in edges:
            if edge not in edge_node_map:
                g.add_edge(node, "end", label=edge)
                continue
            node2_list = edge_node_map[edge]
            for node2 in node2_list:
                g.add_edge(node, node2, label=edge)

    g.layout()
    g.draw(output_filename, prog="dot")


def load_graph_from_pb_file(filename: str, mode: int, output_filename: str):
    '''
    Load Graph From Protobuf
    @params:
        filename: 输入Pb文件名,"xxx/xxx/xxx.pb.txt",
        mode: 0表示pb文本格式，1表示pb二进制格式
    '''
    graph_def = graph_pb2.AsyncGraphDef()
    if mode == 0:
        data = open(filename, "r").read()
        text_format.Parse(data, graph_def)
    elif mode == 1:
        data = open(filename, "rb").read()
        graph_def.ParseFromString(data)
    visualize_graph(graph_def, output_filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--filename", default="", type=str,
                        help="input proto filename")
    parser.add_argument("--mode", default=0, type=int, help="load graph mode")
    parser.add_argument("--output_filename", default="async_graph.png",
                        type=str, help="output saved graph")
    args = parser.parse_args()
    load_graph_from_pb_file(args.filename, args.mode, args.output_filename)
