#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define FILE_SIZE (612 * 1024 * 1024) // 0.5GB

int main() {
    const char *filename = "/mnt/NVMe/largeFile2";
    int fd;
    void *map;

    // Create or open the file
    fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ensure the file is at least 0.5GB in size
    if (ftruncate(fd, FILE_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Memory map the file
    map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Lock the memory to prevent it from being swapped out
    if (mlock(map, FILE_SIZE) == -1) {
        perror("mlock");
        munmap(map, FILE_SIZE);
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("File successfully memory-mapped and locked in memory.\n");

    // Perform any operations on the mapped memory here
 	sleep(100000000);

    // Unlock the memory
    if (munlock(map, FILE_SIZE) == -1) {
        perror("munlock");
    }

    // Unmap the file
    if (munmap(map, FILE_SIZE) == -1) {
        perror("munmap");
    }

    // Close the file
    close(fd);

    return 0;
}

