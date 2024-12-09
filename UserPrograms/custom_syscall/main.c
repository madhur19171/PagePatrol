#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pagepatrol.h"

#define PAGE_SIZE 4096

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

size_t get_file_size(char *filename) {
  struct stat file_status;
  if (stat(filename, &file_status) < 0) {
    return -1;
  }

  return file_status.st_size;
}

int test1(int argc, char **argv) {
  size_t file_size = 1000000000;
  file_size = 256000 * PAGE_SIZE;
  // file_size = 524287 * PAGE_SIZE;
  int fd = open(argv[2], O_RDWR);
  if (fd < 0) {
    perror("File open error");
    return 1;
  }

  int ITER = atoi(argv[3]);
  is_mru = atoi(argv[4]);

  char *mapped_file =
      (char *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped_file == MAP_FAILED) {
    perror("Memory map error");
    close(fd);
    return 1;
  }

  size_t num_pages = file_size / PAGE_SIZE;

  pid_t pid = getpid();
  int ipid = pid;

  printf("is mru %d\n", is_mru);

  int yes = 0;
  int tot = 0;
  for (int iter = 0; iter < ITER; iter++) {
    for (size_t i = 0; i < num_pages; i++) {
      // posix_fadvise(fd, i * PAGE_SIZE, 1, POSIX_FADV_DONTNEED);
      mapped_file[i * PAGE_SIZE] = (mapped_file[i * PAGE_SIZE] + 1) % 256;
      int sret = read_pp(&mapped_file[i * PAGE_SIZE]);
      if (sret) {
        printf("error %d - %d\n", sret, sret == 0);
        yes++;
      }
      tot++;
    }
    // empty_list_pp();
  }

  printf("Result %d/%d\n", yes, tot);

  if (munmap(mapped_file, file_size) < 0) {
    perror("Memory unmap error");
  }
  close(fd);

  printf("Completed\n");
  return 0;
}

int test2(int argc, char **argv) {

  const int ITER = atoi(argv[2]);
  is_mru = atoi(argv[3]);
  const int num_files = 5;

  int meta_fd = open(argv[4], O_RDWR);
  if (meta_fd < 0) {
    perror("File open error");
    return 1;
  }
  size_t meta_file_size = get_file_size(argv[4]);
  char *meta_mapped_file = (char *)mmap(
      NULL, meta_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, meta_fd, 0);
  if (meta_mapped_file == MAP_FAILED) {
    perror("Memory map error");
    close(meta_fd);
    return 1;
  }
  size_t meta_num_pages = meta_file_size / PAGE_SIZE;

  int fds[num_files];
  size_t file_sizes[num_files];
  size_t num_pages[num_files];
  char *mapped_files[num_files];

  for (int i = 0; i < num_files; i++) {
    fds[i] = open(argv[i + 5], O_RDWR);
    if (fds[i] < 0) {
      perror("File open error");
      return 1;
    }
    file_sizes[i] = get_file_size(argv[i + 5]);

    mapped_files[i] = (char *)mmap(NULL, file_sizes[i], PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fds[i], 0);
    if (mapped_files[i] == MAP_FAILED) {
      perror("Memory map error");
      close(fds[i]);
      return 1;
    }
    num_pages[i] = file_sizes[i] / PAGE_SIZE;
  }
  // file_size = 524287 * PAGE_SIZE;

  pid_t pid = getpid();
  int ipid = pid;

  printf("is mru %d\n", is_mru);
  printf("Meta pages %ld\n", meta_num_pages);

  /* Pre processing */
  if (is_mru) {
    for (size_t i = 0; i < meta_num_pages; i++) {
      pin_pp(&meta_mapped_file[i * PAGE_SIZE]);
    }
    fsync(meta_fd);
    for (size_t i = 0; i < num_files / 2; i++) {
      for (size_t j = 0; j < num_pages[i]; j++)
        get_pp(&mapped_files[i][j * PAGE_SIZE]);
      fsync(fds[i]);
    }
  }
  for (size_t i = 0; i < meta_num_pages; i++) {
    meta_mapped_file[i * PAGE_SIZE] =
        (meta_mapped_file[i * PAGE_SIZE] + 1) % 256;
  }

  /* Heavy lifting */
  for (int iter = 0; iter < ITER; iter++) {
    printf("Iter %d started\n", iter);
    for (size_t f = 0; f < num_files; f++) {
      printf("File %ld\n", f);
      size_t s = rand() % meta_num_pages;
      // size_t s_e = MIN(s + (rand() % PAGE_SIZE), meta_num_pages);
      size_t s_e = MIN(s + (PAGE_SIZE), meta_num_pages);
      for (size_t i = s; i < s_e; i++) {
        meta_mapped_file[i * PAGE_SIZE] =
            (meta_mapped_file[i * PAGE_SIZE] + 1) % 256;
      }
      for (size_t i = 0; i < num_pages[f]; i++) {
        mapped_files[f][i * PAGE_SIZE] =
            (mapped_files[f][i * PAGE_SIZE] + 1) % 256;
      }
      // evict_list_pp();
    }
    printf("Iter %d completed\n", iter);
  }

  if (is_mru) {
    unpin_list_pp();
    // for (size_t i = 0; i < meta_num_pages; i++) {
    //   unpin_pp(&meta_mapped_file[i * PAGE_SIZE]);
    // }
  }

  if (munmap(meta_mapped_file, meta_file_size) < 0) {
    perror("Memory unmap error");
  }
  close(meta_fd);
  for (size_t f = 0; f < num_files; f++) {
    if (munmap(mapped_files[f], file_sizes[f]) < 0) {
      perror("Memory unmap error");
    }
    close(fds[f]);
  }

  printf("Completed\n");
  return 0;
}

/*
 * args:
 * 2: n.iter
 * 3: is mru
 * 4: n. meta files (x)
 * 5: n. regular files (y)
 * 6..6+x: path of meta files
 * (6+x)+1..(6+x)+1+y: path of regular files
 */
int test3(int argc, char **argv) {

  const int ITER = atoi(argv[2]);
  is_mru = atoi(argv[3]);
  // const int meta_num_files = 2;
  // const int num_files = 5;
  const int meta_num_files = atoi(argv[4]);
  const int num_files = atoi(argv[5]);

  printf("META FILES %d\nNUM FILES %d\n", meta_num_files, num_files);

  int meta_fds[meta_num_files];
  size_t meta_file_sizes[meta_num_files];
  size_t meta_num_pages[meta_num_files];
  char *meta_mapped_files[meta_num_files];

  for (int i = 0; i < meta_num_files; i++) {
    int idx = i + 6;
    meta_fds[i] = open(argv[idx], O_RDWR);
    printf("Meta at %s\n", argv[idx]);
    if (meta_fds[i] < 0) {
      printf("Error at meta %s\n", argv[idx]);
      perror("File open error");
      return 1;
    }
    meta_file_sizes[i] = get_file_size(argv[idx]);
    meta_mapped_files[i] =
        (char *)mmap(NULL, meta_file_sizes[i], PROT_READ | PROT_WRITE,
                     MAP_SHARED, meta_fds[i], 0);
    if (meta_mapped_files[i] == MAP_FAILED) {
      perror("Memory map error");
      close(meta_fds[i]);
      return 1;
    }
    meta_num_pages[i] = meta_file_sizes[i] / PAGE_SIZE;
  }

  int fds[num_files];
  size_t file_sizes[num_files];
  size_t num_pages[num_files];
  char *mapped_files[num_files];

  for (int i = 0; i < num_files; i++) {
    int idx = i + 6 + meta_num_files;
    printf("File at %s\n", argv[idx]);
    fds[i] = open(argv[idx], O_RDWR);
    if (fds[i] < 0) {
      printf("Error at %s\n", argv[idx]);
      perror("File open error");
      return 1;
    }
    file_sizes[i] = get_file_size(argv[idx]);

    mapped_files[i] = (char *)mmap(NULL, file_sizes[i], PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fds[i], 0);
    if (mapped_files[i] == MAP_FAILED) {
      perror("Memory map error");
      close(fds[i]);
      return 1;
    }
    num_pages[i] = file_sizes[i] / PAGE_SIZE;
  }
  // file_size = 524287 * PAGE_SIZE;

  pid_t pid = getpid();
  int ipid = pid;

  printf("is mru %d\n", is_mru);

  /* Pre processing */
  if (is_mru) {
    for (int f = 0; f < meta_num_files; f++) {
      for (size_t i = 0; i < meta_num_pages[f]; i++) {
        pin_pp(&meta_mapped_files[f][i * PAGE_SIZE]);
        meta_mapped_files[f][i * PAGE_SIZE] =
            (meta_mapped_files[f][i * PAGE_SIZE] + 1) % 256;
      }
      fsync(meta_fds[f]);
    }
    for (size_t i = 0; i < num_files / 2; i++) {
      for (size_t j = 0; j < num_pages[i]; j++)
        get_pp(&mapped_files[i][j * PAGE_SIZE]);
      fsync(fds[i]);
    }
  }

  /* Heavy lifting */
  int meta_idx = 0;
  for (int iter = 0; iter < ITER; iter++) {
    printf("Iter %d started\n", iter);
    for (size_t f = 0; f < num_files; f++) {
      size_t s = rand() % meta_num_pages[meta_idx];
      // size_t s_e = MIN(s + (rand() % PAGE_SIZE), meta_num_pages);
      size_t s_e = MIN(s + (PAGE_SIZE), meta_num_pages[meta_idx]);
      for (size_t i = s; i < s_e; i++) {
        meta_mapped_files[meta_idx][i * PAGE_SIZE] =
            (meta_mapped_files[meta_idx][i * PAGE_SIZE] + 1) % 256;
      }
      meta_idx = (meta_idx + 1) % meta_num_files;
      for (size_t i = 0; i < num_pages[f]; i++) {
        mapped_files[f][i * PAGE_SIZE] =
            (mapped_files[f][i * PAGE_SIZE] + 1) % 256;
      }
      // evict_list_pp();
    }
    printf("Iter %d completed\n", iter);
  }

  if (is_mru) {
    unpin_list_pp();
    // for (size_t i = 0; i < meta_num_pages; i++) {
    //   unpin_pp(&meta_mapped_file[i * PAGE_SIZE]);
    // }
  }

  for (size_t f = 0; f < meta_num_files; f++) {
    if (munmap(meta_mapped_files[f], meta_file_sizes[f]) < 0) {
      perror("Memory unmap error");
    }
    close(meta_fds[f]);
  }
  for (size_t f = 0; f < num_files; f++) {
    if (munmap(mapped_files[f], file_sizes[f]) < 0) {
      perror("Memory unmap error");
    }
    close(fds[f]);
  }

  printf("Completed\n");
  return 0;
}

/*
 * pinning more than available RAM ( >= 1GB)
 */
int test4(int argc, char **argv) {
  size_t file_size = 1000000000;
  file_size = 256000 * PAGE_SIZE;
  // file_size = 524287 * PAGE_SIZE;
  int fd = open(argv[2], O_RDWR);
  if (fd < 0) {
    perror("File open error");
    return 1;
  }

  int ITER = atoi(argv[3]);
  is_mru = atoi(argv[4]);

  char *mapped_file =
      (char *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapped_file == MAP_FAILED) {
    perror("Memory map error");
    close(fd);
    return 1;
  }

  size_t num_pages = file_size / PAGE_SIZE;

  pid_t pid = getpid();
  int ipid = pid;

  printf("is mru %d\n", is_mru);

  for (size_t i = 0; i < num_pages / 3 * 2; i++) {
    pin_pp(&mapped_file[i * PAGE_SIZE]);
  }

  printf("Everything is pinned\n");

  for (int iter = 0; iter < ITER; iter++) {
    for (size_t i = 0; i < num_pages; i++) {
      mapped_file[i * PAGE_SIZE] = (mapped_file[i * PAGE_SIZE] + 1) % 256;
    }
  }

  unpin_list_pp();
  printf("Everything is unpinned\n");

  if (munmap(mapped_file, file_size) < 0) {
    perror("Memory unmap error");
  }
  close(fd);

  printf("Completed\n");
  return 0;
}

int main(int argc, char **argv) {

  switch (atoi(argv[1])) {
  case 1:
    return test1(argc, argv);
  case 2:
    return test2(argc, argv);
  case 3:
    return test3(argc, argv);
  case 4:
    return test4(argc, argv);
  }

  printf("test not found\n");

  return 0;
}
