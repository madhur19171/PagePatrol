import networkx as nx

def generate_social_media_graph(num_nodes, filename="graph.txt"):
    """
    Generates a graph that resembles social media connections.
    The graph will have a power-law degree distribution.
    
    Parameters:
        num_nodes (int): Number of nodes in the graph.
        filename (str): File to save the graph's edge list.
    """
    # Ensure there are enough nodes
    if num_nodes < 2:
        raise ValueError("Number of nodes must be at least 2.")

    # Create a graph using a Barabási-Albert model
    # This model creates a scale-free network with a power-law degree distribution
    m = max(1, num_nodes // 10)  # Number of edges to attach from a new node to existing nodes
    graph = nx.barabasi_albert_graph(num_nodes, 1)

    # Save the graph to a file
    nx.write_edgelist(graph, filename, delimiter=" ", data=False)
    print(f"Graph saved to {filename}")

    # Optionally, visualize the graph
    # try:
    #     import matplotlib.pyplot as plt
    #     plt.figure(figsize=(10, 8))
    #     nx.draw(graph, node_size=20, node_color='blue', edge_color='gray', with_labels=False)
    #     plt.title("Generated Social Media Graph")
    #     plt.show()
    # except ImportError:
    #     print("Matplotlib is not installed. Skipping visualization.")

# Example usage
if __name__ == "__main__":
    num_nodes = 1_000
    generate_social_media_graph(num_nodes)
