#include <linux/module.h>     // Core header for loading modules
#include <linux/kernel.h>     // Kernel logging macros
#include <linux/bpf.h>
#include <linux/btf.h>
#include <linux/btf_ids.h>
#include <linux/bpf_verifier.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pgtable.h>

// Structure to hold the result
struct va_to_pa_result {
	unsigned long physical_address;
	int error_code;
};

__bpf_kfunc int bpf_translate_va_to_pa(int pid, unsigned long vaddr, struct va_to_pa_result *result);

/* Begin kfunc definitions */
__bpf_kfunc_start_defs();

/* Define the bpf_strstr kfunc */
__bpf_kfunc int bpf_translate_va_to_pa(int pid, unsigned long vaddr, struct va_to_pa_result *result) {	// __ign suffix prevents the verifier from doing memory checking. https://stackoverflow.com/questions/79045875/how-to-create-a-new-kfunc-and-pass-an-entry-from-an-ebpf-map-to-it-as-an-argumen
	struct mm_struct *mm;
	struct page *page;
	unsigned long paddr;
	struct pid * spid;
	struct task_struct * task;
	int ret;

	spid = find_get_pid(pid);
	task = get_pid_task(spid, PIDTYPE_PID);

	// Validate inputs
	if (!task || !vaddr || !result)
		return -EINVAL;

	mm = get_task_mm(task);
	if (!mm)
		return -ESRCH; // No memory structure for this task

	// Pin the user page
	ret = pin_user_pages_remote(mm, vaddr, 1, FOLL_GET | FOLL_WRITE, &page, NULL);
	if (ret <= 0) {
		mmput(mm);
		result->physical_address = 0;
		result->error_code = ret;
		return 0;
	}

	// Convert page to physical address
	paddr = page_to_phys(page) + (vaddr & ~PAGE_MASK);

	// Populate the result
	result->physical_address = paddr;
	result->error_code = 0;

	// Release the page reference
	put_page(page);

	mmput(mm);

	return 0;
}


/* End kfunc definitions */
__bpf_kfunc_end_defs();

/* Define the BTF kfuncs ID set */
BTF_KFUNCS_START(bpf_kfunc_example_ids_set)
BTF_ID_FLAGS(func, bpf_translate_va_to_pa)
BTF_KFUNCS_END(bpf_kfunc_example_ids_set)

/* Register the kfunc ID set */
static const struct btf_kfunc_id_set bpf_kfunc_example_set = {
	.owner = THIS_MODULE,
	.set = &bpf_kfunc_example_ids_set,
};

/* Function executed when the module is loaded */
static int __init va_to_pa_kfunc_init(void)
{
	int ret;

	printk(KERN_INFO "Registering kfunc for VA to PA translation.\n");
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
static void __exit va_to_pa_kfunc_exit(void)
{
	/* Unregister the BTF kfunc ID set */
	printk(KERN_INFO "kfunc for VA to PA translation unregistered!\n");
}

/* Macros to define the module’s init and exit points */
module_init(va_to_pa_kfunc_init);
module_exit(va_to_pa_kfunc_exit);

MODULE_LICENSE("GPL");                 // License type (GPL)
MODULE_AUTHOR("Madhur");            // Module author
MODULE_DESCRIPTION("address_translation_module"); // Module description
MODULE_VERSION("1.0");  