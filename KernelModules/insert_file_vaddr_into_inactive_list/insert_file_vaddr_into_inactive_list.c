#include <linux/module.h>     // Core header for loading modules
#include <linux/kernel.h>     // Kernel logging macros
#include <linux/bpf.h>
#include <linux/btf.h>
#include <linux/btf_ids.h>
#include <linux/bpf_verifier.h>
#include <linux/mm.h>
#include <linux/memcontrol.h>
#include <linux/mm_inline.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pgtable.h>

__bpf_kfunc int bpf_insert_file_vaddr_into_inactive_list(int pid, unsigned long vaddr);

/* Begin kfunc definitions */
__bpf_kfunc_start_defs();

/* Define the bpf_strstr kfunc */
__bpf_kfunc int bpf_insert_file_vaddr_into_inactive_list(int pid, unsigned long vaddr) {	// __ign suffix prevents the verifier from doing memory checking. https://stackoverflow.com/questions/79045875/how-to-create-a-new-kfunc-and-pass-an-entry-from-an-ebpf-map-to-it-as-an-argumen
	struct mm_struct *mm;
	struct page *page;
	struct pid *spid;
	struct task_struct *task;
	pte_t *pte;
	spinlock_t *ptl;
	struct folio *folio;
	struct mem_cgroup *memcg;
	pg_data_t *pgdat;
	struct lruvec *lruvec;

	spid = find_get_pid(pid);

	task = get_pid_task(spid, PIDTYPE_PID);

	// Validate inputs
	if (!task || !vaddr || !spid)
		return -EINVAL;

	mm = get_task_mm(task);
	if (!mm)
		return -ESRCH; // No memory structure for this task

	memcg = get_mem_cgroup_from_mm(mm);

	pte = get_locked_pte(mm, vaddr, &ptl);

	page = pte_page(*pte);

	// Release PTE lock
	pte_unmap_unlock(pte, ptl);

	folio = page_folio(page);

	pgdat = page_pgdat(&folio->page);

	lruvec = mem_cgroup_lruvec(memcg, pgdat);

	// Removing from current list
	lruvec_del_folio(lruvec, folio);

	// Adding to INACTIVE_FILE list
	enum lru_list lru = folio_lru_list(folio);
	update_lru_size(lruvec, LRU_INACTIVE_FILE, folio_zonenum(folio),
			folio_nr_pages(folio));
	if (lru != LRU_UNEVICTABLE)
		list_add(&folio->lru, &lruvec->lists[LRU_INACTIVE_FILE]);

	// Release the page reference
	put_page(page);

	mmput(mm);

	return 0;
}


/* End kfunc definitions */
__bpf_kfunc_end_defs();

/* Define the BTF kfuncs ID set */
BTF_KFUNCS_START(bpf_kfunc_example_ids_set)
BTF_ID_FLAGS(func, bpf_insert_file_vaddr_into_inactive_list)
BTF_KFUNCS_END(bpf_kfunc_example_ids_set)

/* Register the kfunc ID set */
static const struct btf_kfunc_id_set bpf_kfunc_example_set = {
	.owner = THIS_MODULE,
	.set = &bpf_kfunc_example_ids_set,
};

/* Function executed when the module is loaded */
static int __init insert_file_vaddr_into_inactive_list_init(void)
{
	int ret;

	printk(KERN_INFO "Registering kfunc for insertion into LRUn.\n");
	/* Register the BTF kfunc ID set for BPF_PROG_TYPE_KPROBE */
	ret = register_btf_kfunc_id_set(BPF_PROG_TYPE_KPROBE, &bpf_kfunc_example_set);
	if (ret)
	{
		pr_err("bpf_kfunc_example: Failed to register BTF kfunc ID set\n");
		return ret;
	}
	printk(KERN_INFO "bpf_kfunc_example: Module loaded successfully\n");
	return 0; // Return 0 if successful
}

/* Function executed when the module is removed */
static void __exit insert_file_vaddr_into_inactive_list_exit(void)
{
	/* Unregister the BTF kfunc ID set */
	printk(KERN_INFO "kfunc for LRU insertion unregistered!\n");
}

/* Macros to define the module’s init and exit points */
module_init(insert_file_vaddr_into_inactive_list_init);
module_exit(insert_file_vaddr_into_inactive_list_exit);

MODULE_LICENSE("GPL");                 // License type (GPL)
MODULE_AUTHOR("Madhur");            // Module author
MODULE_DESCRIPTION("LRU_insertion_module"); // Module description
MODULE_VERSION("1.0");