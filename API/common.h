#ifndef __COMMON_H__
#define __COMMON_H__

#define PID_VA_MAP "/sys/fs/bpf/pid_va_map"

#define MAX_PID_VA_ENTRIES 1024

struct pid_va {
	int flags;	// Flag sotres what kind of operation to perform.
				// 0 means evict, 
				// 1 means pin, 
				// 2 means unpin, 
				// 3 means access info 
	int PID;    // used for the eBPF to translate the address
	unsigned long VA;	// A VA of 0 means that the array entry is Invalid and the
						// There is no need for adding this to the inactive list
};

#endif