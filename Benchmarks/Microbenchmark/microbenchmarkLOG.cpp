#include <algorithm>
#include <sys/resource.h>
#include <cctype>
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "../../API/PagePatrol.h"

#define PAGE_SIZE 4096

bool pagePatrolEnable = false;

std::ofstream logFile;
int logIndex = 0; // Initialize log index

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


// Base class for access patterns
class AccessPattern {
public:
    std::string name;
    virtual void execute(char* mapped_file, size_t num_pages) = 0;
    virtual ~AccessPattern() = default;
};

// Sequential access with gaps
class SequentialAccessWithGaps : public AccessPattern {
public:
    size_t gap;

    SequentialAccessWithGaps(size_t gap) : gap(gap) {
        name = "SequentialAccessWithGaps";
    }

    void execute(char* mapped_file, size_t num_pages) override {

        // we assume the disk is divided in 4 equal segments
        // so we start at the first quarter until the end
        size_t start_i = (num_pages / 4) + 1;

        for (size_t i = start_i; i < num_pages - 1; i += gap) {
            mapped_file[i * PAGE_SIZE] = (mapped_file[i * PAGE_SIZE] + 1) % 256;
        }
    }
};

// Random re-access of older pages
class RandomOlderPageReaccess : public AccessPattern {
public:
    size_t repeats;
    size_t range; // Percentage of file to access

    RandomOlderPageReaccess(size_t repeats, size_t range) : repeats(repeats), range(range) {
        name = "RandomOlderPageReaccess";
    }

    void execute(char* mapped_file, size_t num_pages) override {
        size_t access_limit = (num_pages * range) / 100;
        for (size_t i = 0; i < repeats; ++i) {
            size_t random_page = rand() % access_limit;
            mapped_file[random_page * PAGE_SIZE] = (mapped_file[random_page * PAGE_SIZE] + 1) % 256;
        }
    }
};

// Cyclic bursts on ranges of pages
class CyclicBurstsAccess : public AccessPattern {
public:
    size_t window;
    size_t stride;

    CyclicBurstsAccess(size_t window, size_t stride) : window(window), stride(stride) {
        name = "CyclicBurstsAccess";
    }

    void execute(char* mapped_file, size_t num_pages) override {
        for (size_t start = 0; start < num_pages; start += stride) {
            for (size_t i = start; i < start + window && i < num_pages; ++i) {
                mapped_file[i * PAGE_SIZE] = (mapped_file[i * PAGE_SIZE] + 1) % 256;

                if (pagePatrolEnable) {
                    mark_va_for_activation(&mapped_file[i * PAGE_SIZE]);
                }
            }
        }
    }
};

// Random access to a small portion of the file repeatedly
class SmallRegionRandomAccess : public AccessPattern {
public:
    size_t repeats;
    size_t region_size; // Percentage of file to access

    SmallRegionRandomAccess(size_t repeats, size_t region_size) : repeats(repeats), region_size(region_size) {
        name = "SmallRegionRandomAccess";
    }

    void execute(char* mapped_file, size_t num_pages) override {

        // again, we assume that the disk is divided in 4,
        // and here we only access the first quarter 
        size_t small_region_pages = num_pages / 4;

        for (size_t i = 0; i < repeats; ++i) {

            size_t random_page = rand() % small_region_pages;
            mapped_file[random_page * PAGE_SIZE] = (mapped_file[random_page * PAGE_SIZE] + 1) % 256;

            if (pagePatrolEnable) {
                pin_va(&mapped_file[random_page * PAGE_SIZE]);
            }
        }
    }
};

// Configuration class
struct Config {
    std::string file_name;
    size_t file_size;
    std::vector<std::string> execution_sequence;
    bool pagePatrol;
    int iterations;
    std::string pfLogFileName;
    size_t pattern1_gap = 64;
    size_t pattern2_repeats = 10000;
    size_t pattern2_range = 50;
    size_t pattern3_window = 256;
    size_t pattern3_stride = 1024;
    size_t pattern4_repeats = 10000;
    size_t pattern4_region_size = 1;
};

// Helper function to trim whitespace from the beginning and end of a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    size_t last = str.find_last_not_of(" \t");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

bool load_config(const std::string& config_file, Config& config) {
    std::ifstream infile(config_file);
    if (!infile) {
        std::cerr << "Error: Unable to open config file: " << config_file << std::endl;
        return false;
    }

    std::string line;
    std::string current_section;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip comments and empty lines

        std::istringstream iss(line);
        if (line[0] == '[' && line.back() == ']') {
            // New section
            current_section = line.substr(1, line.size() - 2);
        } else if (current_section.empty()) {
            // General configuration settings
            std::string key, equals;
            if (iss >> key >> equals && equals == "=") {
                std::string rest_of_line;
                std::getline(iss, rest_of_line);  // Get the remaining part of the line

                // Trim leading and trailing whitespace from the value
                std::string value = trim(rest_of_line);

                if (key == "file_name") {
                    config.file_name = value;
                } else if (key == "file_size") {
                    config.file_size = std::stoull(value);
                } else if (key == "execution_sequence") {
                    // Split the trimmed value into individual pattern names
                    std::istringstream sequence_stream(value);
                    std::string pattern_name;
                    while (sequence_stream >> pattern_name) {
                        config.execution_sequence.push_back(pattern_name);
                    }
                } else if (key == "PagePatrol") {
                    config.pagePatrol = std::stoull(value) == 1;
                } else if (key == "iterations") {
                    config.iterations = std::stoi (value);
                } else if (key == "PF_Log_File_Name") {
                    config.pfLogFileName = value;
                }
            }
        } else {
            // Pattern-specific settings
            std::string key, equals, value;
            if (iss >> key >> equals >> value && equals == "=") {
                value = trim(value);  // Trim the value

                if (current_section == "SequentialAccessWithGaps" && key == "gap") {
                    config.pattern1_gap = std::stoul(value);
                } else if (current_section == "RandomOlderPageReaccess") {
                    if (key == "repeats") config.pattern2_repeats = std::stoul(value);
                    else if (key == "range") config.pattern2_range = std::stoul(value);
                } else if (current_section == "CyclicBurstsAccess") {
                    if (key == "window") config.pattern3_window = std::stoul(value);
                    else if (key == "stride") config.pattern3_stride = std::stoul(value);
                } else if (current_section == "SmallRegionRandomAccess") {
                    if (key == "repeats") config.pattern4_repeats = std::stoul(value);
                    else if (key == "region_size") config.pattern4_region_size = std::stoul(value);
                }
            }
        }
    }

    infile.close();
    return true;
}



void print_config(const Config& config) {
    std::cout << "Configuration:" << std::endl;
    
    // General settings
    std::cout << "  File Name: " << config.file_name << std::endl;
    std::cout << "  File Size: " << config.file_size << " bytes" << std::endl;
    std::cout << "  Page Patrol: " << config.pagePatrol << std::endl;
    std::cout << "  Iterations: " << config.iterations << std::endl;
    std::cout << "  Page Fault Log File Name: " << config.pfLogFileName << std::endl;
    
    // Execution sequence
    std::cout << "  Execution Sequence: ";
    for (const auto& pattern : config.execution_sequence) {
        std::cout << pattern << " ";
    }
    std::cout << std::endl;

    // Parameters for each access pattern
    std::cout << "\nPattern Parameters:" << std::endl;

    std::cout << "  [SequentialAccessWithGaps]" << std::endl;
    std::cout << "    gap: " << config.pattern1_gap << std::endl;

    std::cout << "  [RandomOlderPageReaccess]" << std::endl;
    std::cout << "    repeats: " << config.pattern2_repeats << std::endl;
    std::cout << "    range: " << config.pattern2_range << " %" << std::endl;

    std::cout << "  [CyclicBurstsAccess]" << std::endl;
    std::cout << "    window: " << config.pattern3_window << std::endl;
    std::cout << "    stride: " << config.pattern3_stride << std::endl;

    std::cout << "  [SmallRegionRandomAccess]" << std::endl;
    std::cout << "    repeats: " << config.pattern4_repeats << std::endl;
    std::cout << "    region_size: " << config.pattern4_region_size << " %" << std::endl;
}


// Execute access patterns in the specified sequence
void run_access_patterns(char* mapped_file, size_t num_pages, const Config& config,
                         const std::unordered_map<std::string, AccessPattern*>& patterns_map) {
    for (int i = 0; i < config.iterations; i++)
    {
        for (const auto& pattern_name : config.execution_sequence) {
            auto it = patterns_map.find(pattern_name);  // Look up the pattern by name
            if (it != patterns_map.end()) {
                AccessPattern* pattern = it->second;
                std::cout << "Executing " << pattern->name << std::endl;
                pattern->execute(mapped_file, num_pages);  // Execute the pattern

                logPageFaults(pattern->name);
            } else {
                std::cerr << "Unknown pattern: " << pattern_name << std::endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    Config config;
    if (!load_config(argv[1], config)) {
        return 1;
    }
    
    print_config(config);

    pagePatrolEnable = config.pagePatrol != 0;

    if (pagePatrolEnable) {
        config.pfLogFileName += "_pp.log";  // Append _pp to page_fault log file if Page Patrol is enabled
    } else {
        config.pfLogFileName += ".log";
    }

    // Initialize the log file
    initializeLogFile(config.pfLogFileName);

    if (pagePatrolEnable) {
        if (init_page_patrol() == -1) {
            printf("Failed to initialize Page Patrol\n");
            return -1;
        } 
    }

    srand(time(nullptr));

    int fd = open(config.file_name.c_str(), O_RDWR);
    if (fd < 0) {
        perror("File open error");
        return 1;
    }

    char* mapped_file = (char*)mmap(nullptr, config.file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_file == MAP_FAILED) {
        perror("Memory map error");
        close(fd);
        return 1;
    }

    size_t num_pages = config.file_size / PAGE_SIZE;

    // Create instances of access patterns
    SequentialAccessWithGaps sequential_access(config.pattern1_gap);
    RandomOlderPageReaccess random_reaccess(config.pattern2_repeats, config.pattern2_range);
    CyclicBurstsAccess cyclic_bursts(config.pattern3_window, config.pattern3_stride);
    SmallRegionRandomAccess small_region_random(config.pattern4_repeats, config.pattern4_region_size);

    // Store pointers to the access patterns in a map
    std::unordered_map<std::string, AccessPattern*> patterns_map = {
        {sequential_access.name, &sequential_access},
        {random_reaccess.name, &random_reaccess},
        {cyclic_bursts.name, &cyclic_bursts},
        {small_region_random.name, &small_region_random}
    };

    // Run the access patterns based on the execution sequence
    run_access_patterns(mapped_file, num_pages, config, patterns_map);

    // Cleanup
    if (munmap(mapped_file, config.file_size) < 0) {
        perror("Memory unmap error");
    }
    close(fd);

    closeLogFile();

    return 0;
}
