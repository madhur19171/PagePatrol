#ifndef __PAGE_PATROL_H__
#define __PAGE_PATROL_H__

#include <stdint.h>
#include <stdio.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <unistd.h>
#include "common.h"

struct pid_va pid_va;
int pid_va_map_fd;
int idx;

inline int init_page_patrol () {
	idx = 0;

	pid_va.flags = 0;
	pid_va.VA = 0;
	pid_va.PID = getpid();

	// Open the map
	pid_va_map_fd = bpf_obj_get(PID_VA_MAP);
	if (pid_va_map_fd < 0) {
		perror("Failed to open PID VA map");
		return -1;
	}

	return 0;
}

inline void mark_va_for_eviction (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 0;	// Eviction
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Evicting VA(0x%lx)\n", pid_va.VA);
	} else {
		idx++;
		idx %= MAX_PID_VA_ENTRIES;    // Max size of PID_VA map
	}
}

inline void pin_va (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 1;	// Pinning
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Pinning VA(0x%lx)\n", pid_va.VA);
	} else {
		idx++;
		idx %= MAX_PID_VA_ENTRIES;    // Max size of PID_VA map
	}
}

inline void unpin_va (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 2;	// Unpinning
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Pinning VA(0x%lx)\n", pid_va.VA);
	} else {
		idx++;
		idx %= MAX_PID_VA_ENTRIES;    // Max size of PID_VA map
	}
}

#endif