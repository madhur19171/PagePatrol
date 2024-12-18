import matplotlib.pyplot as plt
import networkx as nx

def load_edge_list(file_path):
    """
    Loads an edge list from a text file.

    Parameters:
        file_path (str): Path to the text file containing the edge list.

    Returns:
        list of tuples: List of edges, where each edge is a tuple (node1, node2).
    """
    with open(file_path, 'r') as file:
        edges = [tuple(line.strip().split()) for line in file if line.strip()]
    return edges

def visualize_edge_list(edge_list, output_file=None):
    """
    Visualizes a graph from an edge list.

    Parameters:
        edge_list (list of tuples): List of edges, where each edge is a tuple (node1, node2).
        output_file (str, optional): Path to save the visualization as a file. If None, the graph is shown on screen.
    """
    # Create a graph object
    G = nx.Graph()

    # Add edges to the graph
    G.add_edges_from(edge_list)

    # Draw the graph
    plt.figure(figsize=(8, 6))
    nx.draw(G, node_size=20, node_color='blue', edge_color='gray', with_labels=False)
    
    # Save or show the graph
    if output_file:
        plt.savefig(output_file, format='png', dpi=300, bbox_inches='tight')
        print(f"Graph saved to {output_file}")
    else:
        plt.show()

if __name__ == "__main__":
    # Load edge list from a file
    edge_list_file = "graph50Final.txt"  # Path to your edge list file
    edges = load_edge_list(edge_list_file)

    # Visualize the graph
    visualize_edge_list(edges, output_file="graph_visualization.png")
