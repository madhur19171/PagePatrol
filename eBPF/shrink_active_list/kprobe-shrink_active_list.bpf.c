#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("kprobe/shrink_active_list")
int BPF_KPROBE(shrink_active_list, unsigned long nr_to_scan,
			       struct lruvec *lruvec,
			       struct scan_control *sc,
			       enum lru_list lru)
{
	pid_t pid;

	pid = bpf_get_current_pid_tgid() >> 32;

	bpf_printk("KPROBE ENTRY pid = %d, nr_to_scan = %d\n", pid, nr_to_scan);
	return 0;
}

// SEC("kretprobe/shrink_inactive_list")
// int BPF_KRETPROBE(shrink_inactive_list_exit, long ret)
// {
// 	pid_t pid;

// 	pid = bpf_get_current_pid_tgid() >> 32;
// 	bpf_printk("KPROBE EXIT: pid = %d, nr_reclaimed = %ld\n", pid, ret);
// 	return 0;
// }