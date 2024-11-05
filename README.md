# PagePatrol
A user-driven page cache policy management on Linux using eBPF.

---

## Overview
PagePatrol allows users to fine-tune page age caching policies on their Linux systems, optimizing memory management and potentially improving system performance.

---

## Setting Up the Linux Kernel

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