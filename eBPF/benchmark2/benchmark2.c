#define BPF_NO_GLOBAL_DATA
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "../../API/common.h"

// Map for input: user-space process provides the virtual address
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_PID_VA_ENTRIES);
	__type(key, __u32);
	__type(value, struct pid_va);
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} pid_va_map SEC(".maps");

// Declare the kfunc
extern int bpf_deactivate_file_vaddr(int pid, unsigned long vaddr) __ksym;
extern int bpf_activate_file_vaddr(int pid, unsigned long vaddr) __ksym;
extern int bpf_pin_file_vaddr(int pid, unsigned long vaddr) __ksym;
extern int bpf_unpin_file_vaddr(int pid, unsigned long vaddr) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/pid_nr_ns")
int BPF_KPROBE(pid_nr_ns, struct pid *pid, struct pid_namespace *ns) {
	struct pid_va *pid_va;

	// first get the index of the process
	int zero = 0;
	pid_va = bpf_map_lookup_elem(&pid_va_map, &zero);
	if (!pid_va) {
		return -1;
	}
	int idxProcess = pid_va->flags; // that is where we store the index

	// now iterate over the API calls
	int idx = idxProcess;

	for (int times = 1; times < MAX_PID_VA_ENTRIES; times++) {

		pid_va = bpf_map_lookup_elem(&pid_va_map, &idx);

		if (pid_va) {
			if (pid_va->flags == 0) {
				// bpf_printk("Receied endpoint evict\n");
				if (bpf_deactivate_file_vaddr(pid_va->PID, pid_va->VA) == 0) {
				} else {
					bpf_printk("Failed to insert VA: %lx to inactive list for PID: %d\n", pid_va->VA, pid_va->PID);
				}
			} else if (pid_va->flags == 1) {
				bpf_printk("Receied endpoint pin\n");
				if (bpf_pin_file_vaddr(pid_va->PID, pid_va->VA) == 0) {
				} else {
					bpf_printk("Failed to pin VA: %lx for PID: %d\n", pid_va->VA, pid_va->PID);
				}

			} else if (pid_va->flags == 2) {
				bpf_printk("Receied endpoint unpin\n");
			} else if (pid_va->flags == 3) {
				bpf_printk("Receied endpoint mark access\n");
			}

			bpf_map_delete_elem(&pid_va_map, &idx);
		}

		idx++;
		if (idx == MAX_PID_VA_ENTRIES) {
			idx = 1; // we skip the first as that is where the index is stored
		}
	}

	return 0;
}

// static long callback_fn(struct bpf_map *map, const void *key, void *value, void *ctx) {
// 	struct pid_va *pid_va;
// 	pid_va = (struct pid_va *)value;

// 	if (pid_va) {
// 		if (pid_va->VA) {
// 			// Call the kfunc to add the VA to the inactive list
// 			if (bpf_insert_file_vaddr_into_inactive_list(pid_va->PID, pid_va->VA) == 0) {
// 				bpf_printk("Inserted VA: %lx to inactive list for PID: %d\n", pid_va->VA, pid_va->PID);
// 			} else {
// 				bpf_printk("Failed to insert VA: %lx to inactive list for PID: %d\n", pid_va->VA, pid_va->PID);
// 			}
// 			bpf_map_delete_elem(map, key);
// 		} else {
// 			bpf_printk("VA is 0\n");
// 		}
// 	} else {
// 		bpf_printk("Value is NULL\n");
// 	}

// 	return 0;	// Return 0 to move to next element
// }

// SEC("kprobe/shrink_lruvec")
// int BPF_KPROBE(shrink_lruvec, struct lruvec *lruvec, struct scan_control *sc) {

// 	long (*cb_p)(struct bpf_map *, const void *, void *, void *) = &callback_fn;
//     bpf_for_each_map_elem(&pid_va_map, cb_p, NULL, 0);

// 	bpf_printk("shrink_lruvec called\n");

// 	return 0;
// }
