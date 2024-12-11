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
	idx = 1; // we skip zero as that is where we write the index

	pid_va.flags = 0; // this field is set to the api
	pid_va.VA = 0; // this field is set to the api
	pid_va.PID = getpid();

	// Open the map
	pid_va_map_fd = bpf_obj_get(PID_VA_MAP);
	if (pid_va_map_fd < 0) {
		perror("Failed to open PID VA map");
		return -1;
	}

	// write the index to map
	int zero = 0;
	if (bpf_map_update_elem(pid_va_map_fd, &zero, &idx, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for index %d)\n", idx);
		return -1;
	}

	return 0;
}

inline int mark_va_for_eviction (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 0;	// Eviction
	
	// write the value
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Evicting VA(0x%lx)\n", pid_va.VA);
		return -1;
	}

	idx++;
	if (idx == MAX_PID_VA_ENTRIES) {
		idx = 1; // we must skip zero as that is where we place the index
	}

	// write the new index
	int zero = 0;
	if (bpf_map_update_elem(pid_va_map_fd, &zero, &idx, BPF_ANY) < 0) {
		printf("Failed to update index %d)\n", idx);
		return -1;
	}

	return 0;
}

inline int pin_va (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 1;	// Pinning
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Pinning VA(0x%lx)\n", pid_va.VA);
		return -1;
	}

	idx++;
	if (idx == MAX_PID_VA_ENTRIES) {
		idx = 1; // we must skip zero as that is where we place the index
	}

	// write the new index
	int zero = 0;
	if (bpf_map_update_elem(pid_va_map_fd, &zero, &idx, BPF_ANY) < 0) {
		printf("Failed to update index %d)\n", idx);
		return -1;
	}

	return 0;
}

inline int unpin_va (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 2;	// Unpinning
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Pinning VA(0x%lx)\n", pid_va.VA);
		return -1;
	}

	idx++;
	if (idx == MAX_PID_VA_ENTRIES) {
		idx = 1; // we must skip zero as that is where we place the index
	}

	// write the new index
	int zero = 0;
	if (bpf_map_update_elem(pid_va_map_fd, &zero, &idx, BPF_ANY) < 0) {
		printf("Failed to update index %d)\n", idx);
		return -1;
	}

	return 0;
}

inline int access_va (void * va) {
	pid_va.VA = (unsigned long)(va);
	pid_va.flags = 3;	// access info
	
	if (bpf_map_update_elem(pid_va_map_fd, &idx, &pid_va, BPF_ANY) < 0) {
		printf("Failed to update PID VA map for Pinning VA(0x%lx)\n", pid_va.VA);
		return -1;
	}

	idx++;
	if (idx == MAX_PID_VA_ENTRIES) {
		idx = 1; // we must skip zero as that is where we place the index
	}

	// write the new index
	int zero = 0;
	if (bpf_map_update_elem(pid_va_map_fd, &zero, &idx, BPF_ANY) < 0) {
		printf("Failed to update index %d)\n", idx);
		return -1;
	}


	return 0;
}

#endif