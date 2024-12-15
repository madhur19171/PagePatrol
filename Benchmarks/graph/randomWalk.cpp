#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../API/PagePatrol.h"
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <sstream>
#include <string>

#define PAGE_PATROL

// Define the maximum number of nodes (adjust as needed)
#define NUM_NODES 50
#define NUM_BLOCKS_PER_NODE 2500
#define BLOCK_SIZE 4096

#define STEPS 5000

std::ofstream logFile;
int logIndex = 0; // Initialize log index
size_t currentIter = 0;

// Function to initialize the log file
void initializeLogFile(const std::string &fileName) {
    // Open the file in write mode to initialize it
    logFile.open(fileName, std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << fileName << std::endl;
        exit(1);
    }

    // Write the header line to the log file
    logFile << "Index,Message,Minor Page Faults,Major Page Faults" << std::endl;
}

// Function to log the page faults to the file
void logPageFaults(const std::string &message) {
    if (!logFile.is_open()) {
        std::cerr << "Log file is not open. Did you call initializeLogFile()?" << std::endl;
        return;
    }

    // Get the resource usage for the calling process
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        std::cerr << "Failed to get resource usage." << std::endl;
        return;
    }

    // Log the data to the file
    logFile << logIndex++ << "," << message << "," << usage.ru_minflt << "," << usage.ru_majflt << std::endl;
}

// Don't forget to close the log file when done
void closeLogFile() {
    if (logFile.is_open()) {
        logFile.close();
    }
}


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

    srand(655);  // Seed the random number generator

    unsigned long currentNode = startNode;

    for (int i = 0; i < steps; ++i) {
        if (currentNode < 0 || currentNode >= NUM_NODES) {
            printf("FUCK\n");
        }

        if (i % 100 == 0) {
            logPageFaults("hqldsfj");
        }

        printf("Step %d: Node %d\n", i + 1, currentNode);

        // simulating disk access for that node
#ifdef PAGE_PATROL
        mark_access(&data[currentNode * NUM_BLOCKS_PER_NODE * BLOCK_SIZE]);
#endif
        for (int index = currentNode * NUM_BLOCKS_PER_NODE * BLOCK_SIZE; index < (currentNode+1) * NUM_BLOCKS_PER_NODE * BLOCK_SIZE; index += BLOCK_SIZE) {
            data[index] = (data[index] + 1) % 256;
        }
        //printf("%d\n", data[BLOCK_SIZE * (currentNode)]);

        // Get the neighbors of the current node
        AdjListNode* neighbor = graph->array[currentNode].head;
        if (!neighbor) {
            printf("Node %ld has no neighbors. Ending walk.\n", currentNode);
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
    void *mapped = mmap(NULL, *file_size, PROT_READ | PROT_WRITE,  MAP_SHARED, fd, 0);
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
    const char* graphFileName = "./graph50Final.txt";
    int numNodes;
    Graph* graph = readEdgeList(graphFileName, &numNodes);
    printf("num nodes in edge list %d\n", numNodes);

    // Open the content file
    const char *contentFileName = "/mnt/NVMe/bigfile1";
    size_t file_size;
    void* mapped = readNodeContent(contentFileName, &file_size);
    if (mapped == NULL) {
        perror("Could not map file content");
        return EXIT_FAILURE;
    }

    if (numNodes * NUM_BLOCKS_PER_NODE * BLOCK_SIZE > file_size) {
        perror("Not enough node content");
        return EXIT_FAILURE;
    }

#ifdef PAGE_PATROL
        initializeLogFile("page_faults_pp.log");
#else
        initializeLogFile("page_faults.log");
#endif

    int startNode = 0;
    int steps = STEPS;
    randomWalk(graph, startNode, steps, (char*) mapped);

    // program finished executing

    freeGraph(graph);

    if (munmap(mapped, file_size) == -1) {
        perror("Error unmapping the file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
