#define BPF_NO_GLOBAL_DATA
// #include <linux/bpf.h>
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define MAX_PID_VA_ENTRIES 16

struct pid_va {
	int PID;
	unsigned long VA;	// A VA of 0 means that the array entry is Invalid and the
						// There is no need for adding this to the inactive list
};

// Map for input: user-space process provides the virtual address
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_PID_VA_ENTRIES);
	__type(key, __u32);
	__type(value, struct pid_va);
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} pid_va_map SEC(".maps");

// Declare the kfunc
extern int bpf_insert_file_vaddr_into_inactive_list(int pid, unsigned long vaddr) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/shrink_lruvec")
int BPF_KPROBE(shrink_lruvec, struct lruvec *lruvec, struct scan_control *sc) {
	struct pid_va *pid_va;

	for (int i = 0; i < MAX_PID_VA_ENTRIES; i++) {
		int __idx = i;	// This is necessary to avoid infinite loop error. Otherwise, if &i is directly passed to bpf_map_lookup_elem, 
						// verifier may pessimistically think that bpf_map_lookup_elem can modify the value to -1 and create an infinite loop
		pid_va = bpf_map_lookup_elem(&pid_va_map, &__idx);

		if (pid_va) {
			if (pid_va->VA) {
				// Call the kfunc to add the VA to the inactive list
				if (bpf_insert_file_vaddr_into_inactive_list(pid_va->PID, pid_va->VA) == 0) {
					bpf_printk("Inserted VA: %lx to inactive list for PID: %d\n", pid_va->VA, pid_va->PID);
				} else {
					bpf_printk("Failed to insert VA: %lx to inactive list for PID: %d\n", pid_va->VA, pid_va->PID);
				}
			}
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
