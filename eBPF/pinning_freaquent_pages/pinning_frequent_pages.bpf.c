#define BPF_NO_GLOBAL_DATA
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "../../API/common.h"

#define MAX_FREQUENCY_ENTRIES 1000
#define MAX_PINNED_PAGES (MAX_FREQUENCY_ENTRIES / 10)

// Map for input: user-space process provides the virtual address
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_PID_VA_ENTRIES);
	__type(key, __u32);
	__type(value, struct pid_va);
	__uint(pinning, LIBBPF_PIN_BY_NAME);    // Give the map its own file in /sys/fs/bpf/
} pid_va_map SEC(".maps");

// Map to track the frequency of VAs
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_FREQUENCY_ENTRIES);
    __type(key, __u64); // Key is the VA
	__type(value, __u64); // Value is the frequency
} va_frequency_map SEC("maps");

// Map to hold the top pinned VAs
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries,  MAX_PINNED_PAGES); // Top 10% and size
    __type(key, __u64); 
	__type(value, __u64); // Key is the VA
} pinned_va_map SEC("maps");

// Declare the kfunc
extern int bpf_pin_file_vaddr(int pid, unsigned long vaddr) __ksym;
extern int bpf_unpin_file_vaddr(int pid, unsigned long vaddr) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

static __inline void bubble_up(__u64* min_heap, int heap_size) {
    int index = heap_size;

    for (int i = 0; i < MAX_PINNED_PAGES; i++) {
        if (index > 0) {
            break;
        }

        int parent_index = (index - 1) / 2;
        __u64 current = min_heap[index];
        __u64 parent = min_heap[parent_index];

        __u64 *current_frequency = bpf_map_lookup_elem(&va_frequency_map, &current);
        __u64 *parent_frequency = bpf_map_lookup_elem(&va_frequency_map, &parent);
        if (!current_frequency || !parent_frequency)
            return;

        if (*current_frequency < *parent_frequency) {
            min_heap[index] = parent;
            min_heap[parent_index] = current;
        } else {
            break;
        }
    }
}

static __inline void bubble_down(__u64* min_heap, int heap_size) {
    int index = 0;
    for (int i = 0; i < MAX_PINNED_PAGES; i++) {
        int left_child_index = 2 * index + 1;
        int right_child_index = 2 * index + 2;
        int smallest = index;

        int current = min_heap[index];
        __u64 *current_frequency = bpf_map_lookup_elem(&va_frequency_map, &current);
        if(!current_frequency)
            return;
        __u64 smallest_frequency = *current_frequency;

        __u64* left_child_frequency = NULL;
        if (left_child_index < heap_size) {
            __u64 left_child = min_heap[index];
            left_child_frequency = bpf_map_lookup_elem(&va_frequency_map, &left_child);
        } 

        __u64* right_child_frequency = NULL;
        if (right_child_index < heap_size) {
            __u64 right_child = min_heap[index];
            right_child_frequency = bpf_map_lookup_elem(&va_frequency_map, &right_child);
        }

        if (left_child_index < heap_size && left_child_frequency && *left_child_frequency < *current_frequency) {
            smallest = left_child_index;
            smallest_frequency = *left_child_frequency;
        }

        if (right_child_index < heap_size && right_child_frequency && *right_child_frequency < smallest_frequency) {
            smallest = right_child_index;
            smallest_frequency = *right_child_frequency;
        }

        if (smallest != index) {
            min_heap[index] = min_heap[smallest];
            min_heap[smallest] = current;
            index = smallest;
        } else {
            break;
        }
    }
}

static __inline void insert_into_heap(__u64* min_heap, int heap_size, __u64 va) {
    if (heap_size == MAX_PINNED_PAGES) {
        min_heap[0] = va;
        bubble_down(min_heap, heap_size);
    } else {
        min_heap[heap_size] = va;
        bubble_up(min_heap, heap_size);
    }
}

SEC("kprobe/shrink_lruvec")
int BPF_KPROBE(shrink_lruvec, struct lruvec *lruvec, struct scan_control *sc) {
	struct pid_va *pid_va;
    int pid;
    int zero = 0;	// This is necessary to avoid infinite loop error. Otherwise, if &i is directly passed to bpf_map_lookup_elem, 
						// verifier may pessimistically think that bpf_map_lookup_elem can modify the value to -1 and create an infinite loop
	pid_va = bpf_map_lookup_elem(&pid_va_map, &zero);
    if (!pid_va) 
    {
        return 0;
    }
    
    int ptr = pid_va->flags;

	for (int i = 0; i < MAX_PID_VA_ENTRIES; i++) {
		int __idx = (i + ptr) % MAX_PID_VA_ENTRIES;	
        
        if (__idx == zero)
        {
            continue;
        }

		pid_va = bpf_map_lookup_elem(&pid_va_map, &__idx);

		if (pid_va) 
			if (pid_va->VA) {
                pid = pid_va->PID;
                if(pid_va->flags == 3) {
                    // Update VA frequency in the map
                    __u64 *freq = bpf_map_lookup_elem(&va_frequency_map, &pid_va->VA);
                    __u64 new_freq = freq ? (*freq + 1) : 1;
                    bpf_map_update_elem(&va_frequency_map, &pid_va->VA, &new_freq, BPF_ANY);
                }
            }

        bpf_map_delete_elem(&pid_va_map, &__idx);
    }

    __u64 min_heap[MAX_PINNED_PAGES]; // Stores VAs
    int heap_size = 0;
    __u64 key = 0;       
    __u64 next_key = 0;

    if (bpf_map_get_next_key(&va_frequency_map, NULL, &key ) != 0) {
        return 0;
    }

    for(int i = 0; i < MAX_FREQUENCY_ENTRIES; i++) {
        if (heap_size < MAX_PINNED_PAGES) {
            insert_into_heap(min_heap, heap_size, key);
            heap_size++;
        } else {
            __u64 min_va = min_heap[0];
            __u64 *min_freq = bpf_map_lookup_elem(&va_frequency_map, &min_va);
            __u64 *current_freq = bpf_map_lookup_elem(&va_frequency_map, &key);

            if (!min_freq || !current_freq) 
                return 0;
            
            if (*min_freq < *current_freq) {
                insert_into_heap(min_heap, heap_size, key);
            }
        }

        if (bpf_map_get_next_key(&va_frequency_map, &key, &next_key) != 0) {
            break;
        }

        key = next_key;
    }

    for (int i = 0; i < MAX_PINNED_PAGES; i++) {
		int __idx = i;	

		__u64* pinned_va = bpf_map_lookup_elem(&pinned_va_map, &__idx);

		if (pinned_va) {
		    // call unpin
        }

        bpf_map_delete_elem(&pinned_va_map, &__idx);
    }

    for (int i = 0; i < heap_size; i++) {
		int __idx = i;	

        // call pin
        if (bpf_map_update_elem(&pinned_va_map, &__idx, &min_heap[__idx], BPF_ANY) < 0) {
            return 0;
        }
    }

	return 0;
}
