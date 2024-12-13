import networkx as nx
import matplotlib.pyplot as plt

def plot_degree_distribution(edge_list_file):
    """
    Reads an edge list file and plots the degree distribution of the graph.

    Parameters:
        edge_list_file (str): Path to the edge list file.
    """
    try:
        # Load the graph from the edge list file
        graph = nx.read_edgelist(edge_list_file, nodetype=int)

        # Compute the degree for each node
        degrees = [degree for _, degree in graph.degree()]

        # Plot the degree distribution
        plt.figure(figsize=(10, 6))
        plt.hist(degrees, bins=range(1, max(degrees) + 2), color='blue', edgecolor='black', alpha=0.7)
        plt.title("Degree Distribution")
        plt.xlabel("Number of Edges (Degree)")
        plt.ylabel("Number of Nodes")
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.show()

    except FileNotFoundError:
        print(f"Error: The file '{edge_list_file}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Example usage
if __name__ == "__main__":
    edge_list_file = "graph.txt"
    plot_degree_distribution(edge_list_file)
