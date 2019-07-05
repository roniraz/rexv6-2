/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-06-01 23:56:40
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-07-04 22:13:19
 * @ Description:
 */

#include "xv6/types.h"
#include "xv6/param.h"
#include "xv6/memlayout.h"
#include "xv6/mmu.h"
#include "xv6/proc.h"
#include "xv6/defs.h"
#include "xv6/x86.h"
#include "xv6/elf.h"
#include "xv6/signal.h"

int exec(char *path, char **argv)
{
    char *s, *last;
    int i, off;
    uint argc, sz, sp, ustack[3 + MAXARG + 1];
    struct elfhdr elf;
    struct inode *ip;
    struct proghdr ph;
    pde_t *pgdir, *oldpgdir;

    begin_op();

    if ((ip = namei(path)) == 0)
    {
        end_op();
        return -1;
    }
    ilock(ip);
    pgdir = 0;

    // Check ELF header
    if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf))
        goto bad;
    if (elf.magic != ELF_MAGIC)
        goto bad;

    if ((pgdir = setupkvm()) == 0)
        goto bad;

    // Load program into memory.
    sz = 0x1000; //skip the first page
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
    {
        if (readi(ip, (char *)&ph, off, sizeof(ph)) != sizeof(ph))
            goto bad;
        if (ph.type != ELF_PROG_LOAD)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;
        if (ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
            goto bad;
        if (ph.vaddr % PGSIZE != 0)
            goto bad;
        if (loaduvm(pgdir, (char *)ph.vaddr, ip, ph.off, ph.filesz) < 0)
            goto bad;
    }
    iunlockput(ip);
    end_op();
    ip = 0;

    // Allocate two pages at the next page boundary.
    // Make the first inaccessible.  Use the second as the user stack.
    // sz = PGROUNDUP(sz);
    // if ((sz = allocuvm(pgdir, sz, sz + 2 * PGSIZE)) == 0)
    //     goto bad;
    // clearpteu(pgdir, (char *)(sz - 2 * PGSIZE));
    // sp = sz;
    sz = PGROUNDUP(sz);
    if ((allocuvm(pgdir, STACKBASE - PGSIZE, STACKBASE)) == 0)
        goto bad;
    //clearpteu(pgdir, (char *)(sz - 2 * PGSIZE));
    sp = STACKBASE;

    // Push argument strings, prepare rest of stack in ustack.
    for (argc = 0; argv[argc]; argc++)
    {
        if (argc >= MAXARG)
            goto bad;
        sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
        if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
        ustack[3 + argc] = sp;
    }
    ustack[3 + argc] = 0;

    ustack[0] = 0xffffffff; // fake return PC
    ustack[1] = argc;
    ustack[2] = sp - (argc + 1) * 4; // argv pointer

    sp -= (3 + argc + 1) * 4;
    if (copyout(pgdir, sp, ustack, (3 + argc + 1) * 4) < 0)
        goto bad;

    // Save program name for debugging.
    for (last = s = path; *s; s++)
        if (*s == '/')
            last = s + 1;
    safestrcpy(proc->name, last, sizeof(proc->name));

    // Commit to the user image.
    oldpgdir = proc->pgdir;
    proc->pgdir = pgdir;
    proc->sz = sz;
    proc->tf->eip = elf.entry; // main
    proc->tf->esp = sp;
    proc->stk_sz = 1;

    for (int i = SIGNAL_MIN; i < SIGNAL_MAX; i++)
    {
        proc->sighandlers[i] = (sighandler_t)-1;
    }

    switchuvm(proc);
    freevm(oldpgdir);
    return 0;

bad:
    if (pgdir)
        freevm(pgdir);
    if (ip)
    {
        iunlockput(ip);
        end_op();
    }
    return -1;
}
