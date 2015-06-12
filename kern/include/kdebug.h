#ifndef ROS_KERN_KDEBUG_H
#define ROS_KERN_KDEBUG_H

#include <ros/common.h>
#include <ros/trapframe.h>
#include <arch/kdebug.h>

struct symtab_entry {
	char *name;
	uintptr_t addr;
};

void backtrace(void);
void backtrace_frame(uintptr_t pc, uintptr_t fp);
size_t backtrace_list(uintptr_t pc, uintptr_t fp, uintptr_t *pcs,
                      size_t nr_slots);
void backtrace_kframe(struct hw_trapframe *hw_tf);

/* Arch dependent, listed here for ease-of-use */
static inline uintptr_t get_caller_pc(void);

/* Returns a null-terminated string with the function name for a given PC /
 * instruction pointer.  kfree() the result. */
char *get_fn_name(uintptr_t pc);

/* Returns the address of sym, or 0 if it does not exist */
uintptr_t get_symbol_addr(char *sym);

/* For a poor-mans function tracer (can add these with spatch) */
void __print_func_entry(const char *func, const char *file);
void __print_func_exit(const char *func, const char *file);
#define print_func_entry() __print_func_entry(__FUNCTION__, __FILE__)
#define print_func_exit() __print_func_exit(__FUNCTION__, __FILE__)
void hexdump(void *v, int length);
void pahexdump(uintptr_t pa, int length);
int printdump(char *buf, int buflen, uint8_t *data);

extern bool printx_on;
void set_printx(int mode);
#define printx(args...) if (printx_on) printk(args)
#define trace_printx(args...) if (printx_on) trace_printk(args)

#include <oprofile.h>
#define TRACEME() oprofile_add_backtrace(read_pc(), read_bp())

void debug_addr_proc(struct proc *p, unsigned long addr);
void debug_addr_pid(int pid, unsigned long addr);

#endif /* ROS_KERN_KDEBUG_H */
