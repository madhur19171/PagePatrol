#include <stdint.h>
#include <stdio.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <unistd.h>

#define PID_MAP "/sys/fs/bpf/pid_map"
#define VA_MAP  "/sys/fs/bpf/va_map"
#define PA_MAP  "/sys/fs/bpf/pa_map"

int main() {
	__u32 key = 0;

	int pid = getpid();
	unsigned long va, pa;
	int pid_map_fd, va_map_fd, pa_map_fd;

	// Open the maps
	pid_map_fd = bpf_obj_get(PID_MAP);
	if (pid_map_fd < 0) {
		perror("Failed to open PID map");
		return 1;
	}

	va_map_fd = bpf_obj_get(VA_MAP);
	if (va_map_fd < 0) {
		perror("Failed to open VA map");
		return 1;
	}

	pa_map_fd = bpf_obj_get(PA_MAP);
	if (pa_map_fd < 0) {
		perror("Failed to open PA map");
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

	// Retrieve the physical address from the output map
	if (bpf_map_lookup_elem(pa_map_fd, &key, &pa) == 0) {
		printf("Translated physical address: %lx\n", pa);
	} else {
		printf("Failed to retrieve PA.\n");
	}

	bpf_map_delete_elem(pid_map_fd, &key);
	bpf_map_delete_elem(va_map_fd, &key);
	bpf_map_delete_elem(pa_map_fd, &key);

	return 0;
}
