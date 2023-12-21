#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/unistd.h>

#include <linux/string.h>

#include <asm/special_insns.h>
#include <asm/processor-flags.h>

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

//syscall table adr
uint64_t *syscall_table = 0;


typedef int (*sysfun_t)(struct pt_regs *);

//adr of original syscall open
sysfun_t original_open;


int my_open_hook(struct pt_regs *regs){
    char *path = (char*)regs->di;

    if(strcmp(path, "text.txt") == 0){
	pr_alert("Open text.txt");
    }
    int rtn = original_open(regs);

    return rtn;
}

void cr0_write(unsigned long val){
    asm volatile("mov %0,%%cr0"
		 : "+r"(val)
		 :
		 : "memory");
}

static inline unsigned long unprotect_memory(void){
    unsigned long cr0;
    unsigned long newcr0;

    cr0 = native_read_cr0();
    newcr0 = cr0 & ~(X86_CR0_WP);
    cr0_write(newcr0);
    return cr0;
}

static inline void protect_memory(unsigned long cr0){
    cr0_write(cr0);
}

int __init my_open_init(void) {

    /* Get syscall table adr */
    // Adr of syscall table -> kp.addr
    register_kprobe(&kp);

    typedef void *(*kallsyms_t)(const char *);
    kallsyms_t lookup_name;

    lookup_name = (kallsyms_t) (kp.addr);

    syscall_table = lookup_name("sys_call_table");

    /* Replace adr of original syscall with the hook function  */
    unsigned long old_cr0;
    old_cr0 = unprotect_memory();

    original_open = (sysfun_t) syscall_table[__NR_open];
    syscall_table[__NR_open] = (uint64_t) my_open_hook;

    protect_memory(old_cr0);

    return 0;
}

void __exit my_open_exit(void) {
    /* Replace the hook function with the adr of original syscall */
    unsigned long old_cr0;
    old_cr0 = unprotect_memory();

    syscall_table[__NR_open] = original_open;

    protect_memory(old_cr0);
    unregister_kprobe(&kp);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Open module");
MODULE_AUTHOR("Author");

module_init(my_open_init);
module_exit(my_open_exit);
