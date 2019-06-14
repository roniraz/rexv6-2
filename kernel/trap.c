#include "xv6/types.h"
#include "xv6/defs.h"
#include "xv6/param.h"
#include "xv6/memlayout.h"
#include "xv6/mmu.h"
#include "xv6/proc.h"
#include "xv6/x86.h"
#include "xv6/traps.h"
#include "xv6/spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
uint new_ticks;
void tvinit(void)
{
    int i;

    for (i = 0; i < 256; i++)
        SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
    SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

    initlock(&tickslock, "time");
}

void idtinit(void)
{
    lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void trap(struct trapframe *tf)
{
    if (tf->trapno == T_SYSCALL)
    {
        if (proc->killed)
            exit();
        proc->tf = tf;
        syscall();
        if (proc->killed)
            exit();
        return;
    }

    switch (tf->trapno)
    {
    case T_IRQ0 + IRQ_TIMER:
        if (cpunum() == 0)
        {
            acquire(&tickslock);
            ticks++;
            new_ticks++;
            wakeup(&ticks);
            release(&tickslock);
        }
        else if (cpunum() == 1)
            new_ticks++;
        lapiceoi();
        break;
    case T_IRQ0 + IRQ_IDE:
        ideintr();
        lapiceoi();
        break;
    case T_IRQ0 + IRQ_IDE + 1:
        // Bochs generates spurious IDE1 interrupts.
        break;
    case T_IRQ0 + IRQ_KBD:
        kbdintr(); //handle kbd
        lapiceoi();
        break;
    case T_IRQ0 + IRQ_COM1:
        uartintr();
        lapiceoi();
        break;
    case T_IRQ0 + 7:
    case T_IRQ0 + IRQ_SPURIOUS:
        cprintf("cpu%d: spurious interrupt at %x:%x\n",
                cpunum(), tf->cs, tf->eip);
        lapiceoi();
        break;

    case T_DIVIDE:
        if (VALIDATE_HANDLER(proc->sighandlers[SIGFPE]))
        {
            sigsend(proc->pid, SIGFPE);
        }
        lapiceoi();
        break;

    //PAGEBREAK: 13
    default:
        if (proc == 0 || (tf->cs & 3) == 0)
        {
            // In kernel, it must be our mistake.
            cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
                    tf->trapno, cpunum(), tf->eip, rcr2());
            panic("trap");
        }

        // In user space, assume process misbehaved.
        // Specially, if the trap 14 occurs with address=0x0, it could be a null dereference.
        int addr = rcr2();
        if (tf->trapno == 14 && addr == 0x0)
        {
            cprintf("This trap may indicate a NULL dereference.\n"
                    "pid %d %s: trap %d err %d on cpu %d \n"
                    "eip 0x%x addr 0x%x--kill proc. \n",
                    proc->pid, proc->name, tf->trapno, tf->err, cpunum(), tf->eip,
                    addr);
        }
        else
        {
            cprintf("pid %d %s: trap %d err %d on cpu %d "
                    "eip 0x%x addr 0x%x--kill proc\n",
                    proc->pid, proc->name, tf->trapno, tf->err, cpunum(), tf->eip,
                    addr);
        }

        proc->killed = 1;
    }

    // Force process exit if it has been killed and is in user space.
    // (If it is still executing in the kernel, let it keep running
    // until it gets to the regular system call return.)
    if (proc && proc->killed && (tf->cs & 3) == DPL_USER)
        exit();

    // Force process to give up CPU on clock tick.
    // If interrupts were on while locks held, would need to check nlock.
    if (proc && proc->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER)
        yield();

    // Check if the process has been killed since we yielded
    if (proc && proc->killed && (tf->cs & 3) == DPL_USER)
        exit();
}
