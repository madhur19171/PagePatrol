#include <stdint.h>
#include <stdio.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <unistd.h>

#define PID_VA_MAP "/sys/fs/bpf/pid_va_map"

int main() {
	__u32 key = 0;

	int pid = getpid();
	unsigned long va;
	int pid_va_map_fd;

	// Open the map
	pid_va_map_fd = bpf_obj_get(PID_VA_MAP);
	if (pid_va_map_fd < 0) {
		perror("Failed to open PID VA map");
		return 1;
	}

	va = (unsigned long) (&va_map_fd);

	// Insert the PID into the input map
	if (bpf_map_update_elem(pid_map_fd, &key, &pid, BPF_ANY) < 0) {
		perror("Failed to update PID map");
		return 1;
	}
	printf("PID %d\n", pid);

	// Insert the virtual address into the input map
	if (bpf_map_update_elem(va_map_fd, &key, &va, BPF_ANY) < 0) {
		perror("Failed to update VA map");
		return 1;
	}
	printf("Virtual address %lx inserted\n", va);

	// Wait for the eBPF program to process
	sleep(10);

	return 0;
}
