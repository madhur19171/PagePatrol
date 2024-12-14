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

// Map to track VAs containing frequency
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MAX_FREQUENCY_ENTRIES + 1);
    __type(key, __u32); // Key is the index
    __type(value, __u64); // Value is the VA
} va_frequency_array SEC(".maps");

// Map to track the frequency of VAs
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_FREQUENCY_ENTRIES);
    __type(key, __u64); // Key is the VA
	__type(value, __u64); // Value is the frequency
} va_frequency_map SEC(".maps");

// Map to hold the top pinned VAs
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries,  MAX_PINNED_PAGES); // Top 10% and size
    __type(key, __u64); 
	__type(value, __u64); // Key is the VA
} pinned_va_map SEC(".maps");

// Map to hold a heap
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries,  MAX_PINNED_PAGES); // Top 10% and size
    __type(key, __u64); 
	__type(value, __u64); // Key is the VA
} min_heap SEC(".maps");

// Declare the kfunc
extern int bpf_pin_file_vaddr(int pid, unsigned long vaddr) __ksym;
extern int bpf_unpin_file_vaddr(int pid, unsigned long vaddr) __ksym;

char LICENSE[] SEC("license") = "Dual BSD/GPL";

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
    
    // Find size of frequency map
    __u32* frequecy_map_size_ptr = bpf_map_lookup_elem(&va_frequency_array, &zero);
    __u32 frequecy_map_size = frequecy_map_size_ptr ? *frequecy_map_size_ptr : 0;
    if (!frequecy_map_size_ptr) 
    {
        bpf_map_update_elem(&va_frequency_array, &zero, &frequecy_map_size, BPF_ANY);
    }

    __u64 ptr = pid_va->flags;
    int __idx;
    __u64 i;

    __u64 *freq;
    __u64 new_freq;

	for (i = 0; i < MAX_PID_VA_ENTRIES; i++) {
		__idx = (i + ptr) % MAX_PID_VA_ENTRIES;	
        
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
                    freq = bpf_map_lookup_elem(&va_frequency_map, &pid_va->VA);
                    if (freq) {
                        new_freq = *freq + 1;
                        bpf_map_update_elem(&va_frequency_map, &pid_va->VA, &new_freq, BPF_ANY);
                    } else {
                        if (frequecy_map_size == MAX_FREQUENCY_ENTRIES) 
                        {
                            bpf_map_delete_elem(&pid_va_map, &__idx);
                            continue;
                        }
                        frequecy_map_size++;
                        bpf_map_update_elem(&va_frequency_array, &zero, &frequecy_map_size, BPF_ANY);
                        bpf_map_update_elem(&va_frequency_array, &frequecy_map_size, &pid_va->VA, BPF_ANY);

                        new_freq = 1;
                        bpf_map_update_elem(&va_frequency_map, &pid_va->VA, &new_freq, BPF_ANY);
                    }
                }
            }

        bpf_map_delete_elem(&pid_va_map, &__idx);
    }

    int heap_size = 0;
    __u64* va;

    __u64* min_va;
    __u64 *min_freq, *current_freq; 

    // For bubble down
    __u64 index, left_child_index, right_child_index, smallest;
    __u64 *current_frequency, *left_child_frequency, *right_child_frequency;
    __u64 smallest_frequency; 

    // For bubble up
    __u64 parent_index;
    __u64 *parent_frequency;

    __u64 *current, *parent, *left_child, *right_child;
    __u64 *smallest_va;

    for(i = 1; i < MAX_FREQUENCY_ENTRIES; i++) {
        __idx = i;

        va = bpf_map_lookup_elem(&va_frequency_array, &__idx);
        if (!va) {
            break;
        }

        if (heap_size < MAX_PINNED_PAGES) {
            bpf_map_update_elem(&min_heap, &heap_size, va, BPF_ANY);
            // bubble up 
            index = heap_size;

            for (i = 0; i < MAX_PINNED_PAGES; i++) {
                if (index > 0) {
                    break;
                }

                int parent_index = (index - 1) / 2;
                current = bpf_map_lookup_elem(&min_heap, &index);
                parent = bpf_map_lookup_elem(&min_heap, &parent_index);
                if (!current || !parent)
                {
                    return 0;
                }

                current_frequency = bpf_map_lookup_elem(&va_frequency_map, current);
                parent_frequency = bpf_map_lookup_elem(&va_frequency_map, parent);
                if (!current_frequency || !parent_frequency)
                    return 0;

                if (*current_frequency < *parent_frequency) {
                    bpf_map_update_elem(&min_heap, &index, parent, BPF_ANY);
                    bpf_map_update_elem(&min_heap, &parent_index, current, BPF_ANY);
                } else {
                    break;
                }
            }
            // end of bubble up
            heap_size++;
        } else {
            min_va = bpf_map_lookup_elem(&min_heap, &zero);
            if (!min_va) 
                return 0;
            
            min_freq = bpf_map_lookup_elem(&va_frequency_map, min_va);
            current_freq = bpf_map_lookup_elem(&va_frequency_map, va);

            if (!min_freq || !current_freq) 
                return 0;
            
            if (*min_freq < *current_freq) {
                bpf_map_update_elem(&min_heap, &zero, va, BPF_ANY);

                // bubble down
                index = 0;
                for ( i = 0; i < MAX_PINNED_PAGES; i++) {
                    left_child_index = 2 * index + 1;
                    right_child_index = 2 * index + 2;
                    smallest = index;

                    current = bpf_map_lookup_elem(&min_heap, &index);
                    if (!current)
                        return 0;

                    current_frequency = bpf_map_lookup_elem(&va_frequency_map, current);
                    if(!current_frequency)
                        return 0;
                    smallest_frequency = *current_frequency;

                    left_child_frequency = NULL;
                    if (left_child_index < heap_size) {
                        left_child =  bpf_map_lookup_elem(&min_heap, &left_child_index);
                        if(!left_child)
                            return 0;
                        left_child_frequency = bpf_map_lookup_elem(&va_frequency_map, left_child);
                    } 

                    right_child_frequency = NULL;
                    if (right_child_index < heap_size) {
                        right_child = bpf_map_lookup_elem(&min_heap, &right_child_index);
                        if(!right_child)
                            return 0;

                        right_child_frequency = bpf_map_lookup_elem(&va_frequency_map, right_child);
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
                        smallest_va = bpf_map_lookup_elem(&min_heap, &smallest);
                        if(!smallest_va)
                            return 0;
                        bpf_map_update_elem(&min_heap, &index, smallest_va, BPF_ANY);
                        bpf_map_update_elem(&min_heap, &smallest, current, BPF_ANY);

                        index = smallest;
                    } else {
                        break;
                    }
                } 
                // end of bubble down   
            }
        }
    }

    __u64* pinned_va;
    for (i = 0; i < MAX_PINNED_PAGES; i++) {
		__idx = i;	

		pinned_va = bpf_map_lookup_elem(&pinned_va_map, &__idx);

		if (pinned_va) {
		    // call unpin
        }

        bpf_map_delete_elem(&pinned_va_map, &__idx);
    }

    for (i = 0; i < MAX_PINNED_PAGES; i++) {
		__idx = i;

        if (i >= heap_size) {
            break;
        }	

        pinned_va = bpf_map_lookup_elem(&min_heap, &__idx);
        if (!pinned_va) {
            return 0;
        }

        // call pin
        if (bpf_map_update_elem(&pinned_va_map, &__idx, pinned_va, BPF_ANY) < 0) {
            return 0;
        }
    }

	return 0;
}
