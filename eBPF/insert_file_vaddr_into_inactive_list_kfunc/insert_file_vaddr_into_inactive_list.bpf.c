#define BPF_NO_GLOBAL_DATA
// #include <linux/bpf.h>
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// Map for input: user-space process provides the virtual address
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1);
	__type(key, __u32); // Should be 0
	__type(value, int); // PID
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} pid_map SEC(".maps");

// Map for input: user-space process provides the virtual address
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1);
	__type(key, __u32); // Should be 0
	__type(value, unsigned long); // Virtual Address (VA)
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} va_map SEC(".maps");

// Declare the kfunc
extern int bpf_insert_file_vaddr_into_inactive_list(int pid, unsigned long vaddr) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/do_unlinkat")
int BPF_KPROBE(do_unlinkat, int dfd, struct filename *name) {
	unsigned int key = 0;

	int *pid;
	unsigned long *va;

	pid = bpf_map_lookup_elem(&pid_map, &key);
	if (!pid) {
		bpf_printk("Failed to find PID %d\n", *pid);
		return 0;
	}

	// Retrieve the virtual address (VA) from the input map
	va = bpf_map_lookup_elem(&va_map, &key);
	if (!va) {
		bpf_printk("Failed to find VA 0x%lx\n", *va);
		return 0;
	}

	// Call the kfunc to translate VA to PA
	if (bpf_insert_file_vaddr_into_inactive_list(*pid, *va) == 0) {
		bpf_printk("Inserted VA: %lx to inactive list for PID: %d\n", *va, pid);
	} else {
		bpf_printk("Failed to insert VA: %lx to inactive list for PID: %d\n", *va, pid);
	}

	return 0;
}
