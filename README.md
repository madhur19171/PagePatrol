# PagePatrol
A user-driven page cache policy management on Linux using eBPF.

---

## Overview
PagePatrol allows users to fine-tune page age caching policies on their Linux systems, optimizing memory management and potentially improving system performance.

---

## Setting Up the Linux Kernel

### Cloning the repo
The PagePatrol repo contains the linux fork for PagePatrol. Clone the PagePatrol repo with recursive flag to clone the linux repo as well. As the debian image has a limited size, make usre to use depth parameter to avoid cloning all the commits.

### Configuration

1. **Use Default Configuration:**
    ```bash
    make olddefconfig
    ```
   This command applies the kernel’s default configuration based on the current setup.

2. **Build Only Active Modules:**
    ```bash
    make localmodconfig
    ```
   This builds only the currently used modules, significantly reducing compile time.

3. **Enable BTF Support:**
   To add BTF (BPF Type Format) support to the Linux kernel, open the `.config` file and add the following line:
    ```bash
    CONFIG_DEBUG_INFO_BTF=y
    ```

### Building the Kernel
To build the kernel with a custom local version suffix, use:
   ```bash
   make -j LOCALVERSION=-<name>
   ```
Replace `<name>` with your desired identifier to append it to the kernel name.

### Installing the Kernel

Run the following commands with root privileges to install the modules, headers, and the new kernel:

1. **Install Modules:**
    ```bash
    sudo make modules_install
    ```

2. **Install Headers:**
    ```bash
    sudo make headers_install INSTALL_HDR_PATH=/usr
    ```

3. **Install Kernel:**
    ```bash
    sudo make install
    ```

---

## Mounting NVMe

For instructions on mounting NVMe, please refer to [this guide](https://gist.github.com/a-maumau/b826164698da318f992aad5498d0d934).

---

# Installing QEMU

It is convenient to test custom kernel on 
To install QEMU, follow the installation instructions on [QEMU's official website](https://www.qemu.org/download/).

## Configuration

After downloading QEMU, use the following configuration command:

```bash
../configure --target-list=x86_64-softmmu --enable-debug --enable-slirp
```

This configuration:
- Targets the `x86_64` architecture with soft MMU (memory management unit).
- Enables debugging features.
- Enables `slirp` networking support for easier networking configuration.

---

# Microbenchmark Usage Guide

This guide provides instructions on how to use the `microbenchmark` program and monitor its page fault behavior.

## Prerequisites

- Ensure that the `microbenchmark` binary is compiled and located in the same directory as this script.
- The configuration file (`config.cfg`) should be set up correctly with the necessary parameters.

## Running the Microbenchmark

The `run.sh` script runs the `microbenchmark` program after clearing the page cache. This ensures that each run of the benchmark starts with a clean page cache state.

### `run.sh` Script

```bash
#!/bin/bash

sudo sync    # Write back data to disk
echo 3 | sudo tee /proc/sys/vm/drop_caches      # Clear the Page Cache

./microbenchmark
```

The Microbenchmark directory contains the microbenchmark, and some scripts.
The Microbenchmark can be compiled using the following command:
```bash
g++ -o microbenchmark microbenchmark.cpp -lbpf
```
As the microbenchmark uses eBP maps, -lbpf is necessary to be passed. 

### Configuration
The config.cfg file contains the configuration for running the microbenchmark. It defines the name of the file to use for processing ("bigfile"), the size of the file, access pattern and configurations for the access patterns

### vmtouch.sh
vmtouch is a program that can be used to access and monitor files. it can touch file and evict file pages. It can also show what sections of a file are cached in page cache and what sections are evicted. This script provides a neat way of visualising the memory residency of the file over time. 

## Running and Compiling eBPF

```bash
ecc <program name>          # Compile
sudo ecli run package.json  # Run
sudo cat /sys/kernel/debug/tracing/trace_pipe   # Output of trace
```

### eBPF API
Page Patrol's API uses the eBPF program inside the folder eBPF/insert_file_vaddr_into_inactive_list_kfunc/. It can be compiled using ```ecc insert_file_vaddr_into_inactive_list.bpf.c``` and loaded with ```sudo ecli package.json```

## Creating a bigfile

The microbenchmark mmaps a 1GB file named **bigfile**
The run.sh and vmtouch.sh scripts expect the bigfile to be present in the Benchmark/Microbenchmark directory
You can use the following command in Microbenchmark directory to create a big file
```bash
dd if=/dev/urandom of=bigfile bs=1M count=1K
```