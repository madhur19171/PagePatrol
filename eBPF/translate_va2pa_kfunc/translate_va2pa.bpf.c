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

// Map for output: the physical address returned by the kfunc
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1);
	__type(key, __u32); // PID
	__type(value, unsigned long); // Physical Address (PA)
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} pa_map SEC(".maps");

// Result structure used with the kfunc
struct va_to_pa_result {
	unsigned long physical_address;
	int error_code;
};

// Declare the kfunc
extern int bpf_translate_va_to_pa(int pid, unsigned long vaddr, struct va_to_pa_result *result) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/do_unlinkat")
int BPF_KPROBE(do_unlinkat, int dfd, struct filename *name) {
	unsigned int key = 0;

	int *pid;
	unsigned long *va;
	unsigned long pa;
	struct va_to_pa_result result = {};

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
	if (bpf_translate_va_to_pa(*pid, *va, &result) == 0 && result.error_code == 0) {
		pa = result.physical_address;
		// Store the physical address (PA) in the output map
		bpf_map_update_elem(&pa_map, &key, &pa, BPF_ANY);
		bpf_printk("Translated VA: %lx to PA: %lx for PID: %d\n", *va, pa, pid);
	} else {
		bpf_printk("Failed to translate VA: %lx for PID: %d, error: %d\n", *va, pid, result.error_code);
	}

	return 0;
}
