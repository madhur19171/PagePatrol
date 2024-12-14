#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../API/PagePatrol.h"

#define PAGE_PATROL

// Define the maximum number of nodes (adjust as needed)
#define NUM_NODES 1000
#define BLOCK_SIZE 4096

// Adjacency list node
typedef struct AdjListNode {
    int dest;
    struct AdjListNode* next;
} AdjListNode;

// Adjacency list
typedef struct AdjList {
    AdjListNode* head;
} AdjList;

// Graph structure
typedef struct Graph {
    int numNodes;
    AdjList* array;
} Graph;

// Function to create a new adjacency list node
AdjListNode* createAdjListNode(int dest) {
    AdjListNode* newNode = (AdjListNode*)malloc(sizeof(AdjListNode));
    newNode->dest = dest;
    newNode->next = NULL;
    return newNode;
}

// Function to create a graph
Graph* createGraph(int numNodes) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numNodes = numNodes;
    graph->array = (AdjList*)malloc(numNodes * sizeof(AdjList));
    for (int i = 0; i < numNodes; ++i) {
        graph->array[i].head = NULL;
    }
    return graph;
}

// Function to add an edge to the graph
void addEdge(Graph* graph, int src, int dest) {
    // Add the edge src -> dest
    AdjListNode* newNode = createAdjListNode(dest);
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;

    // Add the edge dest -> src (undirected graph)
    newNode = createAdjListNode(src);
    newNode->next = graph->array[dest].head;
    graph->array[dest].head = newNode;
}

// Function to read the edge list from a file
Graph* readEdgeList(const char* graphFileName, int* numNodes) {
    FILE* file = fopen(graphFileName, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", graphFileName);
        exit(EXIT_FAILURE);
    }

    int src, dest, maxNode = 0;
    Graph* graph = createGraph(NUM_NODES);

    // Read the edges
    while (fscanf(file, "%d %d", &src, &dest) == 2) {
        addEdge(graph, src, dest);
        if (src > maxNode) maxNode = src;
        if (dest > maxNode) maxNode = dest;
    }

    fclose(file);
    *numNodes = maxNode + 1;
    return graph;
}

// Perform a random walk on the graph
void randomWalk(Graph* graph, int startNode, int steps, char* data) {

    srand(time(NULL));  // Seed the random number generator

    int currentNode = startNode;

    for (int i = 0; i < steps; ++i) {

        printf("Step %d: Node %d\n", i + 1, currentNode);

#ifdef PAGE_PATROL
        access_va(&data[BLOCK_SIZE * currentNode]);
#endif
        // simulating disk access for that node
        printf("content node: %d\n", data[BLOCK_SIZE * currentNode]);

        // Get the neighbors of the current node
        AdjListNode* neighbor = graph->array[currentNode].head;
        if (!neighbor) {
            printf("Node %d has no neighbors. Ending walk.\n", currentNode);
            break;
        }

        // Count neighbors and pick a random one
        int neighborCount = 0;
        AdjListNode* temp = neighbor;
        while (temp) {
            ++neighborCount;
            temp = temp->next;
        }

        int randomIndex = rand() % neighborCount;
        temp = neighbor;
        for (int j = 0; j < randomIndex; ++j) {
            temp = temp->next;
        }

        currentNode = temp->dest;
    }
}

// Free memory allocated for the graph
void freeGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; ++i) {
        AdjListNode* temp = graph->array[i].head;
        while (temp) {
            AdjListNode* next = temp->next;
            free(temp);
            temp = next;
        }
    }
    free(graph->array);
    free(graph);
}

void *readNodeContent(const char* contentFileName, size_t *file_size) {

    int fd = open(contentFileName, O_RDWR);
    if (fd == -1) {
        perror("Error opening the file");
        return NULL;
    }

    // Get the size of the file
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file stats");
        close(fd);
        return NULL;
    }

    *file_size = file_stat.st_size;

    // Memory map the file
    void *mapped = mmap(NULL, *file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error memory mapping the file");
        close(fd);
        return NULL;
    }

    // Close the file descriptor as it is no longer needed
    close(fd);

    return mapped;
}

// Main function
int main() {

#ifdef PAGE_PATROL
    if (init_page_patrol() == -1) {
        printf("Failed to initialize Page Patrol\n");
        return -1;
    } 
#endif

    // open the graph file
    const char* graphFileName = "graph.txt";
    int numNodes;
    Graph* graph = readEdgeList(graphFileName, &numNodes);

    // Open the content file
    const char *contentFileName = "largeFile20";
    size_t file_size;
    void* mapped = readNodeContent(contentFileName, &file_size);
    if (mapped == NULL) {
        perror("Could not map file content");
        return EXIT_FAILURE;
    }

    if (numNodes * BLOCK_SIZE > file_size) {
        perror("Not enough node content");
        return EXIT_FAILURE;
    }

    int startNode = 0;
    int steps = 100000;
    randomWalk(graph, startNode, steps, mapped);

    // program finished executing

    freeGraph(graph);

    if (munmap(mapped, file_size) == -1) {
        perror("Error unmapping the file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
